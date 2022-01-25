
#include <Common.h>

#include <BasicInstrument.h>

#include <TaskButton.h>
#include <TaskCAN.h>
#include <TaskErrorLed.h>
#include "TaskSimManager.h"

#define USE_SIMESSAGE 1

// hardware speficics
const int BUTTON_PORT = 7;
const int LED_PORT = 4;
const int MCP2515_SPI_PORT = 10;
const int MCP2515_INT_PIN = 2;

// todo: read from flash?
const int kSimManagerChannel = 15;  // channel P

// todo: read from flash?
const uint16_t kSimAddress = 1;  // master 1

class InstrumentMaster : public CommonInstrument
{
public:
    InstrumentMaster(byte ledPin, byte buttonPin, byte canSPIPin, byte canIntPin, byte simManagerChannel,
                     byte simAddress)
        : CommonInstrument(ledPin, buttonPin, canSPIPin, canIntPin),
          taskSimManager_(&taskErrorLed_, taskManager_, simManagerChannel),
          simAddress_(simAddress)
    {
        taskSimManager_.setReceivedFromHostCallback(fastdelegate::MakeDelegate(this, &InstrumentMaster::onHostMessage));
        taskCAN_.setSimAddress(simAddress);
    }

    virtual void setup() override
    {
        CommonInstrument::setup();
#ifdef USE_SIMESSAGE
        taskSimManager_.start();
#endif
    }

protected:
    virtual void onButtonPressed(bool pressed) override
    {
        CommonInstrument::onButtonPressed(pressed);

        // debug code below!
        if (pressed)
        {
            static float pos = 0;
            pos += 10;
            taskCAN_.sendMessage(0, 0, 16, 4, (byte*)&pos);
        }
    }

    virtual void onCANReceived(byte priority, byte port, uint16_t srcAddress, uint16_t dstAddress, byte len,
                               byte* payload) override
    {
        taskSimManager_.sendToHost(port, srcAddress, len, payload);
    }

private:
    void onHostMessage(byte port, uint16_t toSimAddress, byte len, byte* payload)
    {
        // deal with our messages
        if (toSimAddress == simAddress_)
        {
            switch (port)
            {
            case 0:  // set log level
                if (len == 1)
                {
                    byte logLevel = *reinterpret_cast<byte*>(payload);
                    // Log.setLevel(logLevel);
                    // Log.infoln("Log level set to %d", logLevel);
                }
                else
                {
                    // Log.errorln("Wrong parameters for port 0");
                }
                break;
            case 1:  // set test error (1 or 0)
                if (len == 1)
                {
                    byte led = *reinterpret_cast<byte*>(payload);
                    if (led)
                        taskErrorLed_.addError(TaskErrorLed::ERROR_TEST_LED);
                    else
                        taskErrorLed_.removeAllErrors();
                }
                else
                {
                    // Log.errorln("Wrong parameters for port 1");
                }
                break;
                // default:
                // Log.errorln("Unknown port %d", port);
            }
            return;
        }

        static byte buffer[8];
        memcpy(buffer, payload, len);

        taskCAN_.sendMessage(0, port, toSimAddress, len, buffer);
    }

private:
    TaskSimManager taskSimManager_;
    const byte simAddress_;
};

InstrumentMaster s_instrument(LED_PORT, BUTTON_PORT, MCP2515_SPI_PORT, MCP2515_INT_PIN, kSimManagerChannel,
                              kSimAddress);

void setup()
{
    // skip serial without logging in favor of siminnovation interface
#ifndef USE_SIMESSAGE
    Serial.begin(9600);
#endif

    s_instrument.setup();
}

void loop()
{
    s_instrument.run();
}
