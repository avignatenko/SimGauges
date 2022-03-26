
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

class TaskI2CMaster : private Task
{
public:
    TaskI2CMaster(Scheduler* aScheduler = NULL) : Task(10 * TASK_MILLISECOND, TASK_FOREVER, aScheduler, false)
    {
        instance_ = this;
    }

    virtual bool Callback() override
    {
        // right
        if (encoders_[0].direction != 0)
        {
            long accel = constrain(100 / encoders_[0].millis, 1, 10);
            setPosition(1, motorPos_[1] + encoders_[0].direction * 12 * accel);

            encoders_[0].direction = 0;
        }

        // left
        if (encoders_[1].direction != 0)
        {
            long accel = constrain(100 / encoders_[1].millis, 1, 10);
            setPosition(1, motorPos_[1] + encoders_[1].direction * 12 * accel);

            setPosition(0, motorPos_[0] - encoders_[1].direction * 12 * accel);

            encoders_[1].direction = 0;
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
        if (Wire.available() < sizeof(uint8_t) + sizeof(int8_t) + sizeof(long)) return;

        TaskI2CMaster* me = TaskI2CMaster::instance_;

        uint8_t encoder = Wire.read();
        int8_t direction = Wire.read();

        long millis = 0;
        Wire.readBytes((uint8_t*)&millis, sizeof(millis));

        me->encoders_[encoder].direction = direction;
        me->encoders_[encoder].millis = millis;
    }

private:
    static TaskI2CMaster* instance_;
    int16_t motorPos_[2] = {0};

    struct Encoder
    {
        int16_t direction;
        long millis;
    };

    volatile Encoder encoders_[2] = {{0, 0}, {0, 0}};
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

    virtual void setLPos(byte idx, float value, bool absolute) override
    {
        BasicInstrument::setLPos(idx, value, absolute);

        // get current angle and calculate delta
        uint8_t curPosIdx = (idx == posCard_ ? 0 : 1);
        float curLPos = taskI2C_.position(curPosIdx) / 12 % 360;
        float delta = fmod(value - curLPos + 540, 360) - 180;
        float newValue = curLPos + delta;

        float offset = getVar(idx == posCard_ ? varCardCal_ : varBugCal_);

        taskI2C_.setPosition(curPosIdx, (int16_t)(newValue * 12 + offset));
    }

private:
    virtual void onCANReceived(byte priority, byte port, uint16_t srcAddress, uint16_t dstAddress, byte len,
                               byte* payload) override
    {
        if (len != 4) return;

        float pos = *reinterpret_cast<float*>(payload);
        setLPos(0, pos, true);
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
