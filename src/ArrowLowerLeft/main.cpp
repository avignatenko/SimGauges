

#include <Common.h>

#include <BasicInstrument.h>
#include <Bounce2.h>

#include "FastDelegate.h"

// fixme: min not found for some reason (STL undefines c macros?)
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

#include <GyverFilters.h>

// hardware speficics
const byte BUTTON_PORT = 7;
const byte LED_PORT = 3;
const byte MCP2515_SPI_PORT = 53;
const byte MCP2515_INT_PIN = 2;

const byte GEAR_BUTTON_PORT = 12;
const byte NAV_SWITCH_BUTTON_PORT_1 = 32;
const byte NAV_SWITCH_BUTTON_PORT_2 = 33;
const byte AP_LEFT_BUTTON_PORT = A4;
const byte AP_RIGHT_BUTTON_PORT = A0;
const byte KEY_PORT_BEGIN = 22;
const byte KEY_PORT_COUNT = 5;
const byte AP_SELECTOR_PORTS[6] = {34, 36, 38, 40, 42, 44};
const byte AP_ROLL_RESISTOR_PORT = A2;

const byte LED_BASE_PORT = 8;

// API ports
const byte API_PORT_GEAR = 4;
const byte API_PORT_KEY = 5;
const byte API_PORT_AP_PANEL_LEFT_BUTTON = 6;
const byte API_PORT_AP_PANEL_RIGHT_BUTTON = 7;
const byte API_PORT_AP_PANEL_ROLL = 8;
const byte API_PORT_AP_PANEL_NAV_SWITCH = 9;
const byte API_PORT_AP_PANEL_NAV_SELECTOR = 10;

class RollResistor : private Task
{
public:
    RollResistor(Scheduler& sh) : Task(100 * TASK_MILLISECOND, TASK_FOREVER, &sh, true)
    {
        pinMode(AP_ROLL_RESISTOR_PORT, INPUT);
    }

    void setCallback(fastdelegate::FastDelegate1<int16_t> callback) { callback_ = callback; }

    int16_t value() { return median_.filtered(analogRead(AP_ROLL_RESISTOR_PORT)); }

private:
    bool Callback() override
    {
        if (callback_)
        {
            int newValue = value();
            if (prevValue_ < 0 || abs(newValue - prevValue_) > 3)
            {
                prevValue_ = newValue;
                callback_(newValue);
            }
        }

        return true;
    }

private:
    GMedian3<int> median_;
    int prevValue_ = -1;

    fastdelegate::FastDelegate1<int16_t> callback_;
};

class ButtonsTasks : private Task
{
public:
    ButtonsTasks(Scheduler& sh) : Task(5 * TASK_MILLISECOND, TASK_FOREVER, &sh, true)
    {
        gearButton_.attach(GEAR_BUTTON_PORT, INPUT_PULLUP);
        gearButton_.interval(20);
        gearButton_.setPressedState(LOW);

        navSwitchButton1_.attach(NAV_SWITCH_BUTTON_PORT_1, INPUT_PULLUP);
        navSwitchButton1_.interval(20);
        navSwitchButton1_.setPressedState(LOW);

        navSwitchButton2_.attach(NAV_SWITCH_BUTTON_PORT_2, INPUT_PULLUP);
        navSwitchButton2_.interval(20);
        navSwitchButton2_.setPressedState(LOW);

        apLeftButton_.attach(AP_LEFT_BUTTON_PORT, INPUT_PULLUP);
        apLeftButton_.interval(20);
        apLeftButton_.setPressedState(LOW);

        apRightButton_.attach(AP_RIGHT_BUTTON_PORT, INPUT_PULLUP);
        apRightButton_.interval(20);
        apRightButton_.setPressedState(LOW);

        for (int i = 0; i < 6; ++i)
        {
            apSelector_[i].attach(AP_SELECTOR_PORTS[i], INPUT_PULLUP);
            apSelector_[i].interval(20);
            apSelector_[i].setPressedState(LOW);
        }
    }

    void setGearSwitchCallback(fastdelegate::FastDelegate1<bool> callback) { gearCallback_ = callback; }
    void setKeySwitchCallback(fastdelegate::FastDelegate1<int8_t> callback) { keyCallback_ = callback; }
    void setNavSwitchCallback(fastdelegate::FastDelegate1<int8_t> callback) { navSwitchCallback_ = callback; }
    void setApLeftSwitchCallback(fastdelegate::FastDelegate1<bool> callback) { apLeftSwitchCallback_ = callback; }
    void setApRightSwitchCallback(fastdelegate::FastDelegate1<bool> callback) { apRightSwitchCallback_ = callback; }
    void setApSelectorCallback(fastdelegate::FastDelegate1<int8_t> callback) { apSelectorCallback_ = callback; }

    bool gearSwitchValue() { return gearButton_.isPressed(); }
    bool keySwitchValue() { return keyButton_; }

