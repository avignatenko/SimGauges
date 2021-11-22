#include "TaskMenu.h"

#include <SimpleCLI.h>
#include <TaskStepperX27.h>

void TaskMenu::loopMenuCallbackStatic()
{
    TaskMenu& me = TaskMenu::instance();
    me.loopMenuCallback();
}

void TaskMenu::loopMenuCallback()
{
    Log.traceln("TaskMenu::loopBlinkLedCallback()");

    // Check if user typed something into the serial monitor
    if (Serial.available())
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

TaskMenu* TaskMenu::instance_ = nullptr;

// Command cmdPos;

void TaskMenu::cmdPosCallback(cmd* c)
{
    Command cmd(c);  // Create wrapper object

    // Get arguments
    Argument numberArg = cmd.getArgument("value");

    // Get values
    int numberValue = numberArg.getValue().toInt();

    TaskStepperX27::instance().setPosition(numberValue);
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
    cmdPos.addPositionalArgument("value", 0);

    Command cmdHelp = cli->addCommand("help", cmdHelpCallback);
    cmdHelp.setDescription(" Get help!");

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

TaskMenu& TaskMenu::instance()
{
    return *instance_;
}
