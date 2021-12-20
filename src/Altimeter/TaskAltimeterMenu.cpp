#include "TaskAltimeterMenu.h"

void TaskAltimeterMenu::init(Scheduler& sh)
{
    instance_ = new TaskAltimeterMenu(sh);
}

TaskAltimeterMenu& TaskAltimeterMenu::instance()
{
    return (TaskAltimeterMenu&)*instance_;
}

TaskAltimeterMenu::TaskAltimeterMenu(Scheduler& sh) : TaskSinglePointerMenu(sh) {}