
#include <Common.h>

#include <BasicInstrument.h>
#include <Bounce2.h>

// hardware speficics
const byte BUTTON_PORT = 7;
const byte LED_PORT = 3;
const byte MCP2515_SPI_PORT = 10;
const byte MCP2515_INT_PIN = 2;

class GenericButtonsInstrument : public BasicInstrument, private Task
{
public:
    GenericButtonsInstrument(byte ledPin, byte buttonPin, byte canSPIPin, byte canIntPin)
        : BasicInstrument(ledPin, buttonPin, canSPIPin, canIntPin),
          Task(5 * TASK_MILLISECOND, TASK_FOREVER, &taskManager_, false)
    {
        lutKIdx_ = addLUT("k", 16);
    }

    void setup()
    {
        BasicInstrument::setup();
        StoredLUT& lut = getLUT(lutKIdx_);
        buttons_.resize(lut.size());
        for (int i = 0; i < lut.size(); ++i)
        {
            buttons_[i].attach(lut.x()[i], INPUT_PULLUP);
            buttons_[i].interval(20);
            buttons_[i].setPressedState(LOW);
        }

        enable();
    }

protected:
    virtual void setVar(byte idx, float value) override { BasicInstrument::setVar(idx, value); }

    bool Callback() override
    {
        for (int i = 0; i < buttons_.size(); ++i)
        {
            buttons_[i].update();
            if (buttons_[i].changed()) onInstrumentButtonPressed(buttons_[i].pressed(), buttons_[i].getPin());
        }
        return true;
    }

private:
    virtual void onCANReceived(byte priority, byte port, uint16_t srcAddress, uint16_t dstAddress, byte len,
                               byte* payload) override
    {
        if (port != 0) return;

        for (int i = 0; i < buttons_.size(); ++i)
        {
            onInstrumentButtonPressed(buttons_[i].isPressed(), buttons_[i].getPin());
        }
    }

    void onInstrumentButtonPressed(bool pressed, byte port)
    {
        StoredLUT& lut = getLUT(lutKIdx_);
        uint8_t payload = (uint8_t)pressed;

        // find logical pin
        byte logicalPin = 0;
        for (int i = 0; i < lut.size(); ++i)
        {
            if (lut.x()[i] == port)
            {
                logicalPin = lut.y()[i];
                break;
            }
        }
        taskCAN_.sendMessage(0, logicalPin, 0, 1, &payload);
    }

private:
    byte lutKIdx_ = 0;
    std::vector<Bounce2::Button> buttons_;
};

GenericButtonsInstrument s_instrument(LED_PORT, BUTTON_PORT, MCP2515_SPI_PORT, MCP2515_INT_PIN);

void setup()
{
    Serial.begin(9600);
    s_instrument.setup();
}

void loop()
{
    s_instrument.run();
}
