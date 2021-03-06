
#include <Common.h>

#include <BasicInstrument.h>
#include <Interpolation.h>

#include <TaskEncoder.h>
#include <TaskStepperTMC2208.h>

#include <Wire.h>
// hardware speficics

const byte STEPPER_A_STEP = A0;
const byte STEPPER_A_DIR = A1;

const byte STEPPER_B_STEP = A2;
const byte STEPPER_B_DIR = A3;

const byte STEPPER_RESET = 13;

struct MotorUpdate
{
    uint8_t calibration;  // 0 - not calibrated, 1 - in progress, 2 - calibrated
    int16_t posPitch;
    int16_t posRoll;
};

class TaskI2CSlave : private Task
{
public:
    TaskI2CSlave(Scheduler* aScheduler = NULL) : Task(TASK_IMMEDIATE, TASK_ONCE, aScheduler, false)
    {
        instance_ = this;
    }

    virtual bool Callback() override { return false; }

    void start() { enable(); }

    void sendUpdate(const MotorUpdate& update)
    {
        Wire.beginTransmission(1);
        Wire.write(reinterpret_cast<const uint8_t*>(&update), sizeof(MotorUpdate));

        Wire.endTransmission();
    }

    void sendStatusUpdate() {}

    void setReceiveCallback(fastdelegate::FastDelegate2<int8_t, int16_t> callback) { receiveCallback_ = callback; }

protected:
    virtual bool OnEnable() override
    {
        Wire.begin(2);  // Activate I2C network
        Wire.onReceive(&TaskI2CSlave::receiveEvent);
        Wire.onRequest(&TaskI2CSlave::onRequest);
        return true;
    }

    virtual void OnDisable() override {}

private:
    static void onRequest() {}

    static void receiveEvent(int howMany)
    {
        if (Wire.available() < sizeof(byte) + sizeof(int16_t)) return;

        TaskI2CSlave* me = TaskI2CSlave::instance_;
        byte motor = Wire.read();
        int16_t pos = 0;
        Wire.readBytes(reinterpret_cast<uint8_t*>(&pos), sizeof(pos));

        if (me->receiveCallback_) me->receiveCallback_(motor, pos);
    }

private:
    fastdelegate::FastDelegate2<int8_t, int16_t> receiveCallback_;

    static TaskI2CSlave* instance_;
};

TaskI2CSlave* TaskI2CSlave::instance_ = nullptr;

class TaskCalibratePitch : private Task
{
public:
    TaskCalibratePitch(TaskStepperTMC2208& stepperCard, Scheduler* aScheduler = NULL, bool aEnable = false)
        : Task(TASK_IMMEDIATE, TASK_FOREVER, aScheduler, aEnable), stepperCard_(stepperCard)
    {
    }

    virtual bool Callback() override { return (this->*callback_)(); }

    void start() { enable(); }

    int status() { return status_; }

    void setFinishPosition(int16_t position) { finishPos_ = position; }

protected:
    virtual bool OnEnable() override { return true; }

    virtual void OnDisable() override {}

private:
    bool initResetToBlack()
    {
        Serial.println("initResetToBlack");
        stepperCard_.resetPosition(0);
        stepperCard_.setPosition(2000);

        callback_ = &TaskCalibratePitch::resetToBlack;
        return true;
    }

    bool resetToBlack()
    {
        status_ = 1;
        if (!stepperCard_.isRunning())
        {
            stepperCard_.resetPosition(0);
            stepperCard_.setPosition(finishPos_);
            status_ = 2;
            disable();
        }

        return true;
    }

private:
    bool (TaskCalibratePitch::*callback_)() = &TaskCalibratePitch::initResetToBlack;
    TaskStepperTMC2208& stepperCard_;
    int status_ = 0;
    int16_t finishPos_ = 0;
};

class TaskCalibrate : private Task
{
public:
    TaskCalibrate(TaskStepperTMC2208& stepperBug, Scheduler* aScheduler = NULL, bool aEnable = false)
        : Task(TASK_IMMEDIATE, TASK_FOREVER, aScheduler, aEnable), stepperRoll_(stepperBug)
    {
    }

    virtual bool Callback() override { return (this->*callback_)(); }

    void start() { enable(); }

    int status() { return status_; }

    void setFinishPosition(int16_t position) { finishPos_ = position; }

protected:
    virtual bool OnEnable() override { return true; }

    virtual void OnDisable() override {}

private:
    static constexpr float kStepAngle = 0.9;
    static constexpr int kMicroSteps = 16;
    static constexpr float kGearRatio = 18.0 / 10.0;

    static int32_t angleToSteps(float angle)
    {
        return static_cast<int32_t>(angle * kMicroSteps / kStepAngle * kGearRatio);
    }
    static float stepsToAngle(int16_t steps)
    {
        return static_cast<float>(steps) / kMicroSteps * kStepAngle / kGearRatio;
    }

