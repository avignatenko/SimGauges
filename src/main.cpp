
#include "Common.h"

#include "TaskButton.h"
#include "TaskCAN.h"
#include "TaskErrorLed.h"
#include "TaskStepper.h"

#include <SPI.h>

Scheduler taskManager;

// hardware speficics
const int BUTTON_PORT = 7;
const int LED_PORT = 4;
const int MCP2515_SPI_PORT = 10;
const int MCP2515_INT_PIN = 2;

void initSerial()
{
    Serial.begin(115200);
    while (!Serial)
        ;
}

void setup()
{
    initSerial();

    Log.begin(LOG_LEVEL_NOTICE, &Serial);
    Log.infoln("Started. Serial OK");

    SPI.begin();

    TaskErrorLed::init(taskManager, LED_PORT);
    TaskStepper::init(taskManager, A2, A3, A1, A0);
    TaskButton::init(taskManager, BUTTON_PORT);
    TaskCAN::init(taskManager, MCP2515_SPI_PORT, MCP2515_INT_PIN);

    TaskErrorLed::instance().start();
    TaskStepper::instance().start();
    TaskButton::instance().start();
    TaskCAN::instance().start();
}

void loop()
{
    taskManager.execute();
}
