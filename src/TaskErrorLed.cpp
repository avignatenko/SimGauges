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

bool TaskErrorLed::setupBlinkLedCallbackStatic()
{
    TaskErrorLed& me = TaskErrorLed::instance();
    return me.setupBlinkLedCallback();
}

void TaskErrorLed::loopBlinkLedCallbackStatic()
{
    TaskErrorLed& me = TaskErrorLed::instance();
    me.loopBlinkLedCallback();
}

bool TaskErrorLed::setupBlinkLedCallback()
{
    debugPrint("setup");

    pinMode(ledPort_, OUTPUT);

    // test 2sec on
    led(true);
    delay(2000);
    led(false);

    return true;
}

void TaskErrorLed::loopBlinkLedCallback()
{
    if (error_ == ERROR_OK)
    {
        led(false);
        task_.disable();
        return;
    }

    // blink
    led(!ledOn_);

    int highestBit = getHighestBit(error_);
    int delayMs = 1000 / (highestBit + 1);

    delay(delayMs);
}

TaskErrorLed* TaskErrorLed::instance_ = nullptr;

TaskErrorLed::TaskErrorLed(Scheduler& sh, byte ledPort)
    : ledPort_(ledPort), task_(0, -1, &loopBlinkLedCallbackStatic, &sh, false, &setupBlinkLedCallbackStatic)
{
    debugPrintln("TaskErrorLed constructor");
}

void TaskErrorLed::init(Scheduler& sh, byte ledPort)
{
    instance_ = new TaskErrorLed(sh, ledPort);
}

void TaskErrorLed::shutdown()
{
    delete instance_;
}

void TaskErrorLed::start()
{
    task_.enable();
}
TaskErrorLed& TaskErrorLed::instance()
{
    return *instance_;
}

void TaskErrorLed::addError(int error)
{
    error_ |= error;
    TaskErrorLed& me = TaskErrorLed::instance();
    me.task_.delay(0);
}

void TaskErrorLed::removeError(int error)
{
    error_ &= ~error;
    TaskErrorLed& me = TaskErrorLed::instance();
    me.task_.delay(0);
}

int TaskErrorLed::error()
{
    return error_;
}
