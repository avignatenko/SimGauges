#pragma once

#include "Common.h"

class SimpleCLI;
struct cmd;
struct cmd_error;

class TaskMenu
{
public:
    static void init(Scheduler& sh);

    static TaskMenu& instance();

    void start();

private:
    TaskMenu(Scheduler& sh);

    void updateDelay();

    void loopMenuCallback();
    static void loopMenuCallbackStatic();

    void led(bool on);

    static void cmdPosCallback(cmd* c);
    static void cmdHelpCallback(cmd* c);
    static void errorCallback(cmd_error* e);

private:
    static TaskMenu* instance_;
    Task task_;

    SimpleCLI* cli;
};