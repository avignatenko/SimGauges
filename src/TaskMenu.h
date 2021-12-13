#pragma once

#include "Common.h"

class SerialCommands;

class TaskMenu
{
public:
    static void init(Scheduler& sh);

    static TaskMenu& instance();

    void start();

    using PosCallback = void (*)(int16_t);
    void setPosCallback(PosCallback callback);
    using LPosCallback = void (*)(float);
    void setLPosCallback(LPosCallback callback);
    using InteractiveCallback = void (*)(int16_t);
    void setInteractiveCallback(InteractiveCallback callback);
    using SimAddressCallback = uint16_t (*)(uint16_t);
    void setSimAddressCallback(SimAddressCallback callback);

    enum class LUTCommand
    {
        Invalid,
        Show,
        Load,
        Save,
        Clear,
        Set,
        Remove
    };

    using LUTCallback = void (*)(LUTCommand, float, int16_t);
    void setLUTCallback(SimAddressCallback callback);

private:
    TaskMenu(Scheduler& sh);

    void updateDelay();

    void loopMenuCallback();
    static void loopMenuCallbackStatic();

    void led(bool on);

    static void cmdPosCallback(SerialCommands* sender);
    static void errorCallback(SerialCommands* sender, const char* command);

    static void cmdCCWCallback(SerialCommands* sender);
    static void cmdCCWFastCallback(SerialCommands* sender);
    static void cmdCWCallback(SerialCommands* sender);
    static void cmdCWFastCallback(SerialCommands* sender);

    static void cmdLPosCallback(SerialCommands* sender);
    static void cmdHelpCallback(SerialCommands* sender);
    static void cmdSimAddressCallback(SerialCommands* sender);

    static void cmdLUTCallback(SerialCommands* sender);

private:
    static TaskMenu* instance_;
    Task task_;

    enum Mode
    {
        CLI,
        INTERACTIVE
    };

    SerialCommands* cmdLine_ = nullptr;

    Mode mode_ = CLI;
    PosCallback posCallback_ = nullptr;
    LPosCallback lposCallback_ = nullptr;
    InteractiveCallback interactiveCallback_ = nullptr;
    SimAddressCallback simAddressCallback_ = nullptr;
    LUTCallback lutCallback_ = nullptr;
};