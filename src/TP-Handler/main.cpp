
#include "Bounce3.h"

#include <Common.h>

#include <CommonInstrument.h>

#include <LedControl_SW_SPI.h>
#include <MCP23017.h>
#include <MenuVars.h>
#include <TaskButton.h>
#include <TaskEncoder.h>
#include <TaskMenu.h>

//  hardware speficics
MCP23017 mcp(0x20);

const uint8_t AP_SELECTOR_PORTS_NUM = 5;
MCP23X17Pin AP_SELECTOR_PORTS[AP_SELECTOR_PORTS_NUM] = {MCP23X17Pin(mcp, 0), MCP23X17Pin(mcp, 1), MCP23X17Pin(mcp, 2),
                                                        MCP23X17Pin(mcp, 3), MCP23X17Pin(mcp, 4)};

const uint8_t ENC_1_CW_PORT = 4;
const uint8_t ENC_1_CCW_PORT = 3;

const uint8_t ENC_2_CW_PORT = A3;
const uint8_t ENC_2_CCW_PORT = A2;

const uint8_t ENC_3_CW_PORT = A1;
const uint8_t ENC_3_CCW_PORT = A0;

const uint8_t ENC_4_CW_PORT = 5;
const uint8_t ENC_4_CCW_PORT = 6;

const uint8_t DIGITS_DATA_PIN = 9;
const uint8_t DIGITS_CLK_PIN = 7;
const uint8_t DIGITS_CS_PIN = 8;

const byte MCP2515_SPI_PORT = 10;
const byte MCP2515_INT_PIN = 2;

MCP23X17Pin s_ledPin(mcp, 5);
MCP23X17Pin s_errorLedPin(mcp, 8);
MCP23X17Pin s_buttonPin(mcp, 7);
MCP23X17Pin s_actionButtonPin(mcp, 6);

class TaskSelector : private Task
{
public:
    TaskSelector(Scheduler& sh) : Task(20 * TASK_MILLISECOND, TASK_FOREVER, &sh, false) {}

    void setSelectorCallback(fastdelegate::FastDelegate1<int8_t> callback) { apSelectorCallback_ = callback; }

    uint8_t selectorValue()
    {
        for (uint8_t i = 0; i < AP_SELECTOR_PORTS_NUM; ++i)
        {
            if (apSelector_[i].isPressed())
            {
                return i + 1;
            }
        }
        return 0;
    }

    void start()
    {
        for (uint8_t i = 0; i < AP_SELECTOR_PORTS_NUM; ++i)
        {
            apSelector_[i].interval(20);
            apSelector_[i].setPressedState(LOW);
        }
        enable();
    }

private:
    bool Callback() override
    {
        if (apSelectorCallback_)
        {
            for (uint8_t i = 0; i < AP_SELECTOR_PORTS_NUM; ++i)
            {
                apSelector_[i].update();
                if (apSelector_[i].pressed())
                {
                    apSelectorCallback_(i + 1);
                    return true;
                }
            }

            // if (apSelector_[0].released() && selectorValue() == 0)
            //     apSelectorCallback_(0);
        }

        return true;
    }

private:
    Bounce2::Button3 apSelector_[AP_SELECTOR_PORTS_NUM] = {
        Bounce2::Button3(AP_SELECTOR_PORTS[0]), Bounce2::Button3(AP_SELECTOR_PORTS[1]),
        Bounce2::Button3(AP_SELECTOR_PORTS[2]), Bounce2::Button3(AP_SELECTOR_PORTS[3]),
        Bounce2::Button3(AP_SELECTOR_PORTS[4])};

    fastdelegate::FastDelegate1<int8_t> apSelectorCallback_;
};

class TaskDigits : private Task
{
public:
    TaskDigits(Scheduler* aScheduler = NULL) : Task(TASK_IMMEDIATE, TASK_ONCE, aScheduler, false) {}
    virtual bool Callback() override { return false; }
    void start()
    {
        lc_.begin(DIGITS_DATA_PIN, DIGITS_CLK_PIN, DIGITS_CS_PIN);
        /*
         The MAX72XX is in power-saving mode on startup,
         we have to do a wakeup call
         */
        lc_.shutdown(0, false);
        /* Set the brightness to a medium values */
        lc_.setIntensity(0, 1);
        /* and clear the display */
        lc_.clearDisplay(0);

        enable();
    }