    bool apLeftSwitchValue() { return apLeftButton_.isPressed(); }
    bool apRightSwitchValue() { return apRightButton_.isPressed(); }
    int8_t navSwitchValue()
    {
        int8_t value = 0;
        if (navSwitchButton1_.isPressed())
            value = 1;
        else if (navSwitchButton2_.isPressed())
            value = 2;

        return value;
    }
    int8_t selectorValue() {
        for (int i = 0; i < 6; ++i)
        {
            if (apSelector_[i].isPressed())
            {
                apSelectorCallback_(i);
                break;
            }
        }
    }

private:
    bool Callback() override
    {
        updateKeyState();

        if (apSelectorCallback_)
        {
            for (int i = 0; i < 6; ++i)
            {
                apSelector_[i].update();
                if (apSelector_[i].pressed())
                {
                    apSelectorCallback_(i);
                    break;
                }
            }
        }

        if (apLeftSwitchCallback_)
        {
            apLeftButton_.update();
            if (apLeftButton_.changed()) apLeftSwitchCallback_(apLeftButton_.pressed());
        }

        if (apRightSwitchCallback_)
        {
            apRightButton_.update();
            if (apRightButton_.changed()) apRightSwitchCallback_(apRightButton_.pressed());
        }

        if (navSwitchCallback_)
        {
            navSwitchButton1_.update();
            navSwitchButton2_.update();

            if (navSwitchButton1_.changed() || navSwitchButton2_.changed())
            {
                navSwitchCallback_(navSwitchValue());
            }
        }

        if (gearCallback_)
        {
            gearButton_.update();
            if (gearButton_.changed()) gearCallback_(gearButton_.pressed());
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
    Bounce2::Button navSwitchButton1_;
    Bounce2::Button navSwitchButton2_;

    Bounce2::Button apLeftButton_;
    Bounce2::Button apRightButton_;

    Bounce2::Button apSelector_[6];

    int keyButton_ = -1;
    fastdelegate::FastDelegate1<bool> gearCallback_;
    fastdelegate::FastDelegate1<int8_t> keyCallback_;
    fastdelegate::FastDelegate1<int8_t> navSwitchCallback_;
    fastdelegate::FastDelegate1<bool> apLeftSwitchCallback_;
    fastdelegate::FastDelegate1<bool> apRightSwitchCallback_;
    fastdelegate::FastDelegate1<int8_t> apSelectorCallback_;
};

class ArrowLowerLeft : public BasicInstrument
{
public:
    ArrowLowerLeft(byte ledPin, byte buttonPin, byte canSPIPin, byte canIntPin)
        : BasicInstrument(ledPin, buttonPin, canSPIPin, canIntPin), buttons_(taskManager_), rollResistor_(taskManager_)
    {
        buttons_.setGearSwitchCallback(fastdelegate::FastDelegate1<bool>(this, &ArrowLowerLeft::onGearSwitchChanged));
        buttons_.setKeySwitchCallback(fastdelegate::FastDelegate1<int8_t>(this, &ArrowLowerLeft::onKeySwitchChanged));
        buttons_.setNavSwitchCallback(fastdelegate::FastDelegate1<int8_t>(this, &ArrowLowerLeft::onNavSwitchChanged));
        buttons_.setApLeftSwitchCallback(
            fastdelegate::FastDelegate1<bool>(this, &ArrowLowerLeft::onApLeftSwitchChanged));
        buttons_.setApRightSwitchCallback(
            fastdelegate::FastDelegate1<bool>(this, &ArrowLowerLeft::onApRightSwitchChanged));

        buttons_.setApSelectorCallback(fastdelegate::FastDelegate1<int8_t>(this, &ArrowLowerLeft::onApSelectorChanged));

        rollResistor_.setCallback(fastdelegate::FastDelegate1<int16_t>(this, &ArrowLowerLeft::onRollResistor));
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

        if (port == API_PORT_AP_PANEL_LEFT_BUTTON)
        {
            onApLeftSwitchChanged(buttons_.apLeftSwitchValue());
            return;
        }
        if (port == API_PORT_AP_PANEL_RIGHT_BUTTON)
        {
            onApRightSwitchChanged(buttons_.apRightSwitchValue());
            return;
        }
        if (port == API_PORT_AP_PANEL_ROLL)
        {
            onRollResistor(rollResistor_.value());
            return;
        }
        if (port == API_PORT_AP_PANEL_NAV_SWITCH)
        {
            onNavSwitchChanged(buttons_.navSwitchValue());
            return;
        }
        if (port == API_PORT_AP_PANEL_NAV_SELECTOR)
        {
            onApSelectorChanged(buttons_.selectorValue());
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

    void onApLeftSwitchChanged(bool value)
    {
        byte valueBuffer = value;
        taskCAN_.sendMessage(0, API_PORT_AP_PANEL_LEFT_BUTTON, 0, sizeof(byte), (byte*)&valueBuffer);
    }

    void onApRightSwitchChanged(bool value)
    {
        byte valueBuffer = value;
        taskCAN_.sendMessage(0, API_PORT_AP_PANEL_RIGHT_BUTTON, 0, sizeof(byte), (byte*)&valueBuffer);
    }

    void onKeySwitchChanged(int8_t value) { taskCAN_.sendMessage(0, API_PORT_KEY, 0, sizeof(byte), (byte*)&value); }
    void onNavSwitchChanged(int8_t value)
    {
        taskCAN_.sendMessage(0, API_PORT_AP_PANEL_NAV_SWITCH, 0, sizeof(byte), (byte*)&value);
    }

    void onApSelectorChanged(int8_t value)
    {
        taskCAN_.sendMessage(0, API_PORT_AP_PANEL_NAV_SELECTOR, 0, sizeof(byte), (byte*)&value);
    }

    void onRollResistor(int16_t value)
    {
        Serial.println(value);
        float fValue = (float)value;
        taskCAN_.sendMessage(0, API_PORT_AP_PANEL_ROLL, 0, sizeof(float), (byte*)&fValue);
    }

private:
    ButtonsTasks buttons_;
    RollResistor rollResistor_;
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
