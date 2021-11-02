#pragma once

#include "Common.h"

namespace Bounce2
{
class Button;
}

class TaskButton
{
public:
    static void init(Scheduler& sh, byte btnPort);

    static TaskButton& instance();

    void start();

private:
    TaskButton(Scheduler& sh, byte btnPort);

    void loopButtonCallback();

    static void loopButtonCallbackStatic();

private:
    static TaskButton* instance_;

    Bounce2::Button* button_;

    Task task_;
};