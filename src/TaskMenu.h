#pragma once

#include "Common.h"

#include <cli.hpp>

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
    using SimAddressCallback = uint16_t (*)(uint16_t);
    void setSimAddressCallback(SimAddressCallback callback);

private:
    TaskMenu(Scheduler& sh);

    void updateDelay();

    void loopMenuCallback();
    static void loopMenuCallbackStatic();

    void led(bool on);

    static int cmdPosCallback(ArgumentsInterface* c);
    static void cmdLPosCallback(ArgumentsInterface* c);
    static void cmdHelpCallback(ArgumentsInterface* c);
    static void cmdInteractiveCallback(ArgumentsInterface* c);
    static void cmdSimAddressCallback(ArgumentsInterface* c);
    static void errorCallback(ArgumentsInterface* e);

private:
    static TaskMenu* instance_;
    Task task_;

    enum Mode
    {
        CLI,
        INTERACTIVE
    };

    CommandLineInterface* cmdLine_ = nullptr;

    Mode mode_ = CLI;
    PosCallback posCallback_ = nullptr;
    LPosCallback lposCallback_ = nullptr;
    InteractiveCallback interactiveCallback_ = nullptr;
    SimAddressCallback simAddressCallback_ = nullptr;
};