#include "TaskMenu.h"

#include <SerialCommands.h>

void TaskMenu::loopMenuCallbackStatic()
{
    TaskMenu& me = TaskMenu::instance();
    me.loopMenuCallback();
}

void TaskMenu::loopMenuCallback()
{
    Log.traceln("TaskMenu::loopMenuCallback()");

    int received = Serial.peek();
    if (received >= 0) Serial.print((char)received);

    cmdLine_->ReadSerial();
}

TaskMenu* TaskMenu::instance_ = nullptr;

// Command cmdPos;

void TaskMenu::cmdPosCallback(SerialCommands* sender)
{
    const char* posStr = sender->Next();
    if (!posStr)
    {
        posStr = "0";
    }

    int pos = atoi(posStr);

    if (TaskMenu::instance().posCallback_) TaskMenu::instance().posCallback_(pos);
}

void TaskMenu::cmdCCWCallback(SerialCommands* sender)
{
    if (TaskMenu::instance().interactiveCallback_) TaskMenu::instance().interactiveCallback_(-1);
}
void TaskMenu::cmdCCWFastCallback(SerialCommands* sender)
{
    if (TaskMenu::instance().interactiveCallback_) TaskMenu::instance().interactiveCallback_(-10);
}
void TaskMenu::cmdCWCallback(SerialCommands* sender)
{
    if (TaskMenu::instance().interactiveCallback_) TaskMenu::instance().interactiveCallback_(1);
}
void TaskMenu::cmdCWFastCallback(SerialCommands* sender)
{
    if (TaskMenu::instance().interactiveCallback_) TaskMenu::instance().interactiveCallback_(10);
}

void TaskMenu::cmdLPosCallback(SerialCommands* sender)
{
    const char* posStr = sender->Next();
    if (!posStr)
    {
        posStr = "0.0";
    }

    float pos = atof(posStr);

    if (TaskMenu::instance().lposCallback_) TaskMenu::instance().lposCallback_(pos);
}

void TaskMenu::cmdSimAddressCallback(SerialCommands* sender)
{
    const char* addrStr = sender->Next();
    if (!addrStr) addrStr = "0";
    int addr = atoi(addrStr);

    uint16_t newAddr = 0;
    if (TaskMenu::instance().simAddressCallback_) newAddr = TaskMenu::instance().simAddressCallback_(addr);
    if (newAddr)
    {
        Serial.println();
        Serial.println(newAddr);
    }
}

void TaskMenu::cmdHelpCallback(SerialCommands* c)
{
#if 0
    Command cmd(c);  // Create wrapper object
    Serial.println("Help:");
    Serial.println(TaskMenu::instance().cli->toString());
#endif
}

// Callback in case of an error
void TaskMenu::errorCallback(SerialCommands* sender, const char* command)
{
    sender->GetSerial()->print("Unrecognized command [");
    sender->GetSerial()->print(command);
    sender->GetSerial()->println("]");
}

TaskMenu::TaskMenu(Scheduler& sh) : task_(TASK_IMMEDIATE, TASK_FOREVER, &loopMenuCallbackStatic, &sh, false)
{
    Log.traceln("TaskMenu::TaskMenu()");

    static char s_serial_command_buffer[32];
    cmdLine_ = new SerialCommands(&Serial, s_serial_command_buffer, sizeof(s_serial_command_buffer), "\r\n", " ");

    static SerialCommand s_cmdPos("pos", &TaskMenu::cmdPosCallback);
    static SerialCommand s_cmdPosCCW("q", &TaskMenu::cmdCCWCallback, true);
    static SerialCommand s_cmdPosCCWFast("Q", &TaskMenu::cmdCCWFastCallback, true);
    static SerialCommand s_cmdPosCW("w", &TaskMenu::cmdCWCallback, true);
    static SerialCommand s_cmdPosCWFast("W", &TaskMenu::cmdCWFastCallback, true);
    static SerialCommand s_cmdLPos("lpos", &TaskMenu::cmdLPosCallback);
    static SerialCommand s_cmdAddr("addr", &TaskMenu::cmdSimAddressCallback);

    cmdLine_->AddCommand(&s_cmdPos);
    cmdLine_->AddCommand(&s_cmdPosCCW);
    cmdLine_->AddCommand(&s_cmdPosCCWFast);
    cmdLine_->AddCommand(&s_cmdPosCW);
    cmdLine_->AddCommand(&s_cmdPosCWFast);
    cmdLine_->AddCommand(&s_cmdLPos);
    cmdLine_->AddCommand(&s_cmdAddr);

    cmdLine_->SetDefaultHandler(&TaskMenu::errorCallback);

    Serial.println("Welcome to gauge terminal!");

}

void TaskMenu::init(Scheduler& sh)
{
    instance_ = new TaskMenu(sh);
}

void TaskMenu::start()
{
    Log.traceln("TaskMenu::start()");

    task_.enable();
}

void TaskMenu::setPosCallback(PosCallback callback)
{
    posCallback_ = callback;
}

void TaskMenu::setLPosCallback(LPosCallback callback)
{
    lposCallback_ = callback;
}

void TaskMenu::setInteractiveCallback(InteractiveCallback callback)
{
    interactiveCallback_ = callback;
}

void TaskMenu::setSimAddressCallback(SimAddressCallback callback)
{
    simAddressCallback_ = callback;
}

TaskMenu& TaskMenu::instance()
{
    return *instance_;
}
