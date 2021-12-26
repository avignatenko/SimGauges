
#include <Common.h>

#include <BasicInstrument.h>
#include <TaskButton.h>
#include <TaskCAN.h>
#include <TaskErrorLed.h>
#include <TaskStepperTMC2208.h>

#include <EEPROM.h>
#include <InterpolationLib.h>
#include <StoredLUT.h>

#include <GyverFilters.h>

// hardware speficics
const byte BUTTON_PORT = 7;
const byte LED_PORT = 3;
const byte MCP2515_SPI_PORT = 10;
const byte MCP2515_INT_PIN = 2;

const int STEPPER_STEP = A0;
const int STEPPER_DIR = A1;
const int STEPPER_RESET = A2;

const int kEEPROMAddrIndex = 0;
const int kEEPROMCal0Index = kEEPROMAddrIndex + sizeof(uint16_t);
const int kEEPROMLUTIndex = 5;

//StoredLUT s_lut(10);

/*


void onPosCommand(int32_t pos)
{
    Serial.println(pos);
    TaskStepperTMC2208::instance().setPosition(pos);
}

void onLPosCommand(float pos)
{
    uint16_t hwPos = static_cast<uint16_t>(Interpolation::ConstrainedSpline(s_lut.x(), s_lut.y(), s_lut.size(), pos));
    Serial.print(hwPos);
    TaskStepperTMC2208::instance().setPosition(hwPos);
}

void onInteractiveCommand(int16_t delta)
{
    TaskStepperTMC2208& stp = TaskStepperTMC2208::instance();
    int32_t newPos = stp.position() + delta;
    stp.setPosition(newPos);
    Serial.println(newPos);
}
*/

class TaskCalibrate : public Task
{
public:
    TaskCalibrate(TaskStepperTMC2208& stepper, Task& taskKnob, int32_t calibration, unsigned long aInterval = 0,
                  long aIterations = 0, Scheduler* aScheduler = NULL, bool aEnable = false)
        : Task(aInterval, aIterations, aScheduler, aEnable),
          stepper_(stepper),
          taskKnob_(taskKnob),
          calibration_(calibration)
    {
    }

    virtual bool Callback() override { return (this->*callback_)(); }

    void setCalibration(int32_t calibration) { calibration_ = calibration; }

private:
    bool initSensor()
    {
        Serial.println("initSensor");
        pinMode(A3, INPUT);

        callback_ = &TaskCalibrate::sensorMoveAwayFromMarkerInit;

        return true;
    }

    float readSensorFiltered()
    {
        int res_raw = analogRead(A3);
        static GFilterRA filter(0.8);
        int res = filter.filtered(res_raw);

        return res * 5.0 / 1023.0;
    }

    bool sensorFinalize()
    {
        if (stepper_.position() == stepper_.targetPosition())
        {
            stepper_.resetPosition(0);
            disable();
            taskKnob_.enable();
        }

        return true;
    }

    bool sensorFinalizeInit()
    {
        Serial.println("sensorFinalizeInit");
        // 10863
        stepper_.setPosition(stepper_.position() + calibration_);;

        callback_ = &TaskCalibrate::sensorFinalize;

        return true;
    }

    bool sensorFindMarkerFast()
    {
        if (readSensorFiltered() > 2) callback_ = &TaskCalibrate::sensorFinalizeInit;

        return true;
    }

    bool sensorFindMarkerFastInit()
    {
        Serial.println("sensorFindMarkerFastInit");
        stepper_.setPosition(stepper_.position() - 200L * 10 * 10 * 16);  // full circle
        callback_ = &TaskCalibrate::sensorFindMarkerFast;

        return true;
    }

    bool sensorMoveAwayFromMarker()
    {
        if (readSensorFiltered() < 1) callback_ = &TaskCalibrate::sensorFindMarkerFastInit;

        return true;
    }

    bool sensorMoveAwayFromMarkerInit()
    {
        Serial.println("sensorMoveAwayFromMarkerInit");
        stepper_.setPosition(stepper_.position() - 200L * 10 * 10 * 16);  // full circle

        callback_ = &TaskCalibrate::sensorMoveAwayFromMarker;

        return true;
    }

private:
    TaskStepperTMC2208& stepper_;
    Task& taskKnob_;
    bool (TaskCalibrate::*callback_)() = &TaskCalibrate::initSensor;
    int32_t calibration_;
};

