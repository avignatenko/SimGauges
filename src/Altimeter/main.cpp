
#include <Common.h>

#include "TaskCalibrate.h"
#include "TaskKnob.h"

#include <BasicInstrument.h>
#include <TaskButton.h>
#include <TaskCAN.h>
#include <TaskErrorLed.h>
#include <TaskStepperTMC2208.h>

// hardware speficics
const byte BUTTON_PORT = 7;
const byte LED_PORT = 3;
const byte MCP2515_SPI_PORT = 10;
const byte MCP2515_INT_PIN = 2;

const byte STEPPER_STEP = A0;
const byte STEPPER_DIR = A1;
const byte STEPPER_RESET = A2;

class Altimeter : public BasicInstrument
{
public:
    Altimeter(byte ledPin, byte buttonPin, byte canSPIPin, byte canIntPin)
        : BasicInstrument(ledPin, buttonPin, canSPIPin, canIntPin),
          taskStepper_(taskManager_, STEPPER_STEP, STEPPER_DIR, STEPPER_RESET, 100, 100),
          taskKnob_(taskStepper_, 1 * TASK_MILLISECOND, TASK_FOREVER, &taskManager_, false),
          taskCalibrate_(taskStepper_, taskKnob_.task(), 0, TASK_IMMEDIATE, TASK_FOREVER, &taskManager_, false)
    {
        varCal0Idx_ = addVar("cal0");
        varKnobOn_ = addVar("knob");

        taskCalibrate_.setCalibration(getVar(varCal0Idx_));
        taskKnob_.setStepperLink(getVar(varKnobOn_));

        lutKIdx_ = addLUT("k", 10);

        taskKnob_.setLUT(&getLUT(lutKIdx_));
    }

    void setup()
    {
        BasicInstrument::setup();
        taskStepper_.start();
        taskCalibrate_.start();
    }

protected:
    virtual void setVar(byte idx, float value) override
    {
        BasicInstrument::setVar(idx, value);

        if (idx == varCal0Idx_) taskCalibrate_.setCalibration(value);
        if (idx == varKnobOn_) taskKnob_.setStepperLink(value < 1 ? false : true);
    }

    virtual int32_t posForLut(int idx) override { return taskKnob_.knobValue(); }

    virtual void setPos(byte idx, int32_t value, bool absolute) override
    {
        BasicInstrument::setPos(idx, value, absolute);

        int32_t pos = (absolute ? value : taskStepper_.position() + value);
        taskStepper_.setPosition(pos);
    }

private:
    virtual void onCANReceived(byte priority, byte port, uint16_t srcAddress, uint16_t dstAddress, byte len,
                               byte* payload) override
    {
        if (len != 4) return;

        float pos = *reinterpret_cast<float*>(payload);

        taskKnob_.setPressure(pos);
    }

private:
    TaskStepperTMC2208 taskStepper_;
    TaskKnob taskKnob_;
    TaskCalibrate taskCalibrate_;

    byte varCal0Idx_ = 0;
    byte varKnobOn_ = 0;
    byte lutKIdx_ = 0;
};

Altimeter s_instrument(LED_PORT, BUTTON_PORT, MCP2515_SPI_PORT, MCP2515_INT_PIN);

void setup()
{
    Serial.begin(9600);
    s_instrument.setup();
}

void loop()
{
    s_instrument.run();
}
