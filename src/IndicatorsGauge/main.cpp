
#include <Common.h>

#include <BasicInstrument.h>
#include <Interpolation.h>

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
        pos_[0] = addPos("a");
        pos_[1] = addPos("b");
        pos_[2] = addPos("c");
        pos_[3] = addPos("d");

        lut_[0] = addLUT("a", 10);
        lut_[1] = addLUT("b", 10);
        lut_[2] = addLUT("c", 10);
        lut_[3] = addLUT("d", 10);
    }

protected:
    virtual int32_t posForLut(byte idxLut) override { return positions_[idxLut]; }
    virtual int32_t pos(byte idxPos) override { return positions_[idxPos]; }

    virtual void setPos(byte idx, int32_t value, bool absolute = true) override
    {
        BasicInstrument::setPos(idx, value, absolute);

        static byte portMap[4] = {3, 5, 6, 9};

        uint8_t newPos = (absolute ? value : positions_[idx] + value);
        pinMode(portMap[idx], OUTPUT);
        analogWrite(portMap[idx], newPos);
        positions_[idx] = newPos;
    }

    virtual void setLPos(byte idx, float value, bool absolute = true) override
    {
        BasicInstrument::setLPos(idx, value, absolute);
        StoredLUT& lut = getLUT(idx);
        int32_t p = (int32_t)(catmullSplineInterpolate<double, double>(lut.x(), lut.y(), lut.size(), value));
        setPos(idx, p, absolute);
    }

private:
    virtual void onCANReceived(byte priority, byte port, uint16_t srcAddress, uint16_t dstAddress, byte len,
                               byte* payload) override
    {
        if (port >= 4 || len != 4) return;

        float pos = *reinterpret_cast<float*>(payload);
        setLPos(port, pos);
    }

private:
    byte pos_[4];
    byte lut_[4];

    uint8_t positions_[4] = {0, 0, 0, 0};
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
