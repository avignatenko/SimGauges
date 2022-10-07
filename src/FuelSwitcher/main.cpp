
#include <Common.h>

#include <BasicInstrument.h>

// hardware speficics
const byte BUTTON_PORT = 7;
const byte LED_PORT = 3;
const byte MCP2515_SPI_PORT = 10;
const byte MCP2515_INT_PIN = 2;
const byte INPUT_PIN = A0;
const int OFF_TH = 600;
const int RIGHT_TH = 320;

class GenericButtonsInstrument : public BasicInstrument, private Task
{
public:
    GenericButtonsInstrument(byte ledPin, byte buttonPin, byte canSPIPin, byte canIntPin)
        : BasicInstrument(ledPin, buttonPin, canSPIPin, canIntPin),
          Task(100 * TASK_MILLISECOND, TASK_FOREVER, &taskManager_, false)
    {
    }

    void setup()
    {
        BasicInstrument::setup();
        pos_ = curPos();
        enable();
    }

protected:
    virtual void setVar(byte idx, float value) override { BasicInstrument::setVar(idx, value); }

    bool Callback() override
    {
        int newPos = curPos();
        if (newPos != pos_)
        {
            pos_ = newPos;
            sendValue();
        }
        return true;
    }

private:
    virtual void onCANReceived(byte priority, byte port, uint16_t srcAddress, uint16_t dstAddress, byte len,
                               byte* payload) override
    {
        if (port != 0) return;

        sendValue();
    }

    void sendValue()
    {
        uint8_t payload = (uint8_t)pos_;
        taskCAN_.sendMessage(0, 0, 0, 1, &payload);
        //Serial.println(pos_);
    }
    int posFromValue(int value)
    {
        //Serial.println(value);
        if (value > OFF_TH) return 0;
        if (value < RIGHT_TH) return 2;
        return 1;
    }

    int curPos()
    {
        int sensorValue = analogRead(INPUT_PIN);
        return posFromValue(sensorValue);
    }

private:
    byte lutKIdx_ = 0;
    std::vector<Bounce2::Button> buttons_;
    int pos_ = 0;
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
