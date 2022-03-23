
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
    TaskI2CMaster(Scheduler* aScheduler = NULL) : Task(TASK_IMMEDIATE, TASK_ONCE, aScheduler, false)
    {
        instance_ = this;
    }

    virtual bool Callback() override
    {
        if (Wire.available() > 0)
        {
        }

        return true;
    }

    void setPosition(byte motor, int16_t position)
    {
        Wire.beginTransmission(1);

        Wire.write((uint8_t)motor);
        Wire.write(reinterpret_cast<uint8_t*>(&position), sizeof(position));

        Wire.endTransmission();
    }

    int16_t position(byte motor) const { return 0; }

    using Task::enable;

protected:
    virtual bool OnEnable() override
    {
        Serial.print("Wire ininitiazed");
        Wire.begin();  // Activate I2C network
        Wire.onReceive(&TaskI2CMaster::receiveEvent);
        Serial.print("Wire ininitiazed");
        return true;
    }

    virtual void OnDisable() override {}

private:
    static void receiveEvent(int howMany)
    {
        if (Wire.available() < sizeof(byte) + sizeof(int16_t)) return;

        TaskI2CMaster* me = TaskI2CMaster::instance_;
    }

private:
    static TaskI2CMaster* instance_;
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
        taskI2C_.setPosition(idx == posCard_ ? 0 : 1, (int16_t)value);
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
    int posCard_;
    int posBug_;
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
