#include "TaskMenu.h"

#if 0
// Specific commands
int cmd_about(ArgumentsInterface* args)
{
    // Check if user called "help about" and displays a help message
    if (cmdLine.help(args, "about", "Displays a message") == CMD_OK)
    {
        return (CMD_OK);
    }
    // Execute the command.
    Serial.println("A sample for CLI");
    return (CMD_OK);
}

int cmd_millis(ArgumentsInterface* args)
{
    if (cmdLine.help(args, "millis", "Displays milliseconds since board began running this program.") == CMD_OK)
    {
        return (CMD_OK);
    }
    Serial.println(millis());
    return (CMD_OK);
}

#endif

void TaskMenu::loopMenuCallbackStatic()
{
    TaskMenu& me = TaskMenu::instance();
    me.loopMenuCallback();
}

void TaskMenu::loopMenuCallback()
{
    Log.traceln("TaskMenu::loopMenuCallback()");

    cmdLine_->runOnce();

#if 0
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

#endif
}

TaskMenu* TaskMenu::instance_ = nullptr;

// Command cmdPos;

int TaskMenu::cmdPosCallback(ArgumentsInterface* c)
{
    if (c->count() != 2) return CMD_ERROR;
    c->print(&Serial);

    int numberValue = c->toInteger(1);
    if (TaskMenu::instance().posCallback_) TaskMenu::instance().posCallback_(numberValue);
}

void TaskMenu::cmdLPosCallback(ArgumentsInterface* c)
{
#if 0
    Command cmd(c);  // Create wrapper object

    // Get arguments
    Argument numberArg = cmd.getArgument("value");

    // Get values
    float numberValue = numberArg.getValue().toFloat();

    if (TaskMenu::instance().lposCallback_) TaskMenu::instance().lposCallback_(numberValue);

#endif
}

void TaskMenu::cmdInteractiveCallback(ArgumentsInterface* c)
{
    Serial.println("Switch to interactive mode: 'q' - CCW, 'w' - CW, 'e' - Exit");
    TaskMenu::instance().mode_ = INTERACTIVE;
}

void TaskMenu::cmdSimAddressCallback(ArgumentsInterface* c)
{
#if 0
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

#endif
}

void TaskMenu::cmdHelpCallback(ArgumentsInterface* c)
{
#if 0
    Command cmd(c);  // Create wrapper object
    Serial.println("Help:");
    Serial.println(TaskMenu::instance().cli->toString());
#endif
}

// Callback in case of an error
void TaskMenu::errorCallback(ArgumentsInterface* e)
{
#if 0
    CommandError cmdError(e);  // Create wrapper object

    Serial.print("ERROR: ");
    Serial.println(cmdError.toString());

    if (cmdError.hasCommand())
    {
        Serial.print("Did you mean \"");
        Serial.print(cmdError.getCommand().toString());
        Serial.println("\"?");
    }
#endif
}

TaskMenu::TaskMenu(Scheduler& sh) : task_(TASK_IMMEDIATE, TASK_FOREVER, &loopMenuCallbackStatic, &sh, false)
{
    Log.traceln("TaskMenu::TaskMenu()");

    // All aliases for commands, must be NULL terminated
    static const char* a[] = {"p", NULL};

    // Commands structure array
    static command_t cmds[] = {{.name = "pos", .handler = &TaskMenu::cmdPosCallback, .aliases = a}};

    // Can also be done like this
    cmdLine_ = new CommandLineInterface((command_t*)cmds, sizeof(cmds) / sizeof(command_t));

#if 0
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

#endif
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
