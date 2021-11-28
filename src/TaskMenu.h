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

    using PosCallback = void (*)(uint16_t);
    void setPosCallback(PosCallback callback);
    using LPosCallback = void (*)(float);
    void setLPosCallback(LPosCallback callback);

    using InteractiveCallback = void (*)(uint16_t);
    void setInteractiveCallback(InteractiveCallback callback);

private:
    TaskMenu(Scheduler& sh);

    void updateDelay();

    void loopMenuCallback();
    static void loopMenuCallbackStatic();

    void led(bool on);

    static void cmdPosCallback(cmd* c);
    static void cmdLPosCallback(cmd* c);
    static void cmdHelpCallback(cmd* c);
    static void cmdInteractiveCallback(cmd* c);
    static void errorCallback(cmd_error* e);

private:
    static TaskMenu* instance_;
    Task task_;

    SimpleCLI* cli;

    enum Mode
    {
        CLI,
        INTERACTIVE
    };

    Mode mode_ = CLI;
    PosCallback posCallback_ = nullptr;
    LPosCallback lposCallback_ = nullptr;
    InteractiveCallback interactiveCallback_ = nullptr;
};