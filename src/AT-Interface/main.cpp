
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
    int16_t posPitch;
    int16_t posRoll;
};

class TaskI2CMaster : private Task
{
public:
    TaskI2CMaster(Scheduler* aScheduler = NULL) : Task(10 * TASK_MILLISECOND, TASK_FOREVER, aScheduler, false)
    {
        instance_ = this;
    }

    virtual bool Callback() override { return true; }

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
};

TaskI2CMaster* TaskI2CMaster::instance_ = nullptr;

class ATInterfaceInstrument : public BasicInstrument
{
public:
    ATInterfaceInstrument(byte ledPin, byte buttonPin, byte canSPIPin, byte canIntPin)
        : BasicInstrument(ledPin, buttonPin, canSPIPin, canIntPin), taskI2C_(&taskManager_)
    {
        posPitch_ = addPos("pitch");
        posRoll_ = addPos("roll");

        pitchLUT_ = addLUT("pitch", 10);

        varRollCal_ = addVar("rollcal");
    }

    void setup()
    {
        BasicInstrument::setup();
        taskI2C_.enable();
    }

protected:
    virtual int32_t posForLut(byte idxLut) override { return taskI2C_.position(idxLut == pitchLUT_ ? posPitch_ : 0); }
    virtual int32_t pos(byte idxPos) override { return taskI2C_.position(idxPos == posPitch_ ? 0 : 1); }

    virtual void setPos(byte idx, int32_t value, bool absolute) override
    {
        BasicInstrument::setPos(idx, value, absolute);

        int32_t pos = (absolute ? value : taskI2C_.position(idx) + value);
        taskI2C_.setPosition(idx == posPitch_ ? 0 : 1, pos);
    }

    static constexpr float kStepAngleRoll = 0.9;
    static constexpr int kMicroStepsRoll = 16;
    static constexpr float kGearRatioRoll = 18.0 / 10.0;

    static int32_t angleToStepsRoll(float angle)
    {
        return static_cast<int32_t>(angle * kMicroStepsRoll / kStepAngleRoll * kGearRatioRoll);
    }
    static float stepsToAngleRoll(int16_t steps)
    {
        return static_cast<float>(steps) / kMicroStepsRoll * kStepAngleRoll / kGearRatioRoll;
    }

    float lPosRoll()
    {
        uint8_t curPosIdx = 1;
        int16_t offset = getVar(varRollCal_);
        float curLPos = stepsToAngleRoll(taskI2C_.position(curPosIdx) - offset);
        return curLPos;
    }

    void setLPosRollInternal(float value)
    {
        uint8_t curPosIdx = 1;
        int16_t offset = getVar(varRollCal_);

        int16_t newPos = angleToStepsRoll(value) + offset;
        taskI2C_.setPosition(curPosIdx, newPos);
    }

    virtual void setLPos(byte idx, float value, bool absolute) override
    {
        BasicInstrument::setLPos(idx, value, absolute);

        value = fmod(value, 360);

        if (idx == posRoll_)
        {
            // get current angle and calculate delta (minimum distance)
            float curLPos = lPosRoll();
            float delta = fmod(value - curLPos + 540, 360) - 180;
            float newValue = curLPos + delta;

            setLPosRollInternal(newValue);
        }
        else if (idx == posPitch_)
        {
            StoredLUT& lut = getLUT(pitchLUT_);
            int16_t p = (int16_t)(catmullSplineInterpolate<double, double>(lut.x(), lut.y(), lut.size(), value));
            taskI2C_.setPosition(0, p);
        }
    }

private:
    virtual void onCANReceived(byte priority, byte port, uint16_t srcAddress, uint16_t dstAddress, byte len,
                               byte* payload) override
    {
        if (len != 4) return;

        float pos = *reinterpret_cast<float*>(payload);
        setLPos(port, pos, true);
    }

private:
    TaskI2CMaster taskI2C_;
    byte posPitch_;
    byte posRoll_;
    byte pitchLUT_;

    byte varRollCal_;
};

ATInterfaceInstrument s_instrument(LED_PORT, BUTTON_PORT, MCP2515_SPI_PORT, MCP2515_INT_PIN);

void setup()
{
    Serial.begin(9600);
    s_instrument.setup();
}

void loop()
{
    s_instrument.run();
}
