
#include <Common.h>

#include <BasicInstrument.h>
#include <Bounce2.h>

// hardware speficics
ArduinoPin BUTTON_PORT = 7;
ArduinoPin LED_PORT = 3;
ArduinoPin MCP2515_SPI_PORT = 10;
ArduinoPin MCP2515_INT_PIN = 2;

ArduinoPin LED_1_PORT = A3;
ArduinoPin LED_2_PORT = A2;
ArduinoPin LED_3_PORT = A1;
ArduinoPin LED_4_PORT = A0;
ArduinoPin BIG_BUTTON_PORT = A4;

class STec30Alt : public BasicInstrument, private Task
{
public:
    STec30Alt(ArduinoPin& led1Pin, ArduinoPin& led2Pin, ArduinoPin& led3Pin, ArduinoPin& led4Pin,
              ArduinoPin& bigButtonPin, ArduinoPin& ledPin, ArduinoPin& buttonPin, ArduinoPin& canSPIPin,
              ArduinoPin& canIntPin)
        : BasicInstrument(ledPin, buttonPin, canSPIPin, canIntPin),
          Task(5 * TASK_MILLISECOND, TASK_FOREVER, &taskManager_, false),
          led1PortAlt_(led1Pin),
          led2PortOn_(led2Pin),
          led3PortDown_(led3Pin),
          led4PortUp_(led4Pin),
          bigButtonPort_(bigButtonPin)
    {
    }

    void setup()
    {
        BasicInstrument::setup();

        bigButton_.attach(bigButtonPort_.toInt(), INPUT_PULLUP);
        bigButton_.interval(20);
        bigButton_.setPressedState(LOW);

        led1PortAlt_.pinMode(OUTPUT);
        led1PortAlt_.digitalWrite(HIGH);

        led2PortOn_.pinMode(OUTPUT);
        led2PortOn_.digitalWrite(HIGH);
        
        led3PortDown_.pinMode(OUTPUT);
        led3PortDown_.digitalWrite(LOW);
        
        led4PortUp_.pinMode(OUTPUT);
        led4PortUp_.digitalWrite(HIGH);
        
        enable();
    }

protected:
    bool Callback() override
    {
        bigButton_.update();
        if (bigButton_.changed()) onInstrumentButtonPressed(bigButton_.pressed(), 0);

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
        Serial.println("AA");
        uint8_t payload = (uint8_t)pressed;
        taskCAN_.sendMessage(0, port, 0, 1, &payload);
    }

private:
    Bounce2::Button bigButton_;
  
    ArduinoPin& led1PortAlt_;
    ArduinoPin& led2PortOn_;
    ArduinoPin& led3PortDown_;
    ArduinoPin& led4PortUp_;
    ArduinoPin& bigButtonPort_;
};

STec30Alt s_instrument(LED_1_PORT, LED_2_PORT, LED_3_PORT, LED_4_PORT, BIG_BUTTON_PORT, LED_PORT, BUTTON_PORT,
                       MCP2515_SPI_PORT, MCP2515_INT_PIN);

void setup()
{
    Serial.begin(9600);
    s_instrument.setup();
}

void loop()
{
    s_instrument.run();
}
