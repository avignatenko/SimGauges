#include "TaskMenu.h"

#include <SimpleCLI.h>

void TaskMenu::loopMenuCallbackStatic()
{
    TaskMenu& me = TaskMenu::instance();
    me.loopMenuCallback();
}

void TaskMenu::loopMenuCallback()
{
    Log.traceln("TaskMenu::loopMenuCallback()");

    // Check if user typed something into the serial monitor
    if (Serial.available())
    {
        if (mode_ == INTERACTIVE)
        {
            char received = Serial.read();
            if (received == 'e')
            {
                mode_ = CLI;
                Serial.println("Exited from interactive mode");
                Serial.print("# ");
                return;
            }
            int16_t delta = 0;

            switch (received)
            {
            case 'q':
                delta = 1;
                break;
            case 'Q':
                delta = 10;
                break;
            case 'w':
                delta = -1;
                break;
            case 'W':
                delta = -10;
                break;
            }

            if (TaskMenu::instance().interactiveCallback_) TaskMenu::instance().interactiveCallback_(delta);
        }

        if (mode_ == CLI)
        {
            static String buffer;

            char received = Serial.read();
            if (received == '\n')
            {
                // Parse the user input into the CLI
                cli->parse(buffer);
                buffer = "";
                Serial.println();
                Serial.print("# ");
                return;
            }

            buffer += received;
            Serial.print(received);
        }
    }
}

TaskMenu* TaskMenu::instance_ = nullptr;

// Command cmdPos;

void TaskMenu::cmdPosCallback(cmd* c)
{
    Command cmd(c);  // Create wrapper object

    // Get arguments
    Argument numberArg = cmd.getArgument("value");

    // Get values
    int numberValue = numberArg.getValue().toInt();

    if (TaskMenu::instance().posCallback_) TaskMenu::instance().posCallback_(numberValue);
}

void TaskMenu::cmdLPosCallback(cmd* c)
{
    Command cmd(c);  // Create wrapper object

    // Get arguments
    Argument numberArg = cmd.getArgument("value");

    // Get values
    float numberValue = numberArg.getValue().toFloat();

    if (TaskMenu::instance().lposCallback_) TaskMenu::instance().lposCallback_(numberValue);
}

void TaskMenu::cmdInteractiveCallback(cmd* c)
{
    Serial.println("Switch to interactive mode: 'q' - CCW, 'w' - CW, 'e' - Exit");
    TaskMenu::instance().mode_ = INTERACTIVE;
}

void TaskMenu::cmdSimAddressCallback(cmd* c)
{
    Command cmd(c);  // Create wrapper object

    // Get arguments
    Argument numberArg = cmd.getArgument("value");

    // Get values
    float numberValue = numberArg.getValue().toFloat();

    uint16_t newAddr = 0;
    if (TaskMenu::instance().simAddressCallback_) newAddr = TaskMenu::instance().simAddressCallback_(numberValue);
    if (newAddr)
    {
        Serial.println();
        Serial.println(newAddr);
    }
}

void TaskMenu::cmdHelpCallback(cmd* c)
{
    Command cmd(c);  // Create wrapper object
    Serial.println("Help:");
    Serial.println(TaskMenu::instance().cli->toString());
}

// Callback in case of an error
void TaskMenu::errorCallback(cmd_error* e)
{
    CommandError cmdError(e);  // Create wrapper object

    Serial.print("ERROR: ");
    Serial.println(cmdError.toString());

    if (cmdError.hasCommand())
    {
        Serial.print("Did you mean \"");
        Serial.print(cmdError.getCommand().toString());
        Serial.println("\"?");
    }
}

TaskMenu::TaskMenu(Scheduler& sh) : task_(TASK_IMMEDIATE, TASK_FOREVER, &loopMenuCallbackStatic, &sh, false)
{
    Log.traceln("TaskMenu::TaskMenu()");

    cli = new SimpleCLI();

    cli->setErrorCallback(errorCallback);

    Command cmdPos = cli->addCommand("pos", cmdPosCallback);
    cmdPos.addPositionalArgument("value", "0");
    cmdPos.setDescription(" Move to given steps value");

    Command cmdHelp = cli->addCommand("help", cmdHelpCallback);
    cmdHelp.setDescription(" Get help!");

    Command cmdInteractive = cli->addCommand("int", cmdInteractiveCallback);
    cmdInteractive.setDescription(" Interactive mode");

    Command cmdLPos = cli->addCommand("lpos", cmdLPosCallback);
    cmdLPos.addPositionalArgument("value", "0.0");
    cmdLPos.setDescription(" Move to logical position");

    Command cmdSimAddr = cli->addCommand("addr", cmdSimAddressCallback);
    cmdSimAddr.addPositionalArgument("value", "0");
    cmdSimAddr.setDescription(" Get/Set this gauge CAN ID");

    Serial.println("Welcome to gauge terminal!");
    Serial.print("# ");
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
