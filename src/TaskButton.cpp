#include "TaskButton.h"

#include "TaskErrorLed.h"

#include <Bounce2.h>

void TaskButton::loopButtonCallbackStatic()
{
    TaskButton& me = TaskButton::instance();
    me.loopButtonCallback();
}


void TaskButton::loopButtonCallback()
{
    button_->update();

    if (button_->pressed())
    {
        Log.verboseln("TaskButton::loopButtonCallback::sensorVal == LOW");
        TaskErrorLed::instance().addError(TaskErrorLed::ERROR_TEST_LED);
    }
    else if (button_->released())
    {
        Log.verboseln("TaskButton::loopButtonCallback::sensorVal == HIGH");
        TaskErrorLed::instance().removeError(TaskErrorLed::ERROR_TEST_LED);
    }
}

TaskButton* TaskButton::instance_ = nullptr;

TaskButton::TaskButton(Scheduler& sh, byte ledPort)
    : button_(new Bounce2::Button()),
      task_(0, -1, &loopButtonCallbackStatic, &sh, false)
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
