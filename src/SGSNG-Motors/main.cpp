
#include <Common.h>

#include <BasicInstrument.h>
#include <Interpolation.h>

#include <TaskEncoder.h>
#include <TaskStepperX27Driver.h>

#include <Wire.h>
// hardware speficics

const byte STEPPER_A_STEP = A0;
const byte STEPPER_A_DIR = A1;

class TaskI2CSlave : private Task
{
public:
    TaskI2CSlave(Scheduler* aScheduler = NULL) : Task(TASK_IMMEDIATE, TASK_ONCE, aScheduler, false)
    {
        instance_ = this;
    }

    virtual bool Callback() override {}

    void start() { enable(); }

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

class SGSNGMotors : public InstrumentBase
{
public:
    SGSNGMotors()
        : taskStepper_(taskManager_, STEPPER_A_STEP, STEPPER_A_DIR, 0xFF),
          taskI2C_(&taskManager_)
    {
        taskI2C_.setReceiveCallback(fastdelegate::FastDelegate2<int8_t, int16_t>(this, &SGSNGMotors::onReceive));
    }

    void setup()
    {
        taskStepper_.start();
        taskI2C_.start();
    }

private:
   
    void onReceive(int8_t motor, int16_t pos)
    {
       taskStepper_.setPosition(pos);
    }

private:
    TaskStepperX27Driver taskStepper_;
    TaskI2CSlave taskI2C_;
};

SGSNGMotors s_instrument;

void setup()
{
    Serial.begin(9600);
    s_instrument.setup();
}

void loop()
{
    s_instrument.run();
}
