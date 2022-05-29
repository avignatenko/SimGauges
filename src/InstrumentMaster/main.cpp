
#include <Common.h>

#include <BasicInstrument.h>

#include <TaskButton.h>
#include <TaskCAN.h>
#include <TaskErrorLed.h>
#include "TaskSimManager.h"

#include <EEPROM.h>
#include <map>

// hardware speficics
const int BUTTON_PORT = 7;
const int LED_PORT = 4;
const int MCP2515_SPI_PORT = 10;
const int MCP2515_INT_PIN = 2;

#define WRITE_EEPROM 0

#if WRITE_EEPROM
const int kSimManagerChannel = 15;  // channel P
const uint16_t kSimAddress = 1;     // master 1
#endif

const int8_t kSimAddressEEPROMAddress = 0;
const int8_t kSimManagerChannelEEPROMAddress = 2;

class StatsTask : public Task
{
public:
    StatsTask(Scheduler& scheduler, TaskSimManager& taskSimManager)
        : Task(5 * TASK_SECOND, TASK_FOREVER, &scheduler), taskSimManager_(taskSimManager)
    {
    }

    void addToStats(byte priority, byte port, uint16_t dstSimAddress, byte len, byte* payload)
    {
        stats_[{port, dstSimAddress}]++;
    }

private:
    bool Callback() override
    {
        taskSimManager_.sendDebugMessageToHost("Stats!");
        for (std::map<Msg, long>::iterator it = stats_.begin(); it != stats_.end(); ++it)
        {
            String msg;
            msg += it->first.address;
            msg += " ";
            msg += it->first.port;
            msg += " ";
            msg += it->second;

            taskSimManager_.sendDebugMessageToHost(msg);
        }
        return true;
    }

private:
    struct Msg
    {
        uint8_t port;
        uint16_t address;

        bool operator<(const Msg& rhs) const { return address < rhs.address || port < rhs.port; }
    };
    std::map<Msg, long> stats_;
    TaskSimManager& taskSimManager_;
};

class InstrumentMaster : public CommonInstrument
{
public:
    InstrumentMaster(byte ledPin, byte buttonPin, byte canSPIPin, byte canIntPin)
        : CommonInstrument(ledPin, buttonPin, canSPIPin, canIntPin),
          taskSimManager_(&taskErrorLed_, taskManager_),
          stats_(taskManager_, taskSimManager_)
    {
#if WRITE_EEPROM
        EEPROM.put(kSimAddressEEPROMAddress, kSimAddress);
        EEPROM.put(kSimManagerChannelEEPROMAddress, kSimManagerChannel);
#endif

        EEPROM.get(kSimAddressEEPROMAddress, simAddress_);

        taskSimManager_.setReceivedFromHostCallback(fastdelegate::MakeDelegate(this, &InstrumentMaster::onHostMessage));
        taskCAN_.setSimAddress(simAddress_);
        taskCAN_.setReceiveUnknown(true);
        taskCAN_.setErrorCAllback(fastdelegate::MakeDelegate(this, &InstrumentMaster::onCANError));
    }

    virtual void setup() override
    {
        CommonInstrument::setup();

        uint8_t channel = 0;
        EEPROM.get(kSimManagerChannelEEPROMAddress, channel);

#ifndef USE_SIMESSAGE
        Serial.println("Sim channel:");
        Serial.println(channel);
        Serial.println("CANSim addr:");
        Serial.println(simAddress_);

#endif
        taskSimManager_.start(channel);
    }

protected:
    virtual void onCANReceived(byte priority, byte port, uint16_t srcAddress, uint16_t dstAddress, byte len,
                               byte* payload) override
    {
        taskSimManager_.sendToHost(port, srcAddress, len, payload);
    }

private:
    void onCANError(byte error) { taskSimManager_.sendDebugMessageToHost(String(F("CAN Error: ")) + error); }
    void onHostMessage(byte port, uint16_t toSimAddress, byte len, byte* payload)
    {
        // deal with our messages
        if (toSimAddress == simAddress_)
        {
            switch (port)
            {
            case 0:  // set stats on/off
                if (len == 1)
                {
                    byte enable = *reinterpret_cast<byte*>(payload);
                    if (enable)
                        stats_.enableIfNot();
                    else
                        stats_.disable();
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

        if (stats_.isEnabled()) stats_.addToStats(0, port, toSimAddress, len, buffer);
        taskCAN_.sendMessage(0, port, toSimAddress, len, buffer);
    }

private:
    TaskSimManager taskSimManager_;
    uint16_t simAddress_;

    StatsTask stats_;
};

InstrumentMaster s_instrument(LED_PORT, BUTTON_PORT, MCP2515_SPI_PORT, MCP2515_INT_PIN);

void setup()
{
    // skip serial without logging in favor of siminnovation interface
#ifndef USE_SIMESSAGE
    Serial.begin(115200);
    Serial.println("Debug mode");
#endif

    s_instrument.setup();
}

void loop()
{
    s_instrument.run();
}
