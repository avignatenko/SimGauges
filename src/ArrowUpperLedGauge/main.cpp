
#include <Common.h>

#include <BasicInstrument.h>
#include <Bounce2.h>

// hardware speficics
const byte BUTTON_PORT = 7;
const byte LED_PORT = 3;
const byte MCP2515_SPI_PORT = 10;
const byte MCP2515_INT_PIN = 2;

class ArrowUpperLedGauge : public BasicInstrument, private Task
{
public:
    ArrowUpperLedGauge(byte ledPin, byte buttonPin, byte canSPIPin, byte canIntPin)
        : BasicInstrument(ledPin, buttonPin, canSPIPin, canIntPin),
          Task(5 * TASK_MILLISECOND, TASK_FOREVER, &taskManager_, false)
    {
    }

    void setup()
    {
        BasicInstrument::setup();

        b1_.attach(6, INPUT_PULLUP);
        b1_.interval(20);
        b1_.setPressedState(LOW);

        b2_.attach(A3, INPUT_PULLUP);
        b2_.interval(20);
        b2_.setPressedState(LOW);

        enable();
    }

protected:
    bool Callback() override
    {
        b1_.update();
        if (b1_.changed()) onInstrumentButtonPressed(b1_.pressed(), 0);

        b2_.update();
        if (b2_.changed()) onInstrumentButtonPressed(b2_.pressed(), 1);

        return true;
    }

private:
    virtual void onCANReceived(byte priority, byte port, uint16_t srcAddress, uint16_t dstAddress, byte len,
                               byte* payload) override
    {
        if (port >= 8 || len != 1) return;

        static int s_leds[] = {4, 5, 8, 9, A0, A1, A2};
        byte value = *payload;

        pinMode(s_leds[port], OUTPUT);
        digitalWrite(s_leds[port], value > 0 ? 255 : 0);
    }

    void onInstrumentButtonPressed(bool pressed, byte port)
    {
        uint8_t payload = (uint8_t)pressed;
        taskCAN_.sendMessage(0, port, 0, 1, &payload);
    }

private:
    Bounce2::Button b1_;
    Bounce2::Button b2_;
};

ArrowUpperLedGauge s_instrument(LED_PORT, BUTTON_PORT, MCP2515_SPI_PORT, MCP2515_INT_PIN);

void setup()
{
    Serial.begin(9600);
    s_instrument.setup();
}

void loop()
{
    s_instrument.run();
}
