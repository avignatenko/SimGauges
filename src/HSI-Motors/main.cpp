
#include <Common.h>

#include <BasicInstrument.h>
#include <Interpolation.h>

#include <TaskEncoder.h>
#include <TaskStepperX27Driver.h>

#include <Wire.h>
// hardware speficics

const byte STEPPER_A_STEP = A0;
const byte STEPPER_A_DIR = A1;

const byte STEPPER_B_STEP = A2;
const byte STEPPER_B_DIR = A3;

const byte STEPPER_RESET = 13;

class TaskI2CSlave : private Task
{
public:
    TaskI2CSlave(TaskStepperX27Driver& stepperCard, TaskStepperX27Driver& stepperBug, Scheduler* aScheduler = NULL)
        : Task(TASK_IMMEDIATE, TASK_ONCE, aScheduler, false), stepperCard_(stepperCard), stepperBug_(stepperBug)
    {
        instance_ = this;
    }

    virtual bool Callback() override {}

    void start() { enable(); }

    void sendEncoder(uint8_t encoder, int8_t direction, long millis)
    {
        Wire.beginTransmission(1);

        Wire.write((uint8_t)encoder);
        Wire.write((uint8_t)direction);
        Wire.write(reinterpret_cast<uint8_t*>(&millis), sizeof(long));

        Wire.endTransmission();
    }

protected:
    virtual bool OnEnable() override
    {
        Wire.begin(2);  // Activate I2C network
        Wire.onReceive(&TaskI2CSlave::receiveEvent);
        return true;
    }

    virtual void OnDisable() override {}

private:
    static void receiveEvent(int howMany)
    {
        if (Wire.available() < sizeof(byte) + sizeof(int16_t)) return;

        TaskI2CSlave* me = TaskI2CSlave::instance_;
        byte motor = Wire.read();
        int16_t pos = 0;
        Wire.readBytes(reinterpret_cast<uint8_t*>(&pos), sizeof(pos));

        if (motor == 0)
            me->stepperCard_.setPosition(pos);
        else if (motor == 1)
            me->stepperBug_.setPosition(pos);
    }

private:
    TaskStepperX27Driver& stepperCard_;
    TaskStepperX27Driver& stepperBug_;

    static TaskI2CSlave* instance_;
};

TaskI2CSlave* TaskI2CSlave::instance_ = nullptr;

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
    virtual bool OnEnable() override { return true; }

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
        if (stepperCard_.position() == stepperCard_.targetPosition())
        {
            stepperCard_.resetPosition(0);
            callback_ = &TaskCalibrate::initCalibrateBug;
        }

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
        if (stepperBug_.position() == stepperBug_.targetPosition())
        {
            stepperBug_.resetPosition(0);
            disable();
        }

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
        : taskStepperBug_(taskManager_, STEPPER_A_STEP, STEPPER_A_DIR, STEPPER_RESET, false),
          taskStepperCard_(taskManager_, STEPPER_B_STEP, STEPPER_B_DIR, STEPPER_RESET, false),
          taskEncoder1_(taskManager_, 8, 9),
          taskEncoder2_(taskManager_, 11, 12),
          taskCalibrate_(taskStepperCard_, taskStepperBug_, &taskManager_),
          taskI2C_(taskStepperCard_, taskStepperBug_, &taskManager_)
    {
        taskStepperBug_.setMaxSpeed(1000);
        taskStepperBug_.setMaxAcceleration(3000);

        taskStepperCard_.setMaxSpeed(1000);
        taskStepperCard_.setMaxAcceleration(3000);

        taskEncoder1_.setRotationCallback(fastdelegate::FastDelegate2<int8_t, long>(this, &HSIMotors::onEncoder2));
        taskEncoder2_.setRotationCallback(fastdelegate::FastDelegate2<int8_t, long>(this, &HSIMotors::onEncoder1));
    }

    void setup()
    {
        taskStepperBug_.start();
        taskStepperCard_.start();

        taskEncoder1_.start();
        taskEncoder2_.start();

        taskCalibrate_.start();

        taskI2C_.start();

        Serial.println("AA");
    }

private:
    void onEncoder1(int8_t dir, long millis)
    {
        taskI2C_.sendEncoder(0, dir, millis);
        // long accel = constrain(100 / millis, 1, 10);
        // taskStepperA_.setPosition(taskStepperA_.targetPosition() + dir * 12 * accel);
    }

    void onEncoder2(int8_t dir, long millis)
    {
        taskI2C_.sendEncoder(1, dir, millis);
        // long accel = constrain(100 / millis, 1, 10);
        // taskStepperA_.setPosition(taskStepperA_.targetPosition() - dir * 12 * accel);
        // taskStepperB_.setPosition(taskStepperB_.targetPosition() + dir * 12 * accel);
    }

private:
    TaskStepperX27Driver taskStepperBug_;
    TaskStepperX27Driver taskStepperCard_;
    TaskEncoder taskEncoder1_;
    TaskEncoder taskEncoder2_;
    TaskCalibrate taskCalibrate_;
    TaskI2CSlave taskI2C_;
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
