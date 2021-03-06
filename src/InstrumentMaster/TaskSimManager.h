#pragma once

#include "Common.h"
#include "FastDelegate.h"

class SiMessagePort;
class TaskErrorLed;

#define USE_SIMESSAGE 1

class TaskSimManager : private Task
{
public:
    TaskSimManager(TaskErrorLed* taskErrorLed, Scheduler& sh);

    void start(byte channel);

    void sendToHost(byte port, uint16_t fromSimAddress, byte len, byte* payload);
    void sendDebugMessageToHost(const String& message);

    using MessageCallback = fastdelegate::FastDelegate4<byte, uint16_t, byte, byte*>;
    void setReceivedFromHostCallback(MessageCallback callback);

protected:
    virtual bool Callback() override;

private:
    void simManagerCallback(uint16_t message_id, struct SiMessagePortPayload* payload);
    static void simManagerCallbackStatic(uint16_t message_id, struct SiMessagePortPayload* payload);

private:
    static TaskSimManager* instance_;

    TaskErrorLed* taskErrorLed_;
    SiMessagePort* messagePort_;
    MessageCallback callback_;
};