    void setDigit(uint8_t idx, uint8_t value)
    {
        digits_[idx] = value;
        if (on_) lc_.setDigit(0, idx, value, false);
    }

    uint8_t digit(uint8_t idx) { return digits_[idx]; }

    void setOn()
    {
        on_ = true;

        setDigit(0, digits_[0]);
        setDigit(1, digits_[1]);
        setDigit(2, digits_[2]);
        setDigit(3, digits_[3]);
    }

    void setOff()
    {
        on_ = false;
        lc_.clearDisplay(0);
    }

protected:
    virtual bool OnEnable() override { return true; }

    virtual void OnDisable() override {}

private:
    LedControl_SW_SPI lc_;
    uint8_t digits_[4] = {0, 0, 0, 0};
    bool on_ = false;
};

class TPHandler : public CommonInstrument
{
public:
    TPHandler()
        : CommonInstrument(s_errorLedPin, s_buttonPin, MCP2515_SPI_PORT, MCP2515_INT_PIN),
          taskDigits_(&taskManager_),
          taskButton_(taskManager_, s_actionButtonPin),
          taskSelector_(taskManager_),
          taskEncoder1_(taskManager_, ENC_1_CW_PORT, ENC_1_CCW_PORT),
          taskEncoder2_(taskManager_, ENC_2_CW_PORT, ENC_2_CCW_PORT),
          taskEncoder3_(taskManager_, ENC_3_CW_PORT, ENC_3_CCW_PORT),
          taskEncoder4_(taskManager_, ENC_4_CW_PORT, ENC_4_CCW_PORT),
          taskMenu_(taskManager_),
          vars_({"addr"}),
          varsMenu_(vars_, taskMenu_.cmdLine())

    {
        vars_.setVarCallback(etl::delegate<void(uint8_t, float)>::create<TPHandler, &TPHandler::onVarSet>(*this));
        taskCAN_.setSimAddress(vars_.getVar(0));

        taskButton_.setPressedCallback(fastdelegate::FastDelegate2<bool, Pin&>(this, &TPHandler::onXPDRButtonPressed));
        taskSelector_.setSelectorCallback(fastdelegate::FastDelegate1<int8_t>(this, &TPHandler::onSelectorSwitched));
        taskEncoder1_.setRotationCallback(fastdelegate::FastDelegate2<int8_t, long>(this, &TPHandler::onEncoder1));
        taskEncoder2_.setRotationCallback(fastdelegate::FastDelegate2<int8_t, long>(this, &TPHandler::onEncoder2));
        taskEncoder3_.setRotationCallback(fastdelegate::FastDelegate2<int8_t, long>(this, &TPHandler::onEncoder3));
        taskEncoder4_.setRotationCallback(fastdelegate::FastDelegate2<int8_t, long>(this, &TPHandler::onEncoder4));
    }

    void onVarSet(uint8_t var, float value)
    {
        // there is only one var
        taskCAN_.setSimAddress((uint16_t)value);
    }

    virtual void setup() override
    {
        CommonInstrument::setup();

        // configure LED pins for output
        s_ledPin.pinMode(OUTPUT);
        s_ledPin.digitalWrite(LOW);

        taskDigits_.start();

        taskButton_.start();
        taskSelector_.start();

        updateScreenOnOff();

        taskEncoder1_.start();
        taskEncoder2_.start();
        taskEncoder3_.start();
        taskEncoder4_.start();

        taskMenu_.cmdLine().AddCommand(new SerialCommand("help", helpCallback));
        taskMenu_.start();
    }

private:
    static void helpCallback(SerialCommands* cmd, void* param)
    {
        TPHandler* me = reinterpret_cast<TPHandler*>(param);
        me->helpCallback(cmd);
    }

