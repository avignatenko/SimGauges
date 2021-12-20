#pragma once

#include <TaskSinglePointerMenu.h>
#include "Common.h"

class SerialCommands;

class TaskAltimeterMenu : public TaskSinglePointerMenu
{
public:
    static void init(Scheduler& sh);

    static TaskAltimeterMenu& instance();

private:
    TaskAltimeterMenu(Scheduler& sh);
};