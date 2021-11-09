
#include <Common.h>

#include <TaskButton.h>
#include "TaskCAN.h"
#include "TaskErrorLed.h"
#include "TaskStepperX27.h"

#include "mcp_can.h"

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

void onButtonPressed(bool pressed)
{
    if (pressed)
        TaskErrorLed::instance().addError(TaskErrorLed::ERROR_TEST_LED);
    else
        TaskErrorLed::instance().removeError(TaskErrorLed::ERROR_TEST_LED);
}

void onSetValue(byte len, byte* payload, void* data)
{
    Log.traceln("onSetValue");
    if (len != 2)
    {
        Log.errorln("onSetValue: wrong len");
        return;
    }

    uint16_t pos = *reinterpret_cast<uint16_t*>(payload);
    Log.verboseln("Value: %d", pos);
    TaskStepperX27::instance().setPosition(pos);
}

void setup()
{
    initSerial();

    Log.begin(LOGLEVEL, &Serial);
    Log.infoln("Started. Serial OK");

    TaskErrorLed::init(taskManager, LED_PORT);
    TaskStepperX27::init(taskManager, A2, A3, A1, A0);
    TaskButton::init(taskManager, BUTTON_PORT);
    TaskButton::instance().setPressedCallback(onButtonPressed);

    const uint16_t kSimAddress = 16;
    TaskCAN::init(taskManager, MCP2515_SPI_PORT, MCP2515_INT_PIN, kSimAddress);

    // port 0: set arrow position
    TaskCAN::instance().setReceiveCallback(onSetValue, 0);

    TaskErrorLed::instance().start();
    TaskStepperX27::instance().start();
    TaskButton::instance().start();
    TaskCAN::instance().start();
}

void loop()
{
    taskManager.execute();
}
