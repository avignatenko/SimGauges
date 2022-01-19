
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

const byte STEPPER_A_STEP = A0;
const byte STEPPER_A_DIR = A1;

const byte STEPPER_B_STEP = A3;
const byte STEPPER_B_DIR = A4;

const byte STEPPER_RESET = A2;

class GenericTwoNeedelsInstrument : public BasicInstrument
{
public:
    GenericTwoNeedelsInstrument(byte ledPin, byte buttonPin, byte canSPIPin, byte canIntPin)
        : BasicInstrument(ledPin, buttonPin, canSPIPin, canIntPin),
          taskStepperA_(taskManager_, STEPPER_A_STEP, STEPPER_A_DIR, STEPPER_RESET),
          taskStepperB_(taskManager_, STEPPER_B_STEP, STEPPER_B_DIR, STEPPER_RESET)
    {
        lutAIdx_ = addLUT("a", 15);
        lutBIdx_ = addLUT("b", 15);

        posAIdx_ = addPos("a");
        posBIdx_ = addPos("b");
    }

    void setup()
    {
        BasicInstrument::setup();
        taskStepperA_.start();
        taskStepperB_.start();
    }

protected:
    virtual void setVar(byte idx, float value) override { BasicInstrument::setVar(idx, value); }

    virtual int32_t posForLut(byte idxLut) override { return idxToStepperLut(idxLut).position(); }
    virtual int32_t pos(byte idxPos) override { return idxToStepper(idxPos).position(); }

    virtual void setPos(byte idx, int32_t value, bool absolute) override
    {
        BasicInstrument::setPos(idx, value, absolute);

        int32_t pos = (absolute ? value : idxToStepper(idx).position() + value);
        idxToStepper(idx).setPosition(pos);
    }

    virtual void setLPos(byte idx, float value, bool absolute) override
    {
        BasicInstrument::setLPos(idx, value, absolute);

        StoredLUT& lut = getLUT(idx == 0 ? lutAIdx_ : lutBIdx_);
        int16_t p = (int16_t)(cubicInterpolate<double, double>(lut.x(), lut.y(), lut.size(), value));
        idxToStepper(idx).setPosition(p);
    }

private:
    virtual void onCANReceived(byte priority, byte port, uint16_t srcAddress, uint16_t dstAddress, byte len,
                               byte* payload) override
    {
        if (len != 4) return;

        float pos = *reinterpret_cast<float*>(payload);
        setLPos(port, pos, true);
    }

    TaskStepperX27Driver& idxToStepper(byte idx) { return (idx == posAIdx_ ? taskStepperA_ : taskStepperB_); }
    TaskStepperX27Driver& idxToStepperLut(byte idx) { return (idx == lutAIdx_ ? taskStepperA_ : taskStepperB_); }

private:
    TaskStepperX27Driver taskStepperA_;
    TaskStepperX27Driver taskStepperB_;

    byte lutAIdx_ = 0;
    byte lutBIdx_ = 0;

    byte posAIdx_ = 0;
    byte posBIdx_ = 0;
};

GenericTwoNeedelsInstrument s_instrument(LED_PORT, BUTTON_PORT, MCP2515_SPI_PORT, MCP2515_INT_PIN);

void setup()
{
    Serial.begin(9600);
    s_instrument.setup();
}

void loop()
{
    s_instrument.run();
}