class TaskKnob : public Task
{
public:
    TaskKnob(TaskStepperTMC2208& stepper, unsigned long aInterval = 0, long aIterations = 0,
             Scheduler* aScheduler = NULL, bool aEnable = false)
        : Task(aInterval, aIterations, aScheduler, aEnable), stepper_(stepper)
    {
    }

    void setLUT(StoredLUT* lut) { lut_ = lut; }
    int knobValue()
    {
        pinMode(A4, INPUT);
        int resRaw = analogRead(A4);
        // exp filter
        static GFilterRA filter(0.01);
        int res = filter.filtered(resRaw);
        return res;
    }

    int pos()
    {
        // 31.0 -> 0
        // 28 -> 820

        // float p = (knobValue() - 0) * (28.0 - 31.0) / (820 - 0) + 31.0;

        float p = Interpolation::ConstrainedSpline(lut_->x(), lut_->y(), lut_->size(), knobValue());
       
        float p0 = 29.92;

        // https://en.wikipedia.org/wiki/Pressure_altitude
        float h = 145366.45 * (1 - pow(p / p0, 0.190284));

        int pos = h * 16 * 200 / 1000;
        //
        //Serial.println(pos);
        return pos;
    }

    void setStepperLink(bool link) { stepperLink_ = link; }

    virtual bool Callback() override
    {
        if (stepperLink_)
            stepper_.setPosition(pos());
        else
            knobValue();
        return true;
    }

private:
    TaskStepperTMC2208& stepper_;
    bool stepperLink_ = false;
    StoredLUT* lut_ = nullptr;
};

class Altimeter : public BasicInstrument
{
public:
    Altimeter(byte ledPin, byte buttonPin, byte canSPIPin, byte canIntPin)
        : BasicInstrument(ledPin, buttonPin, canSPIPin, canIntPin),
          taskStepper_(taskManager_, STEPPER_STEP, STEPPER_DIR, STEPPER_RESET, 100, 100),
          taskKnob_(taskStepper_, 1 * TASK_MILLISECOND, TASK_FOREVER, &taskManager_, false),
          taskCalibrate_(taskStepper_, taskKnob_, 0, TASK_IMMEDIATE, TASK_FOREVER, &taskManager_, false)
    {
        taskCAN_.setReceiveCallback(fastdelegate::MakeDelegate(this, &Altimeter::onSetValue));

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

        Serial.println("Altimeter::setup()");
        taskStepper_.start();
        taskCalibrate_.enable();

        /*
                TaskAltimeterMenu::init(taskManager_);
                TaskAltimeterMenu::instance().setPosCallback(onPosCommand);


                // port 0: set arrow position
                taskCAN_.setReceiveCallback(onSetValue, 0);*/
        /*
                // set some fake LUT
                bool hasLut = s_lut.load();

                if (!hasLut)
                {
                    s_lut.clear();

                    s_lut.addValue(0.0, 0.0);
                    s_lut.addValue(40, 380);
                    s_lut.addValue(60, 928);
                    s_lut.addValue(80, 1417);
                    s_lut.addValue(100, 1869);
                    s_lut.addValue(200, 3657);
                }

               */
    }

protected:
    virtual void onVarSet(int idx, float value) override
    {
        BasicInstrument::onVarSet(idx, value);
        if (idx == varCal0Idx_) taskCalibrate_.setCalibration(value);
        if (idx == varKnobOn_) taskKnob_.setStepperLink(value < 1 ? false : true);
    }

    virtual int32_t posForLut(int idx) override { return taskKnob_.knobValue(); }

    virtual void setPos(byte idx, int32_t value, bool absolute) override
    {
        int32_t pos = (absolute ? value : taskStepper_.position() + value);
        taskStepper_.setPosition(pos);
    }

private:
    void onSetValue(byte priority, byte port, uint16_t srcAddress, uint16_t dstAddress, byte len, byte* payload)
    {
        if (len != 4)
        {
            return;
        }

        float pos = *reinterpret_cast<float*>(payload);

        // TaskStepperTMC2208::instance().setPosition(
        //    static_cast<uint16_t>(Interpolation::ConstrainedSpline(s_lut.x(), s_lut.y(), s_lut.size(), pos)));
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
