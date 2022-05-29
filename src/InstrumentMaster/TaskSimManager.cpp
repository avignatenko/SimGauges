#include "TaskSimManager.h"
#include <TaskErrorLed.h>

#include <si_message_port.hpp>

#define DISABLE_LOGGING

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

}  // namespace

TaskSimManager* TaskSimManager::instance_ = nullptr;

TaskSimManager::TaskSimManager(TaskErrorLed* taskErrorLed, Scheduler& sh)
    : Task(TASK_IMMEDIATE, TASK_FOREVER, &sh, true), taskErrorLed_(taskErrorLed)
{
    instance_ = this;
}

void TaskSimManager::start(byte channel)
{
#ifdef USE_SIMESSAGE
    messagePort_ = new SiMessagePort(SI_MESSAGE_PORT_DEVICE_ARDUINO_NANO, static_cast<SiMessagePortChannel>(channel),
                                     simManagerCallbackStatic);
    enable();
#endif
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
        if (taskErrorLed_) taskErrorLed_->addError(TaskErrorLed::ERROR_HOST);
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
#ifdef USE_SIMESSAGE
    messagePort_->Tick();
#endif

    return true;
}

void TaskSimManager::sendToHost(byte port, uint16_t fromSimAddress, byte len, byte* payload)
{
#ifdef USE_SIMESSAGE
    SiMessagePortResult result = messagePort_->SendMessage(toSimManagerMessageId(port, fromSimAddress), payload, len);
    if (result != SI_MESSAGE_PORT_RESULT_OK && taskErrorLed_) taskErrorLed_->addError(TaskErrorLed::ERROR_HOST);
#endif
}

void TaskSimManager::sendDebugMessageToHost(const String& message)
{
#ifdef USE_SIMESSAGE
    messagePort_->DebugMessage(SI_MESSAGE_PORT_LOG_LEVEL_DEBUG, message);
#else
    Serial.println(message);
#endif
}

void TaskSimManager::setReceivedFromHostCallback(MessageCallback callback)
{
    callback_ = callback;
}
