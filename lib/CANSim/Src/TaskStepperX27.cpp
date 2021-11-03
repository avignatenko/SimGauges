#include "TaskStepperX27.h"

#include <SwitecX25.h>

void TaskStepperX27::loopStepperCallbackStatic()
{
    TaskStepperX27& me = TaskStepperX27::instance();
    me.loopStepperCallback();
}

void TaskStepperX27::loopStepperCallback()
{
    motor_->update();
}

TaskStepperX27* TaskStepperX27::instance_ = nullptr;

TaskStepperX27::TaskStepperX27(Scheduler& sh, byte p1, byte p2, byte p3, byte p4)
    : task_(1 * TASK_MILLISECOND, TASK_FOREVER, &loopStepperCallbackStatic, &sh, false)
{
    Log.traceln("TaskStepper::TaskStepper");

    // standard X25.168 range 315 degrees at 1/3 degree steps
    // FIXME: solder in proper way, to have A0, A1, A2, A3!
    // https://guy.carpenter.id.au/gaugette/2012/04/04/making-wiring-harnesses/
    motor_ = new SwitecX25(315 * 3, p1, p2, p3, p4);
}

void TaskStepperX27::init(Scheduler& sh, byte p1, byte p2, byte p3, byte p4)
{
    instance_ = new TaskStepperX27(sh, p1, p2, p3, p4);
}

void TaskStepperX27::start()
{
    // run the motor against the stops
    // motor_->zero();  // FIXME: need to impplement this async

    motor_->currentStep = motor_->steps - 1;
    for (unsigned int i = 0; i < motor_->steps; i++)
    {
        motor_->stepDown();
        delayMicroseconds(1000);
    }

    motor_->setPosition(motor_->steps / 2);

    task_.enable();
}
TaskStepperX27& TaskStepperX27::instance()
{
    return *instance_;
}
