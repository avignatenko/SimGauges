
#include <Common.h>

#include <TaskButton.h>
#include <TaskCAN.h>
#include <TaskErrorLed.h>
#include "TaskSimManager.h"

Scheduler taskManager;

// hardware speficics
const int BUTTON_PORT = 7;
const int LED_PORT = 4;
const int MCP2515_SPI_PORT = 10;
const int MCP2515_INT_PIN = 2;

// todo: read from flash?
const int kSimManagerChannel = 15;  // channel P

// todo: read from flash?
const uint16_t kSimAddress = 1;  // master 1

void initSerial()
{
    Serial.begin(115200);
    while (!Serial)
        ;
}

void onButtonPressed(bool pressed)
{
    if (pressed)
    {
        TaskErrorLed::instance().addError(TaskErrorLed::ERROR_TEST_LED);
        static float pos = 0;
        pos += 10;
        TaskCAN::instance().sendMessage(0, 0, 16, 4, (byte*)&pos);
    }
    else
    {
        TaskErrorLed::instance().removeAllErrors();
    }
}

void onCANMessage(byte priority, byte port, uint16_t srcAddress, uint16_t dstAddress, byte len, byte* payload,
                  void* data)
{
    TaskSimManager::instance().sendToHost(port, srcAddress, len, payload);
}

void onHostMessage(byte port, uint16_t toSimAddress, byte len, byte* payload, void* data)
{
    // deal with our messages
    if (toSimAddress == kSimAddress)
    {
        switch (port)
        {
        case 0:  // set log level
            if (len == 1)
            {
                byte logLevel = *reinterpret_cast<byte*>(payload);
                //Log.setLevel(logLevel);
                //Log.infoln("Log level set to %d", logLevel);
            }
            else
            {
                //Log.errorln("Wrong parameters for port 0");
            }
            break;
        case 1:  // set test error (1 or 0)
            if (len == 1)
            {
                byte led = *reinterpret_cast<byte*>(payload);
                if (led)
                    TaskErrorLed::instance().addError(TaskErrorLed::ERROR_TEST_LED);
                else
                    TaskErrorLed::instance().removeAllErrors();
            }
            else
            {
                //Log.errorln("Wrong parameters for port 1");
            }
            break;
        //default:
            //Log.errorln("Unknown port %d", port);
        }
        return;
    }

    static byte buffer[8];
    memcpy(buffer, payload, len);

    TaskCAN::instance().sendMessage(0, port, toSimAddress, len, buffer);
}

#define USE_SIMESSAGE 1

void setup()
{
    // skip serial without logging in favor of siminnovation interface
#ifndef USE_SIMESSAGE
    initSerial();
    Log.begin(LOGLEVEL, &Serial);
    Log.infoln("Started. Serial OK");
#endif

    TaskErrorLed::init(taskManager, LED_PORT);
    TaskButton::init(taskManager, BUTTON_PORT);
    TaskButton::instance().setPressedCallback(onButtonPressed);

#ifdef USE_SIMESSAGE
    TaskSimManager::instance().init(taskManager, kSimManagerChannel);
    TaskSimManager::instance().setReceivedFromHostCallback(onHostMessage);
#endif

    TaskCAN::init(taskManager, MCP2515_SPI_PORT, MCP2515_INT_PIN, kSimAddress);
    TaskCAN::instance().setReceiveCallback(onCANMessage);

    TaskErrorLed::instance().start();
    TaskButton::instance().start();
    TaskCAN::instance().start();

#ifdef USE_SIMESSAGE
    TaskSimManager::instance().start();

    //Log.begin(LOGLEVEL, TaskSimManager::instance().debugPrinter());
    //Log.infoln("Started. Serial OK");
#endif
}

void loop()
{
    taskManager.execute();
}