    bool initResetToBlack()
    {
        Serial.println("initResetToBlack");
        stepperRoll_.resetPosition(0);
        stepperRoll_.setPosition(angleToSteps(400));

        callback_ = &TaskCalibrate::resetToBlack;
        return true;
    }

    bool resetToBlack()
    {
        status_ = 1;
        // Serial.println("resetToWhite");
        int calRead = analogRead(A6);
        // Serial.println(calRead);
        if (calRead > 400)
        {
            stepperRoll_.stop();
            callback_ = &TaskCalibrate::initCalibrateRollFast;
        }

        return true;
    }

    bool initCalibrateRollFast()
    {
        Serial.println("initCalibrateRollFast");
        stepperRoll_.resetPosition(0);
        stepperRoll_.setPosition(angleToSteps(-400));

        callback_ = &TaskCalibrate::calibrateRollFast;

        return true;
    }

    bool calibrateRollFast()
    {
        // Serial.println("calibrateCard");
        int calRead = analogRead(A6);
        // Serial.println(calRead);
        if (calRead < 80)
        {
            Serial.println("calibrateRollFast found pos");
            Serial.println(stepperRoll_.position());

            callback_ = &TaskCalibrate::calibrateTransitionInit;
        }
        return true;
    }

    bool calibrateTransitionInit()
    {
        Serial.println("calibrateTransitionInit");
        stepperRoll_.setPosition(stepperRoll_.position() + angleToSteps(20));
        callback_ = &TaskCalibrate::calibrateTransition;
    }

    bool calibrateTransition()
    {
        if (!stepperRoll_.isRunning())
        {
            callback_ = &TaskCalibrate::initCalibrateRollSlow;
        }
        return true;
    }

    bool initCalibrateRollSlow()
    {
        savedSpeed_ = stepperRoll_.speed();
        stepperRoll_.setSpeed(200);

        Serial.println("initCalibrateRollSlow");
        stepperRoll_.resetPosition(0);
        stepperRoll_.setPosition(angleToSteps(-400));

        callback_ = &TaskCalibrate::calibrateRollSlow;

        return true;
    }

    bool calibrateRollSlow()
    {
        // Serial.println("calibrateCard");
        int calRead = analogRead(A6);
        // Serial.println(calRead);
        if (calRead < 80)
        {
            Serial.println("calibrateRollSlow found pos");
            Serial.println(stepperRoll_.position());
            stepperRoll_.resetPosition(0);
            stepperRoll_.setPosition(finishPos_);
            callback_ = &TaskCalibrate::finishCalibrateRoll;
        }
        return true;
    }

    bool finishCalibrateRoll()
    {
        Serial.println("finishCalibrateRoll");
        stepperRoll_.setSpeed(savedSpeed_);

        if (!stepperRoll_.isRunning())
        {
            status_ = 2;
            disable();
        }

        return true;
    }

private:
    bool (TaskCalibrate::*callback_)() = &TaskCalibrate::initResetToBlack;
    TaskStepperTMC2208& stepperRoll_;
    int status_ = 0;
    int16_t finishPos_ = 0;
    float savedSpeed_ = 0.0f;
};

class ATMotors : public InstrumentBase
{
public:
    ATMotors()
        : taskStepperRoll_(taskManager_, STEPPER_A_STEP, STEPPER_A_DIR, STEPPER_RESET),
          taskStepperCard_(taskManager_, STEPPER_B_STEP, STEPPER_B_DIR, STEPPER_RESET),
          taskCalibrate_(taskStepperRoll_, &taskManager_),
          taskCalibratePitch_(taskStepperCard_, &taskManager_),
          taskI2C_(&taskManager_)
    {
        taskI2C_.setReceiveCallback(fastdelegate::FastDelegate2<int8_t, int16_t>(this, &ATMotors::onReceive));

        taskStepperRoll_.setSpeed(4000);
        taskStepperRoll_.setAcceleration(6000);

        taskStepperCard_.setSpeed(3000);
        taskStepperCard_.setAcceleration(5000);
    }

    void setup()
    {
        taskStepperRoll_.start();
        taskStepperCard_.start();

        taskCalibrate_.start();
        taskCalibratePitch_.start();

        taskI2C_.start();
    }

private:
    void onReceive(int8_t motor, int16_t pos)
    {
        if (taskCalibrate_.status() < 2)
        {
            if (motor == 1)
                taskCalibrate_.setFinishPosition(pos);
            else if (motor == 0)
                taskCalibratePitch_.setFinishPosition(pos);
            return;
        }

        if (motor == 0)
            taskStepperCard_.setPosition(pos);
        else if (motor == 1)
            taskStepperRoll_.setPosition(pos);
    }

private:
    TaskStepperTMC2208 taskStepperRoll_;
    TaskStepperTMC2208 taskStepperCard_;
    TaskCalibrate taskCalibrate_;
    TaskCalibratePitch taskCalibratePitch_;
    TaskI2CSlave taskI2C_;
};

ATMotors s_instrument;

void setup()
{
    Serial.begin(9600);
    s_instrument.setup();
}

void loop()
{
    s_instrument.run();
}
