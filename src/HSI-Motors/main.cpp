
#include <Common.h>

#include <BasicInstrument.h>
#include <Interpolation.h>

#include <TaskEncoder.h>
#include <TaskStepperX27Driver.h>

// hardware speficics

const byte STEPPER_A_STEP = A0;
const byte STEPPER_A_DIR = A1;

const byte STEPPER_B_STEP = A2;
const byte STEPPER_B_DIR = A3;

const byte STEPPER_RESET = 13;

class TaskCalibrate : private Task
{
public:
    TaskCalibrate(TaskStepperX27Driver& stepperCard, TaskStepperX27Driver& stepperBug, Scheduler* aScheduler = NULL,
                  bool aEnable = false)
        : Task(TASK_IMMEDIATE, TASK_FOREVER, aScheduler, aEnable), stepperCard_(stepperCard), stepperBug_(stepperBug)
    {
    }

    virtual bool Callback() override { return (this->*callback_)(); }

    void start() { enable(); }

protected:
    virtual bool OnEnable() override
    {
        pinMode(A6, INPUT);
        // int value = analogRead(A7);
        // Serial.println(value);

        // int value2 = analogRead(A6);
        // Serial.println(value2);
        return true;
    }

    virtual void OnDisable() override {}

private:
    bool initResetToWhite()
    {
        Serial.println("initResetToWhite");
        stepperCard_.resetPosition(0);
        stepperCard_.setPosition(360 * 12 * 1.2);

        stepperBug_.resetPosition(0);
        stepperBug_.setPosition(360 * 12 * 1.2);

        callback_ = &TaskCalibrate::resetToWhite;
        return true;
    }

    bool resetToWhite()
    {
        // Serial.println("resetToWhite");
        int calRead = analogRead(A6);
        // Serial.println(calRead);
        if (calRead < 100)
        {
            stepperCard_.stop();
            stepperBug_.stop();
            callback_ = &TaskCalibrate::initCalibrateCard;
        }

        return true;
    }

    bool initCalibrateCard()
    {
        Serial.println("initCalibrateCard");
        stepperCard_.resetPosition(0);
        stepperCard_.setPosition(360 * 12 * 1.2);

        callback_ = &TaskCalibrate::calibrateCard;

        return true;
    }
    bool calibrateCard()
    {
        // Serial.println("calibrateCard");
        int calRead = analogRead(A6);
        // Serial.println(calRead);
        if (calRead > 200)
        {
            Serial.println("calibrateCard found pos, moving to +60 degrees");
            Serial.println(stepperCard_.position());
            stepperCard_.setPosition(stepperCard_.position() + 60 * 12);
            callback_ = &TaskCalibrate::finishCalibrateCard;
        }
        return true;
    }

    bool finishCalibrateCard()
    {
        if (stepperCard_.position() == stepperCard_.targetPosition()) callback_ = &TaskCalibrate::initCalibrateBug;
        return true;
    }

    bool initCalibrateBug()
    {
        Serial.println("initCalibrateBug");
        stepperBug_.resetPosition(0);
        stepperBug_.setPosition(360 * 12 * 1.2);

        callback_ = &TaskCalibrate::calibrateBug;

        return true;
    }

    bool calibrateBug()
    {
        // Serial.println("calibrateBug");
        int calRead = analogRead(A6);
        // Serial.println(calRead);
        if (calRead > 200)
        {
            Serial.println("calibrateBug found pos");
            Serial.println(stepperBug_.position());
            stepperBug_.stop();
            callback_ = &TaskCalibrate::finishCalibrateBug;
        }
        return true;
    }

    bool finishCalibrateBug()
    {
        disable();
        return true;
    }

private:
    bool (TaskCalibrate::*callback_)() = &TaskCalibrate::initResetToWhite;
    TaskStepperX27Driver& stepperCard_;
    TaskStepperX27Driver& stepperBug_;
};

class HSIMotors : public InstrumentBase
{
public:
    HSIMotors()
        : taskStepperA_(taskManager_, STEPPER_A_STEP, STEPPER_A_DIR, STEPPER_RESET, false),
          taskStepperB_(taskManager_, STEPPER_B_STEP, STEPPER_B_DIR, STEPPER_RESET, false),
          taskEncoder1_(taskManager_, 8, 9),
          taskEncoder2_(taskManager_, 11, 12),
          taskCalibrate_(taskStepperB_, taskStepperA_, &taskManager_)
    {
        taskStepperA_.setMaxSpeed(1000);
        taskStepperA_.setMaxAcceleration(3000);

        taskStepperB_.setMaxSpeed(1000);
        taskStepperB_.setMaxAcceleration(3000);

        taskEncoder1_.setRotationCallback(fastdelegate::FastDelegate2<int8_t, long>(this, &HSIMotors::onEncoder2));
        taskEncoder2_.setRotationCallback(fastdelegate::FastDelegate2<int8_t, long>(this, &HSIMotors::onEncoder1));
    }

    void setup()
    {
        taskStepperA_.start();
        taskStepperB_.start();

        // taskStepperA_.setPosition(50000);
        // taskStepperB_.setPosition(50000);

        taskEncoder1_.start();
        taskEncoder2_.start();

        taskCalibrate_.start();

        Serial.println("AA");
    }

private:
    void onEncoder1(int8_t dir, long millis)
    {
        long accel = constrain(100 / millis, 1, 10);
        taskStepperA_.setPosition(taskStepperA_.targetPosition() + dir * 12 * accel);
    }

    void onEncoder2(int8_t dir, long millis)
    {
        long accel = constrain(100 / millis, 1, 10);
        taskStepperB_.setPosition(taskStepperB_.targetPosition() + dir * 12 * accel);
    }

private:
    TaskStepperX27Driver taskStepperA_;
    TaskStepperX27Driver taskStepperB_;
    TaskEncoder taskEncoder1_;
    TaskEncoder taskEncoder2_;
    TaskCalibrate taskCalibrate_;
};

HSIMotors s_instrument;

void setup()
{
    Serial.begin(9600);
    s_instrument.setup();
}

void loop()
{
    s_instrument.run();
}
