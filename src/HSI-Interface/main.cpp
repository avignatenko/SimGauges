
#include <Common.h>

#include <BasicInstrument.h>
#include <Interpolation.h>
#include <TaskButton.h>
#include <TaskCAN.h>
#include <TaskErrorLed.h>

#include <Wire.h>

// hardware speficics
const byte BUTTON_PORT = 7;
const byte LED_PORT = 3;
const byte MCP2515_SPI_PORT = 10;
const byte MCP2515_INT_PIN = 2;

struct MotorUpdate
{
    uint8_t calibration;  // 0 - not calibrated, 1 - in progress, 2 - calibrated
    int16_t posCard;
    int16_t posBug;
    int8_t direction1;
    long millis1;
    int8_t direction2;
    long millis2;
};

class TaskI2CMaster : private Task
{
public:
    TaskI2CMaster(Scheduler* aScheduler = NULL) : Task(10 * TASK_MILLISECOND, TASK_FOREVER, aScheduler, false)
    {
        instance_ = this;
    }

    virtual bool Callback() override
    {
        if (!encodersCallback_) return false;

        // right
        if (m_.direction1 != 0)
        {
            long accel = constrain(100 / m_.millis1, 1, 10);

            encodersCallback_(0, accel * m_.direction1);

            m_.direction1 = 0;
        }

        // left
        if (m_.direction2 != 0)
        {
            float accel = constrain(100 / m_.millis2, 1, 10);

            encodersCallback_(1, accel * m_.direction2);

            m_.direction2 = 0;
        }
        return true;
    }

    void setPosition(byte motor, int16_t position)
    {
        Wire.beginTransmission(2);

        Wire.write((uint8_t)motor);
        Wire.write(reinterpret_cast<uint8_t*>(&position), sizeof(position));

        Wire.endTransmission();

        motorPos_[motor] = position;
    }

    int16_t position(byte motor) const { return motorPos_[motor]; }

    void requestUpdate() { Wire.requestFrom(2, sizeof(MotorUpdate)); }

    void setEncodersCallback(fastdelegate::FastDelegate2<int8_t, float> callback) { encodersCallback_ = callback; }

    using Task::enable;

protected:
    virtual bool OnEnable() override
    {
        Serial.print("Wire ininitiazed");
        Wire.begin(1);  // Activate I2C network
        Wire.onReceive(&TaskI2CMaster::receiveEvent);
        Serial.print("Wire ininitiazed");
        return true;
    }

    virtual void OnDisable() override {}

private:
    static void receiveEvent(int howMany)
    {
        if (Wire.available() < sizeof(MotorUpdate)) return;

        TaskI2CMaster* me = TaskI2CMaster::instance_;

        Wire.readBytes((uint8_t*)&me->m_, sizeof(MotorUpdate));
    }

private:
    static TaskI2CMaster* instance_;
    int16_t motorPos_[2] = {0};

    volatile MotorUpdate m_;

    fastdelegate::FastDelegate2<int8_t, float> encodersCallback_;
};

TaskI2CMaster* TaskI2CMaster::instance_ = nullptr;

class GenericSingleNeedleInstrument : public BasicInstrument
{
public:
    GenericSingleNeedleInstrument(byte ledPin, byte buttonPin, byte canSPIPin, byte canIntPin)
        : BasicInstrument(ledPin, buttonPin, canSPIPin, canIntPin), taskI2C_(&taskManager_)
    {
        posCard_ = addPos("card");
        posBug_ = addPos("bug");

        varCardCal_ = addVar("cardcal");
        varBugCal_ = addVar("bugcal");

        taskI2C_.setEncodersCallback(
            fastdelegate::FastDelegate2<int8_t, float>(this, &GenericSingleNeedleInstrument::onEncoders));
    }

    void setup()
    {
        BasicInstrument::setup();
        taskI2C_.enable();
    }

protected:
    virtual void setVar(byte idx, float value) override { BasicInstrument::setVar(idx, value); }

    virtual int32_t posForLut(byte idxLut) override { return taskI2C_.position(idxLut); }
    virtual int32_t pos(byte idxPos) override { return taskI2C_.position(idxPos == posCard_ ? 0 : 1); }

    virtual void setPos(byte idx, int32_t value, bool absolute) override
    {
        BasicInstrument::setPos(idx, value, absolute);

        int32_t pos = (absolute ? value : taskI2C_.position(idx) + value);
        taskI2C_.setPosition(idx == posCard_ ? 0 : 1, pos);
    }

    int16_t angleToSteps(float angle) { return static_cast<int16_t>(angle * 12); }
    float stepsToAngle(int16_t steps) { return static_cast<float>(steps) / 12; }

    float lPos(byte idx)
    {
        uint8_t curPosIdx = (idx == posCard_ ? 0 : 1);
        int16_t offset = getVar(idx == posCard_ ? varCardCal_ : varBugCal_);

        float curLPos = stepsToAngle(taskI2C_.position(curPosIdx) - offset);

        if (idx == posBug_)
        {
            float lPosCard = lPos(posCard_);
            curLPos += lPosCard;
        }
        return curLPos;
    }

    void setLPosInternal(byte idx, float value)
    {
        if (idx == posBug_)
        {
            float lPosCard = lPos(posCard_);
            value -= lPosCard;
        }

        uint8_t curPosIdx = (idx == posCard_ ? 0 : 1);
        int16_t offset = getVar(idx == posCard_ ? varCardCal_ : varBugCal_);

        int16_t newPos = angleToSteps(value) + offset;
        int16_t posDelta = newPos - taskI2C_.position(curPosIdx);
        taskI2C_.setPosition(curPosIdx, newPos);

        if (idx == posCard_)
        {
            taskI2C_.setPosition(1, taskI2C_.position(1) - posDelta);
        }
    }

    virtual void setLPos(byte idx, float value, bool absolute) override
    {
        value = fmod(value, 360);

        BasicInstrument::setLPos(idx, value, absolute);

        // get current angle and calculate delta
        float curLPos = lPos(idx);
        float delta = fmod(value - curLPos + 540, 360) - 180;
        float newValue = curLPos + delta;

        setLPosInternal(idx, newValue);
    }

private:
    virtual void onCANReceived(byte priority, byte port, uint16_t srcAddress, uint16_t dstAddress, byte len,
                               byte* payload) override
    {
        if (len != 4) return;

        float pos = *reinterpret_cast<float*>(payload);
        setLPos(port, pos, true);
    }

    void onEncoders(int8_t encoder, float speed)
    {
        if (encoder == 0)
        {
            // taskI2C_.setPosition(1, taskI2C_.position(1) + speed * 12);
            taskCAN_.sendMessage(0, 1, 0, sizeof(speed), reinterpret_cast<byte*>(&speed));
        }
        else
        {
            //taskI2C_.setPosition(1, taskI2C_.position(1) + speed * 12);
            //taskI2C_.setPosition(0, taskI2C_.position(0) - speed * 12);

            taskCAN_.sendMessage(0, 0, 0, sizeof(speed), reinterpret_cast<byte*>(&speed));
        }
    }

private:
    TaskI2CMaster taskI2C_;
    byte posCard_;
    byte posBug_;

    byte varCardCal_;
    byte varBugCal_;
};

GenericSingleNeedleInstrument s_instrument(LED_PORT, BUTTON_PORT, MCP2515_SPI_PORT, MCP2515_INT_PIN);

void setup()
{
    Serial.begin(9600);
    s_instrument.setup();
}

void loop()
{
    s_instrument.run();
}
