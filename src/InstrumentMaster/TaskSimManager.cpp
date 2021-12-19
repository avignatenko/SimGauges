#include "TaskSimManager.h"
#include <TaskErrorLed.h>

#include <si_message_port.hpp>

namespace
{
uint16_t toSimManagerMessageId(byte port, uint16_t fromSimAddress)
{
    return (fromSimAddress & 0b1111111111) | ((port & 0b11111) << 10);
}

void fromSimManagerMessageId(uint16_t messageId, byte& out_port, uint16_t& out_toSimAddress)
{
    out_toSimAddress = messageId & 0b1111111111;
    out_port = (messageId >> 10) & 0b11111;
}

class SimManagerDebugPrinter : public Print
{
public:
    SimManagerDebugPrinter(SiMessagePort& messagePort) : messagePort_(messagePort) {}

    virtual size_t write(uint8_t ch) override { return 1; }
    virtual size_t write(const uint8_t* buffer, size_t size) override
    {
        messagePort_.DebugMessage(SI_MESSAGE_PORT_LOG_LEVEL_INFO, String((const char*)buffer));
        // messagePort_.DebugMessage(SI_MESSAGE_PORT_LOG_LEVEL_INFO, "TEST");
        return size;
    }

private:
    SiMessagePort& messagePort_;
};
}  // namespace

TaskSimManager* TaskSimManager::instance_ = nullptr;

TaskSimManager::TaskSimManager(Scheduler& sh, byte channel)
    : task_(TASK_IMMEDIATE, TASK_FOREVER, &loopSimManagerCallbackStatic, &sh, false)
{
    //Log.traceln("TaskSimManager::TaskSimManager()");
    messagePort_ = new SiMessagePort(SI_MESSAGE_PORT_DEVICE_ARDUINO_NANO, static_cast<SiMessagePortChannel>(channel),
                                     simManagerCallbackStatic);
#ifndef DISABLE_LOGGING
    debugPrinter_ = new SimManagerDebugPrinter(*messagePort_);
#endif
}

void TaskSimManager::init(Scheduler& sh, byte channel)
{
    instance_ = new TaskSimManager(sh, channel);
}

void TaskSimManager::start()
{
    //Log.traceln("TaskSimManager::start()");
    task_.enable();
}

void TaskSimManager::simManagerCallback(uint16_t message_id, struct SiMessagePortPayload* payload)
{
#ifndef DISABLE_LOGGING
    messagePort_->DebugMessage(SI_MESSAGE_PORT_LOG_LEVEL_DEBUG, String("Message id: ") + message_id);
    messagePort_->DebugMessage(SI_MESSAGE_PORT_LOG_LEVEL_DEBUG, String("Payload type ") + payload->type);
    messagePort_->DebugMessage(SI_MESSAGE_PORT_LOG_LEVEL_DEBUG, String("Payload len: ") + payload->len);
#endif

    if (payload->type != SI_MESSAGE_PORT_DATA_TYPE_BYTE || payload->len > 8)
    {
        TaskErrorLed::instance().addError(TaskErrorLed::ERROR_HOST);
        return;
    }

    byte port = 0;
    uint16_t toSimAddress = 0;
    fromSimManagerMessageId(message_id, port, toSimAddress);

    if (callback_.callback)
    {
        callback_.callback(port, toSimAddress, payload->len, payload->data_byte, callback_.data);
#ifndef DISABLE_LOGGING
        messagePort_->DebugMessage(SI_MESSAGE_PORT_LOG_LEVEL_DEBUG, String("Address: ") + toSimAddress);
        messagePort_->DebugMessage(SI_MESSAGE_PORT_LOG_LEVEL_DEBUG, String("Port: ") + port);
#endif
    }
}

void TaskSimManager::simManagerCallbackStatic(uint16_t message_id, struct SiMessagePortPayload* payload)
{
    TaskSimManager& me = TaskSimManager::instance();
    me.simManagerCallback(message_id, payload);
}

void TaskSimManager::loopSimManagerCallback()
{
    messagePort_->Tick();
}

void TaskSimManager::loopSimManagerCallbackStatic()
{
    TaskSimManager& me = TaskSimManager::instance();
    me.loopSimManagerCallback();
}

TaskSimManager& TaskSimManager::instance()
{
    return *instance_;
}

void TaskSimManager::sendToHost(byte port, uint16_t fromSimAddress, byte len, byte* payload)
{
    SiMessagePortResult result = messagePort_->SendMessage(toSimManagerMessageId(port, fromSimAddress), payload, len);
    if (result != SI_MESSAGE_PORT_RESULT_OK) TaskErrorLed::instance().addError(TaskErrorLed::ERROR_HOST);
}

void TaskSimManager::setReceivedFromHostCallback(MessageCallback callback, void* data)
{
    callback_.callback = callback;
    callback_.data = data;
}

Print* TaskSimManager::debugPrinter()
{
    return debugPrinter_;
}