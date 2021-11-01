#include "TaskStepper.h"

#include <SwitecX25.h>

namespace
{
}  // namespace

bool TaskStepper::setupStepperCallbackStatic()
{
    TaskStepper& me = TaskStepper::instance();
    return me.setupStepperCallback();
}

void TaskStepper::loopStepperCallbackStatic()
{
    TaskStepper& me = TaskStepper::instance();
    me.loopStepperCallback();
}

bool TaskStepper::setupStepperCallback()
{
    debugPrint("setup");

    // run the motor against the stops
    // motor_->zero();  // FIXME: need to impplement this async

    motor_->currentStep = motor_->steps - 1;
    for (unsigned int i = 0; i < motor_->steps; i++)
    {
        motor_->stepDown();
        delayMicroseconds(1000);
    }

    motor_->setPosition(motor_->steps / 2);

    return true;
}

void TaskStepper::loopStepperCallback()
{
    motor_->update();
}

TaskStepper* TaskStepper::instance_ = nullptr;

TaskStepper::TaskStepper(Scheduler& sh, byte p1, byte p2, byte p3, byte p4)
    : task_(1 * TASK_MILLISECOND, -1, &loopStepperCallbackStatic, &sh, false, &setupStepperCallbackStatic)
{
    debugPrintln("TaskStepper constructor");

    // standard X25.168 range 315 degrees at 1/3 degree steps
#define STEPS (315 * 3)
    // FIXME: solder in proper way, to have A0, A1, A2, A3!
    // https://guy.carpenter.id.au/gaugette/2012/04/04/making-wiring-harnesses/
    motor_ = new SwitecX25(STEPS, p1, p2, p3, p4);
}

void TaskStepper::init(Scheduler& sh, byte p1, byte p2, byte p3, byte p4)
{
    instance_ = new TaskStepper(sh, p1, p2, p3, p4);
}

void TaskStepper::shutdown()
{
    delete instance_;
}

void TaskStepper::start()
{
    task_.enable();
}
TaskStepper& TaskStepper::instance()
{
    return *instance_;
}
