#include <Arduino.h>
#include <CoopTask.h>
#include <SPI.h>
#include <mcp2515.h>

// hardware speficics
const int BUTTON_PORT = 7;
const int LED_PORT = 4;

struct can_frame canMsg;
MCP2515 mcp2515(10);

enum class Error
{
    OK = 0,
    CAN = 1,
};

Error s_error = Error::OK;

int loopErrorLed()
{
    pinMode(LED_PORT, OUTPUT);
    digitalWrite(LED_PORT, HIGH);
    delay(2000);
    digitalWrite(LED_PORT, LOW);

    for (;;)
    {
        if (s_error == Error::OK)
        {
            delay(10);
            continue;
        }
        int delayMs = 1000 / static_cast<int>(s_error);
        digitalWrite(LED_PORT, LOW);
        delay(delayMs);
        digitalWrite(LED_PORT, HIGH);
        delay(delayMs);
    }

    return 0;
}

void processCANError(MCP2515::ERROR e)
{
    Serial.print("CAN Error : ");
    Serial.println(e);

    if (s_error == Error::OK && e != MCP2515::ERROR_OK) s_error = Error::CAN;
}

int loopCAN()
{
    MCP2515::ERROR e = MCP2515::ERROR_OK;
    e = mcp2515.reset();
    processCANError(e);
    e = mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
    processCANError(e);
    e = mcp2515.setNormalMode();
    processCANError(e);

    for (;;)
    {
        canMsg.can_id = 0x12345678 | CAN_EFF_FLAG;
        canMsg.can_dlc = 4;
        canMsg.data[0] = 101;
        mcp2515.sendMessage(&canMsg);
        yield();
    }
}

int loopButton()
{
    pinMode(BUTTON_PORT, INPUT_PULLUP);

    for (;;)
    {
        int sensorVal = digitalRead(BUTTON_PORT);
        if (sensorVal == LOW) digitalWrite(LED_PORT, HIGH);
        delay(10);
    }
}

CoopTask<> taskErrorLed("errorLed", loopErrorLed);
CoopTask<> taskCAN("can", loopCAN);
CoopTask<> taskButton("button", loopButton);

void setup()
{
    while (!Serial)
        ;
    Serial.begin(115200);
    SPI.begin();

    delay(2000);

    taskErrorLed.scheduleTask();
    taskCAN.scheduleTask();
    taskButton.scheduleTask();
}

void loop()
{
    runCoopTasks();
}
