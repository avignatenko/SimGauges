
#include <Common.h>

#include <BasicInstrument.h>
#include <Interpolation.h>
#include <TaskButton.h>
#include <TaskCAN.h>
#include <TaskErrorLed.h>
#include <TaskStepperX27Driver.h>

// hardware speficics
const byte BUTTON_PORT = 7;
const byte LED_PORT = 3;
const byte MCP2515_SPI_PORT = 10;
const byte MCP2515_INT_PIN = 2;

const byte STEPPER_STEP = A0;
const byte STEPPER_DIR = A1;
const byte STEPPER_RESET = A2;

class GenericSingleNeedleInstrument : public BasicInstrument
{
public:
    GenericSingleNeedleInstrument(byte ledPin, byte buttonPin, byte canSPIPin, byte canIntPin)
        : BasicInstrument(ledPin, buttonPin, canSPIPin, canIntPin),
          taskStepper_(taskManager_, STEPPER_STEP, STEPPER_DIR, STEPPER_RESET)
    {
        lutKIdx_ = addLUT("k", 15);
    }

    void setup()
    {
        BasicInstrument::setup();
        taskStepper_.start();
    }

protected:
    virtual void setVar(byte idx, float value) override { BasicInstrument::setVar(idx, value); }

    virtual int32_t posForLut(byte idxLut) override { return taskStepper_.position(); }
    virtual int32_t pos(byte idxPos) override { return taskStepper_.position(); }

    virtual void setPos(byte idx, int32_t value, bool absolute) override
    {
        BasicInstrument::setPos(idx, value, absolute);

        int32_t pos = (absolute ? value : taskStepper_.position() + value);
        taskStepper_.setPosition(pos);
    }

    virtual void setLPos(byte idx, float value, bool absolute) override
    {
        BasicInstrument::setLPos(idx, value, absolute);

        StoredLUT& lut = getLUT(lutKIdx_);
        int16_t p = (int16_t)(catmullSplineInterpolate<double, double>(lut.x(), lut.y(), lut.size(), value));
        taskStepper_.setPosition(p);
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
    TaskStepperX27Driver taskStepper_;

    byte lutKIdx_ = 0;
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