    void helpCallback(SerialCommands* cmd)
    {
        Stream* serial = cmd->GetSerial();
        serial->println(F("Help: "));
        serial->print(" ");
        serial->println(varsMenu_.help());
    }

    virtual void onCANReceived(byte priority, byte port, uint16_t srcAddress, uint16_t dstAddress, byte len,
                               byte* payload) override
    {
        if (port == 0)
        {
            sendStatus();
            return;
        }

        if (port == 1 && len == sizeof(uint8_t))
        {
            uint8_t value = *(reinterpret_cast<int8_t*>(payload));
            voltage_ = value;
            updateScreenOnOff();
            updateLedOnOff();
        }

        if (port == 2 && len == sizeof(uint8_t))
        {
            uint8_t value = *(reinterpret_cast<int8_t*>(payload));
            led_ = value;
            updateLedOnOff();
        }
    }

    void onXPDRButtonPressed(bool pressed, Pin& btn)
    {
        if (pressed) return;  // send on release

        sendButton();
    }

    void onSelectorSwitched(int8_t idx)
    {
        sendSelector();
        updateScreenOnOff();
    }

    bool voltageOk() { return voltage_ >= 10; }
    void updateScreenOnOff()
    {
        uint8_t idx = taskSelector_.selectorValue();
        if (idx == 1 || !voltageOk())
            taskDigits_.setOff();
        else
            taskDigits_.setOn();
    }

    void updateLedOnOff() { s_ledPin.digitalWrite(HIGH * led_ * (voltageOk() ? 1 : 0)); }
    uint8_t increment(uint8_t digit, int8_t dir)
    {
        int8_t newDigit = (int8_t)digit + dir;
        return ((uint8_t)newDigit) % 8;
    }

    uint16_t makeTransponderNumber()
    {
        uint16_t res = taskDigits_.digit(3);
        res += taskDigits_.digit(2) * 10;
        res += taskDigits_.digit(1) * 100;
        res += taskDigits_.digit(0) * 1000;
        return res;
    }

    void onEncoder(uint8_t idx, int8_t dir)
    {
        taskDigits_.setDigit(idx, increment(taskDigits_.digit(idx), dir));
        sendSquawk();
    }

    void onEncoder1(int8_t dir, long) { onEncoder(0, dir); }
    void onEncoder2(int8_t dir, long) { onEncoder(1, dir); }
    void onEncoder3(int8_t dir, long) { onEncoder(3, dir); }
    void onEncoder4(int8_t dir, long) { onEncoder(2, dir); }

    void sendButton()
    {
        uint8_t value = 0;
        taskCAN_.sendMessage(0, 2, 0, sizeof(value), &value);
    }

    void sendSelector()
    {
        uint8_t value = taskSelector_.selectorValue();
        taskCAN_.sendMessage(0, 0, 0, sizeof(value), &value);
    }

    void sendSquawk()
    {
        uint16_t newValue = makeTransponderNumber();
        taskCAN_.sendMessage(0, 1, 0, sizeof(newValue), reinterpret_cast<byte*>(&newValue));
    }

    void sendStatus()
    {
        sendSelector();
        sendSquawk();
    }

private:
    TaskDigits taskDigits_;
    TaskButton taskButton_;
    TaskSelector taskSelector_;
    TaskEncoder taskEncoder1_;
    TaskEncoder taskEncoder2_;
    TaskEncoder taskEncoder3_;
    TaskEncoder taskEncoder4_;
    TaskMenu taskMenu_;
    VarsStorage<1, 0> vars_;
    VarsMenu varsMenu_;

    uint8_t voltage_ = 12;
    uint8_t led_ = 0;
};

TPHandler* s_instrument = nullptr;

void setup()
{
    Serial.begin(9600);

    // init MCP first
    if (!mcp.begin())
    {
        Serial.println("I2C Error.");
    }

    s_instrument = new TPHandler();
    s_instrument->setup();
}

void loop()
{
    s_instrument->run();
}
