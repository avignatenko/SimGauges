#include <Arduino.h>
#include <CoopTask.h>
#include <SwitecX25.h>
#include <mcp2515.h>

// hardware speficics
const int BUTTON_PORT = 7;
const int LED_PORT = 4;
const int MCP2515_PORT = 10;

// standard X25.168 range 315 degrees at 1/3 degree steps
#define STEPS (315 * 3)
// For motors connected to digital pins 4,5,6,7
SwitecX25 motor1(STEPS, A0, A1, A2, A3);

struct can_frame canMsg;
MCP2515 mcp2515(MCP2515_PORT);

enum class Error
{
    OK = 0,
    CAN = 1,
};

Error s_error = Error::OK;

void digitalWritePeriod(int port, int value, int period)
{
    digitalWrite(port, value);
    delay(period);
    digitalWrite(port, 1 - value);
}

int loopErrorLed()
{
    pinMode(LED_PORT, OUTPUT);

    // test 2sec on
    digitalWritePeriod(LED_PORT, HIGH, 2000);

    for (;;)
    {
        if (s_error == Error::OK)
        {
            delay(50);
            continue;
        }
        int delayMs = 1000 / static_cast<int>(s_error);
        digitalWritePeriod(LED_PORT, LOW, delayMs);
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

    return 0;
}

int loopButton()
{
    pinMode(BUTTON_PORT, INPUT_PULLUP);

    for (;;)
    {
        int sensorVal = digitalRead(BUTTON_PORT);
        if (sensorVal == LOW)
        {
            digitalWritePeriod(LED_PORT, HIGH, 100);
            continue;
        }
        delay(40);
    }

    return 0;
}

int loopStepper()
{
    // run the motor against the stops
    motor1.zero();
    // start moving towards the center of the range
    motor1.setPosition(0);

    for (;;)
    {
        motor1.update();
        yield();
    }

    return 0;
}

CoopTask<> taskErrorLed("errorLed", loopErrorLed);
CoopTask<> taskCAN("can", loopCAN);
CoopTask<> taskButton("button", loopButton);
CoopTask<> taskStepper("stepper", loopStepper);

void initSerial()
{
    Serial.begin(115200);
    while (!Serial)
        ;
}

void setup()
{
    initSerial();

    SPI.begin();

    taskErrorLed.scheduleTask();
    taskCAN.scheduleTask();
    taskButton.scheduleTask();
    taskStepper.scheduleTask();
}

void loop()
{
    runCoopTasks();
}
