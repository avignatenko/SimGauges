
#include "Common.h"

#include <mcp_can.h>

#include "TaskButton.h"
#include "TaskErrorLed.h"
#include "TaskStepper.h"

Scheduler taskManager;

// hardware speficics
const int BUTTON_PORT = 7;
const int LED_PORT = 4;
const int MCP2515_SPI_PORT = 10;
const int MCP2515_INT_PIN = 2;

// struct can_frame canMsg;
MCP_CAN mcpCAN(MCP2515_SPI_PORT);

int loopCAN()
{
    for (int i = 0; i < 5; ++i)
    {
        if (CAN_OK == mcpCAN.begin(CAN_500KBPS))  // init can bus : baudrate = 500k
        {
            Serial.println("CAN BUS Shield init ok!");
            TaskErrorLed::instance().removeError(TaskErrorLed::ERROR_CAN);
            break;
        }
        else
        {
            Serial.println("CAN BUS Shield init fail");
            Serial.println("Init CAN BUS Shield again");
            TaskErrorLed::instance().addError(TaskErrorLed::ERROR_CAN);
            delay(500);
            continue;
        }
    }

    for (;;)
    {
        if (CAN_MSGAVAIL == mcpCAN.checkReceive())
        {
            static unsigned char len = 0;
            static unsigned char buf[8];
            // check if data coming
            mcpCAN.readMsgBuf(&len, buf);  // read data,  len: data length, buf: data buf
            for (int i = 0; i < len; i++)  // print the data
            {
                Serial.print(buf[i]);
                Serial.print("\t");
            }
            Serial.println();
        }
        yield();
    }

    return 0;
}

int loopCANCheck()
{
    for (;;)
    {
        delay(1000);
        int error = mcpCAN.checkError();
        if (error != 0 && !(TaskErrorLed::instance().error() & TaskErrorLed::ERROR_CAN))
        {
            Serial.print("CAN error: ");
            Serial.println(error);
            TaskErrorLed::instance().addError(TaskErrorLed::ERROR_CAN);
        }
    }
}

// CoopTask<> taskErrorLed("errorLed", loopErrorLed);
// CoopTask<> taskCAN("can", loopCAN);
// CoopTask<> taskCANCheck("canCheck", loopCANCheck);
// CoopTask<> taskStepper("stepper", loopStepper);

void initSerial()
{
    Serial.begin(115200);
    while (!Serial)
        ;
}

void setup()
{
    initSerial();

    Log.begin(LOG_LEVEL_VERBOSE, &Serial);
    Log.noticeln("Started. Serial OK");

    SPI.begin();

    TaskErrorLed::init(taskManager, LED_PORT);
    TaskStepper::init(taskManager, A2, A3, A1, A0);
    TaskButton::init(taskManager, BUTTON_PORT);

    TaskErrorLed::instance().start();
    TaskStepper::instance().start();
    TaskButton::instance().start();
    // taskCAN.scheduleTask();
    // taskCANCheck.scheduleTask();
}

void loop()
{
    taskManager.execute();
}
