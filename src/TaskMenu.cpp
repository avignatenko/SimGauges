#include "TaskMenu.h"

#include <SerialCommands.h>

void TaskMenu::loopMenuCallbackStatic()
{
    TaskMenu& me = TaskMenu::instance();
    me.loopMenuCallback();
}

void TaskMenu::loopMenuCallback()
{
    int received = Serial.peek();
    if (received >= 0) Serial.print((char)received);

    cmdLine_->ReadSerial();
}

TaskMenu* TaskMenu::instance_ = nullptr;

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

void TaskMenu::cmdLUTCallback(SerialCommands* sender)
{
    if (!TaskMenu::instance().lutCallback_) return;

    LUTCommand cmd = LUTCommand::Invalid;
    float posl = -1.0;
    uint16_t pos = -1;

    const char* commandStr = sender->Next();
    if (!commandStr || strcmp(commandStr, "show") == 0) cmd = LUTCommand::Show;
    if (strcmp(commandStr, "save") == 0) cmd = LUTCommand::Save;
    if (strcmp(commandStr, "load") == 0) cmd = LUTCommand::Load;
    if (strcmp(commandStr, "clear") == 0) cmd = LUTCommand::Clear;
    if (strcmp(commandStr, "set") == 0)
    {
        cmd = LUTCommand::Set;
        const char* poslStr = sender->Next();
        if (!poslStr)
        {
            sender->GetSerial()->println("Error: set needs logical position");
            return;
        }

        posl = atof(poslStr);

        const char* posStr = sender->Next();
        if (posStr) pos = atoi(posStr);
    }
    if (strcmp(commandStr, "rm") == 0)
    {
        cmd = LUTCommand::Remove;
        const char* poslStr = sender->Next();
        if (!poslStr)
        {
            sender->GetSerial()->println("Error: set needs logical position");
            return;
        }

        posl = atof(poslStr);
    }
    if (cmd == LUTCommand::Invalid)
    {
        sender->GetSerial()->println("Error: unknown command");
        return;
    }

    TaskMenu::instance().lutCallback_(cmd, posl, pos);
}

void TaskMenu::cmdHelpCallback(SerialCommands* sender)
{
    static const __FlashStringHelper* a = F(R"=====( 
Help: 
 help - this help text
 pos <position> - move motor to given position
 q,Q,w,W - (single key commands) move motor needle (q,w - slow, Q,W - fast)
 lpos <position> - move motor to logical position
 addr <address> - change sim address for this device
 lut [show|load|save|clear|set|rm] [valuel] [valuep] - modify LUT table for the motor 
)=====");

    Stream* s = sender->GetSerial();
    s->println(a);
}

// Callback in case of an error
void TaskMenu::errorCallback(SerialCommands* sender, const char* command)
{
    sender->GetSerial()->print(F("Unrecognized command ["));
    sender->GetSerial()->print(command);
    sender->GetSerial()->println(F("]"));
}

TaskMenu::TaskMenu(Scheduler& sh) : task_(TASK_IMMEDIATE, TASK_FOREVER, &loopMenuCallbackStatic, &sh, false)
{
    static char s_serial_command_buffer[32];
    cmdLine_ = new SerialCommands(&Serial, s_serial_command_buffer, sizeof(s_serial_command_buffer), "\r\n", " ");

    static SerialCommand s_cmdHelp("help", &TaskMenu::cmdHelpCallback);
    static SerialCommand s_cmdPos("pos", &TaskMenu::cmdPosCallback);
    static SerialCommand s_cmdPosCCW("q", &TaskMenu::cmdCCWCallback, true);
    static SerialCommand s_cmdPosCCWFast("Q", &TaskMenu::cmdCCWFastCallback, true);
    static SerialCommand s_cmdPosCW("w", &TaskMenu::cmdCWCallback, true);
    static SerialCommand s_cmdPosCWFast("W", &TaskMenu::cmdCWFastCallback, true);
    static SerialCommand s_cmdLPos("lpos", &TaskMenu::cmdLPosCallback);
    static SerialCommand s_cmdAddr("addr", &TaskMenu::cmdSimAddressCallback);
    static SerialCommand s_cmdLUT("lut", &TaskMenu::cmdLUTCallback);

    cmdLine_->AddCommand(&s_cmdHelp);
    cmdLine_->AddCommand(&s_cmdPos);
    cmdLine_->AddCommand(&s_cmdPosCCW);
    cmdLine_->AddCommand(&s_cmdPosCCWFast);
    cmdLine_->AddCommand(&s_cmdPosCW);
    cmdLine_->AddCommand(&s_cmdPosCWFast);
    cmdLine_->AddCommand(&s_cmdLPos);
    cmdLine_->AddCommand(&s_cmdAddr);
    cmdLine_->AddCommand(&s_cmdLUT);

    cmdLine_->SetDefaultHandler(&TaskMenu::errorCallback);

    Serial.println("Welcome to gauge terminal! Type 'help' for list of commands.");
}

void TaskMenu::init(Scheduler& sh)
{
    instance_ = new TaskMenu(sh);
}

void TaskMenu::start()
{
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

void TaskMenu::setLUTCallback(SimAddressCallback callback)
{
    lutCallback_ = callback;
}

TaskMenu& TaskMenu::instance()
{
    return *instance_;
}
