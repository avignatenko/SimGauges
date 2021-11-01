#pragma once

#include "Common.h"

class TaskErrorLed
{
public:
    static void init(Scheduler& sh, byte ledPort);
    static void shutdown();
    static TaskErrorLed& instance();

    enum Error
    {
        ERROR_OK = 0,
        ERROR_CAN = (1 << 0),
    };

    void start();
    void addError(int error);
    void removeError(int error);
    int error();

private:
    TaskErrorLed(Scheduler& sh, byte ledPort);

    void loopBlinkLedCallback();
    bool setupBlinkLedCallback();

    static void loopBlinkLedCallbackStatic();
    static bool setupBlinkLedCallbackStatic();

    void led(bool on);

private:
    static TaskErrorLed* instance_;

    byte ledPort_;
    byte error_ = 0;
    bool ledOn_ = false;

    Task task_;
};