#include "TaskErrorLed.h"

namespace
{
int getHighestBit(int value)
{
    int r = 0;
    while (value >>= 1) r++;
    return r;
}
}  // namespace

void TaskErrorLed::led(bool on)
{
    digitalWrite(ledPort_, on ? HIGH : LOW);
    ledOn_ = on;
}

void TaskErrorLed::loopBlinkLedCallbackStatic()
{
    TaskErrorLed& me = TaskErrorLed::instance();
    me.loopBlinkLedCallback();
}

void TaskErrorLed::loopBlinkLedCallback()
{
    Log.traceln("TaskErrorLed::loopBlinkLedCallback()");
    // blink
    led(!ledOn_);
}

TaskErrorLed* TaskErrorLed::instance_ = nullptr;

TaskErrorLed::TaskErrorLed(Scheduler& sh, byte ledPort)
    : ledPort_(ledPort), task_(0 * TASK_MILLISECOND, -1, &loopBlinkLedCallbackStatic, &sh, false)
{
    Log.traceln("TaskErrorLed::TaskErrorLed()");

    pinMode(ledPort_, OUTPUT);
}

void TaskErrorLed::init(Scheduler& sh, byte ledPort)
{
    instance_ = new TaskErrorLed(sh, ledPort);
}

void TaskErrorLed::start()
{
    Log.traceln("TaskErrorLed::start()");

    // test 2sec on
    led(true);
    delay(2000);
    led(false);
}

TaskErrorLed& TaskErrorLed::instance()
{
    return *instance_;
}

void TaskErrorLed::addError(int error)
{
    int newError = error_ | error;
    if (error_ == newError) return;
    error_ = newError;
    updateDelay();
}

void TaskErrorLed::removeError(int error)
{
    int newError = error_ & ~error;
    if (error_ == newError) return;
    error_ = newError;
    updateDelay();
}

int TaskErrorLed::error()
{
    return error_;
}

void TaskErrorLed::updateDelay()
{
    if (error_ == ERROR_OK)
    {
        Log.verboseln("TaskErrorLed:updateDelay No error");
        led(false);
        task_.disable();
        return;
    }

    int highestBit = getHighestBit(error_);
    int delayMs = 1000 / (highestBit + 1);

    Log.verboseln("TaskErrorLed:updateDelay delays ms = %d", delayMs);

    task_.enableIfNot();
    task_.setInterval(delayMs * TASK_MILLISECOND);
}
