#pragma once

#include "Common.h"

class SiMessagePort;

class TaskSimManager
{
public:
    static void init(Scheduler& sh, byte channel);

    static TaskSimManager& instance();
    void start();

    void sendToHost(byte port, uint16_t fromSimAddress, byte len, byte* payload);

    using MessageCallback = void (*)(byte, uint16_t, byte, byte*, void*);
    void setReceivedFromHostCallback(MessageCallback callback, void* data = nullptr);

    Print* debugPrinter();

private:
    TaskSimManager(Scheduler& sh, byte channel);

    void loopSimManagerCallback();
    static void loopSimManagerCallbackStatic();

    void simManagerCallback(uint16_t message_id, struct SiMessagePortPayload* payload);
    static void simManagerCallbackStatic(uint16_t message_id, struct SiMessagePortPayload* payload);

private:
    static TaskSimManager* instance_;
    Task task_;
    SiMessagePort* messagePort_;

    struct CallBack
    {
        MessageCallback callback = nullptr;
        void* data = nullptr;
    };

    CallBack callback_;
    Print* debugPrinter_ = nullptr;
};