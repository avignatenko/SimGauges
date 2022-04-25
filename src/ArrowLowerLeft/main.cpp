

#include <Common.h>

#include <BasicInstrument.h>
#include <Bounce2.h>
#include "FastDelegate.h"

// hardware speficics
const byte BUTTON_PORT = 7;
const byte LED_PORT = 3;
const byte MCP2515_SPI_PORT = 53;
const byte MCP2515_INT_PIN = 2;

const byte GEAR_BUTTON_PORT = 12;

const byte KEY_PORT_BEGIN = 22;
const byte KEY_PORT_COUNT = 5;

const byte LED_BASE_PORT = 8;

// API ports
const byte API_PORT_GEAR = 4;
const byte API_PORT_KEY = 5;

class ButtonsTasks : private Task
{
public:
    ButtonsTasks(Scheduler& sh) : Task(5 * TASK_MILLISECOND, TASK_FOREVER, &sh, true)
    {
        gearButton_.attach(GEAR_BUTTON_PORT, INPUT_PULLUP);
        gearButton_.interval(20);
        gearButton_.setPressedState(LOW);
    }

    void setGearSwitchCallback(fastdelegate::FastDelegate1<bool> callback) { gearCallback_ = callback; }
    void setKeySwitchCallback(fastdelegate::FastDelegate1<int8_t> callback) { keyCallback_ = callback; }

    bool gearSwitchValue() { return gearButton_.pressed(); }
    bool keySwitchValue() { return keyButton_; }

private:
    bool Callback() override
    {
        updateKeyState();

        if (gearCallback_)
        {
            gearButton_.update();

            if (gearButton_.pressed())
                gearCallback_(true);
            else if (gearButton_.released())
                gearCallback_(false);
        }
        return true;
    }

    void updateKeyState()
    {
        for (int i = KEY_PORT_BEGIN; i < KEY_PORT_BEGIN + KEY_PORT_COUNT; ++i)
        {
            pinMode(i, OUTPUT);
            pinMode(i + 1, INPUT_PULLUP);

            digitalWrite(i, LOW);
            if (digitalRead(i + 1) == 0)
            {
                int keyButton = i - KEY_PORT_BEGIN;
                if (keyButton != keyButton_)
                {
                    keyButton_ = keyButton;
                    if (keyCallback_) keyCallback_(keyButton);
                }

                digitalWrite(i, HIGH);
                pinMode(i, INPUT_PULLUP);
                break;
            };

            digitalWrite(i, HIGH);
            pinMode(i, INPUT_PULLUP);
        }
    }

private:
    Bounce2::Button gearButton_;
    int keyButton_ = -1;
    fastdelegate::FastDelegate1<bool> gearCallback_;
    fastdelegate::FastDelegate1<int8_t> keyCallback_;
};

class ArrowLowerLeft : public BasicInstrument
{
public:
    ArrowLowerLeft(byte ledPin, byte buttonPin, byte canSPIPin, byte canIntPin)
        : BasicInstrument(ledPin, buttonPin, canSPIPin, canIntPin), buttons_(taskManager_)
    {
        buttons_.setGearSwitchCallback(fastdelegate::FastDelegate1<bool>(this, &ArrowLowerLeft::onGearSwitchChanged));
        buttons_.setKeySwitchCallback(fastdelegate::FastDelegate1<int8_t>(this, &ArrowLowerLeft::onKeySwitchChanged));
    }

    void setup() { BasicInstrument::setup(); }

protected:
    virtual void setVar(byte idx, float value) override { BasicInstrument::setVar(idx, value); }

private:
    virtual void onCANReceived(byte priority, byte port, uint16_t srcAddress, uint16_t dstAddress, byte len,
                               byte* payload) override
    {
        if (port <= 3 && len == 1)
        {
            byte payloadCast = *reinterpret_cast<byte*>(payload);
            setLed(port, payloadCast);
            return;
        }

        if (port == API_PORT_GEAR)
        {
            onGearSwitchChanged(buttons_.gearSwitchValue());
            return;
        }

        if (port == API_PORT_KEY)
        {
            onGearSwitchChanged(buttons_.keySwitchValue());
            return;
        }
    }

    void setLed(int8_t idx, bool on)
    {
        int8_t pin = LED_BASE_PORT + idx;
        pinMode(pin, OUTPUT);
        analogWrite(pin, on ? 255 : 0);
    }

    void onGearSwitchChanged(bool value)
    {
        byte valueBuffer = value;
        taskCAN_.sendMessage(0, API_PORT_GEAR, 0, sizeof(byte), (byte*)&valueBuffer);
    }
    void onKeySwitchChanged(int8_t value) { taskCAN_.sendMessage(0, API_PORT_KEY, 0, sizeof(byte), (byte*)&value); }

private:
    ButtonsTasks buttons_;
};

ArrowLowerLeft s_instrument(LED_PORT, BUTTON_PORT, MCP2515_SPI_PORT, MCP2515_INT_PIN);

void setup()
{
    Serial.begin(9600);
    s_instrument.setup();
}

void loop()
{
    s_instrument.run();
}
