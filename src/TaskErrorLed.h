#pragma once

#include "Common.h"

class TaskErrorLed
{
public:
    static void init(Scheduler& sh, byte ledPort);

    static TaskErrorLed& instance();

    enum Error
    {
        ERROR_OK = 0,
        ERROR_TEST_LED = (1 << 0),
        ERROR_CAN = (2 << 0),
    };

    void start();
    void addError(int error);
    void removeError(int error);
    int error();

private:
    TaskErrorLed(Scheduler& sh, byte ledPort);

    void loopBlinkLedCallback();

    void updateDelay();

    static void loopBlinkLedCallbackStatic();

    void led(bool on);

private:
    static TaskErrorLed* instance_;

    byte ledPort_;
    byte error_ = 0;
    bool ledOn_ = false;

    Task task_;
};