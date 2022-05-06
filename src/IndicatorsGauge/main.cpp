
#include <Common.h>

#include <BasicInstrument.h>
#include <Bounce2.h>

// hardware speficics
const byte BUTTON_PORT = 7;
const byte LED_PORT = 4;
const byte MCP2515_SPI_PORT = 10;
const byte MCP2515_INT_PIN = 2;

class IndicatorsGauge : public BasicInstrument
{
public:
    IndicatorsGauge(byte ledPin, byte buttonPin, byte canSPIPin, byte canIntPin)
        : BasicInstrument(ledPin, buttonPin, canSPIPin, canIntPin)
    {
    }

protected:
private:
    virtual void onCANReceived(byte priority, byte port, uint16_t srcAddress, uint16_t dstAddress, byte len,
                               byte* payload) override
    {
        Serial.println("CAN");
        if (port >= 4 || len != 1) return;

        static byte portMap[4] = {3, 5, 6, 9};

        byte value = *payload;
        pinMode(portMap[port], OUTPUT);
        analogWrite(portMap[port], value);
    }

private:
};

IndicatorsGauge s_instrument(LED_PORT, BUTTON_PORT, MCP2515_SPI_PORT, MCP2515_INT_PIN);

void setup()
{
    Serial.begin(9600);
    s_instrument.setup();
}

void loop()
{
    s_instrument.run();
}
