#include "TaskButton.h"

#include <Bounce2.h>

void TaskButton::loopButtonCallbackStatic()
{
    TaskButton& me = TaskButton::instance();
    me.loopButtonCallback();
}

void TaskButton::loopButtonCallback()
{
    if (!callback_) return;

    button_->update();

    if (button_->pressed())
    {
        Log.verboseln("TaskButton::loopButtonCallback::sensorVal == LOW");
        callback_(true);
       
    }
    else if (button_->released())
    {
        Log.verboseln("TaskButton::loopButtonCallback::sensorVal == HIGH");
        callback_(false);
    }
}

TaskButton* TaskButton::instance_ = nullptr;

TaskButton::TaskButton(Scheduler& sh, byte ledPort)
    : button_(new Bounce2::Button()), task_(TASK_IMMEDIATE, TASK_FOREVER, &loopButtonCallbackStatic, &sh, false)
{
    Log.traceln("TaskButton::TaskButton()");

    button_->attach(ledPort, INPUT_PULLUP);
    button_->interval(20);
    button_->setPressedState(LOW);
}

void TaskButton::init(Scheduler& sh, byte ledPort)
{
    instance_ = new TaskButton(sh, ledPort);
}

void TaskButton::start()
{
    task_.enable();
}
TaskButton& TaskButton::instance()
{
    return *instance_;
}
