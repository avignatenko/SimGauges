#pragma once

#include "Common.h"

class SwitecX25;

class TaskStepperX27
{
public:
    static void init(Scheduler& sh, byte p1, byte p2, byte p3, byte p4);
    static TaskStepperX27& instance();

    void start();

private:
    TaskStepperX27(Scheduler& sh, byte p1, byte p2, byte p3, byte p4);

    void loopStepperCallback();
    static void loopStepperCallbackStatic();

private:
    static TaskStepperX27* instance_;

    Task task_;

    SwitecX25* motor_;
};