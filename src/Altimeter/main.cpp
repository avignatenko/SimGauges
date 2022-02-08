
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
          taskStepper_(taskManager_, STEPPER_STEP, STEPPER_DIR, STEPPER_RESET),
          taskKnob_(20 * TASK_MILLISECOND, TASK_FOREVER, &taskManager_, false),
          taskCalibrate_(taskStepper_, 0, TASK_IMMEDIATE, TASK_FOREVER, &taskManager_, false)
    {
        varCal0Idx_ = addVar("cal0");
        taskCalibrate_.setCalibration(getVar(varCal0Idx_));

        lutKIdx_ = addLUT("k", 10);
        taskKnob_.setLUT(&getLUT(lutKIdx_));
        taskKnob_.setPressureCallback(fastdelegate::MakeDelegate(this, &Altimeter::onPressureKnobChanged));
    }

    void setup()
    {
        BasicInstrument::setup();
        taskStepper_.start();
        taskCalibrate_.start();
        taskKnob_.start();
    }

protected:
    virtual void setVar(byte idx, float value) override
    {
        BasicInstrument::setVar(idx, value);

        if (idx == varCal0Idx_) taskCalibrate_.setCalibration(value);
    }

    virtual int32_t posForLut(byte idx) override { return taskKnob_.knobValue(); }
    virtual int32_t pos(byte idx) override { return taskKnob_.knobValue(); }

    virtual void setPos(byte idx, int32_t value, bool absolute) override
    {
        BasicInstrument::setPos(idx, value, absolute);

        int32_t pos = (absolute ? value : taskStepper_.position() + value);
        taskStepper_.setPosition(pos);
    }

private:
    void onPressureKnobChanged(float pressure)
    {
        Serial.print("pr: ");
        Serial.println(pressure);
        taskCAN_.sendMessage(0, 0, 0, sizeof(float), reinterpret_cast<byte*>(&pressure));
    }

    virtual void onCANReceived(byte priority, byte port, uint16_t srcAddress, uint16_t dstAddress, byte len,
                               byte* payload) override
    {
        if (len != 4) return;

        float h = *reinterpret_cast<float*>(payload);

        if (port == 0 && !taskCalibrate_.isEnabled())  // set altimeter
        {
            int32_t pos = h * 16 * 200 / 1000;
            taskStepper_.setPosition(pos);
        }
    }

private:
    TaskStepperTMC2208 taskStepper_;
    TaskKnob taskKnob_;
    TaskCalibrate taskCalibrate_;

    byte varCal0Idx_ = 0;
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
