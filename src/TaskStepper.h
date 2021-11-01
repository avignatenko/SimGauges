#pragma once

#include "Common.h"

class SwitecX25;

class TaskStepper
{
public:
    static void init(Scheduler& sh, byte p1, byte p2, byte p3, byte p4);
    static void shutdown();
    static TaskStepper& instance();

    void start();

private:
    TaskStepper(Scheduler& sh, byte p1, byte p2, byte p3, byte p4);

    void loopStepperCallback();
    bool setupStepperCallback();

    static void loopStepperCallbackStatic();
    static bool setupStepperCallbackStatic();

private:
    static TaskStepper* instance_;

    Task task_;

    SwitecX25* motor_;
};