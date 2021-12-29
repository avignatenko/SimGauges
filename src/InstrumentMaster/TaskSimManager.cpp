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

TaskSimManager::TaskSimManager(TaskErrorLed& taskErrorLed, Scheduler& sh, byte channel)
    : Task(TASK_IMMEDIATE, TASK_FOREVER, &sh, false), taskErrorLed_(taskErrorLed)
{
    instance_ = this;
    // Log.traceln("TaskSimManager::TaskSimManager()");
    messagePort_ = new SiMessagePort(SI_MESSAGE_PORT_DEVICE_ARDUINO_NANO, static_cast<SiMessagePortChannel>(channel),
                                     simManagerCallbackStatic);
#ifndef DISABLE_LOGGING
    debugPrinter_ = new SimManagerDebugPrinter(*messagePort_);
#endif
}

void TaskSimManager::start()
{
    enable();
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
        taskErrorLed_.addError(TaskErrorLed::ERROR_HOST);
        return;
    }

    byte port = 0;
    uint16_t toSimAddress = 0;
    fromSimManagerMessageId(message_id, port, toSimAddress);

    if (callback_)
    {
        callback_(port, toSimAddress, payload->len, payload->data_byte);
#ifndef DISABLE_LOGGING
        messagePort_->DebugMessage(SI_MESSAGE_PORT_LOG_LEVEL_DEBUG, String("Address: ") + toSimAddress);
        messagePort_->DebugMessage(SI_MESSAGE_PORT_LOG_LEVEL_DEBUG, String("Port: ") + port);
#endif
    }
}

void TaskSimManager::simManagerCallbackStatic(uint16_t message_id, struct SiMessagePortPayload* payload)
{
    TaskSimManager& me = *instance_;
    me.simManagerCallback(message_id, payload);
}

bool TaskSimManager::Callback()
{
    messagePort_->Tick();
    return true;
}

void TaskSimManager::sendToHost(byte port, uint16_t fromSimAddress, byte len, byte* payload)
{
    SiMessagePortResult result = messagePort_->SendMessage(toSimManagerMessageId(port, fromSimAddress), payload, len);
    if (result != SI_MESSAGE_PORT_RESULT_OK) taskErrorLed_.addError(TaskErrorLed::ERROR_HOST);
}

void TaskSimManager::setReceivedFromHostCallback(MessageCallback callback)
{
    callback_ = callback;
}

Print* TaskSimManager::debugPrinter()
{
    return debugPrinter_;
}