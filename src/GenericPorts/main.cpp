


#include <Common.h>

#include <BasicInstrument.h>
#include <TaskButton.h>
#include <TaskCAN.h>
#include <TaskErrorLed.h>

// hardware speficics
const byte BUTTON_PORT = 7;
const byte LED_PORT = 3;
const byte MCP2515_SPI_PORT = 53;
const byte MCP2515_INT_PIN = 2;

class GenericPorts : public BasicInstrument
{
public:
    GenericPorts(byte ledPin, byte buttonPin, byte canSPIPin, byte canIntPin)
        : BasicInstrument(ledPin, buttonPin, canSPIPin, canIntPin)
    {
    }

    void setup() { BasicInstrument::setup(); }

protected:
    virtual void setVar(byte idx, float value) override { BasicInstrument::setVar(idx, value); }

private:
    virtual void onCANReceived(byte priority, byte port, uint16_t srcAddress, uint16_t dstAddress, byte len,
                               byte* payload) override
    {
    }

private:
};

GenericPorts s_instrument(LED_PORT, BUTTON_PORT, MCP2515_SPI_PORT, MCP2515_INT_PIN);

void setup()
{
    Serial.begin(9600);
    s_instrument.setup();
}

void loop()
{
    s_instrument.run();
}
