#include <Arduino.h>
#include <CoopTask.h>
#include <SwitecX25.h>
#include <mcp_can.h>

// hardware speficics
const int BUTTON_PORT = 7;
const int LED_PORT = 4;
const int MCP2515_SPI_PORT = 10;
const int MCP2515_INT_PIN = 2;

// standard X25.168 range 315 degrees at 1/3 degree steps
#define STEPS (315 * 3)
// For motors connected to digital pins 4,5,6,7
SwitecX25 motor1(STEPS, A0, A1, A2, A3);

// struct can_frame canMsg;
MCP_CAN mcpCAN(MCP2515_SPI_PORT);

enum Error
{
    ERROR_OK = 0,
    ERROR_CAN = (1 << 0),
};

int s_error = ERROR_OK;

void digitalWritePeriod(int port, int value, int period)
{
    digitalWrite(port, value);
    delay(period);
    digitalWrite(port, 1 - value);
}

int getHighestBit(int value)
{
    int r = 0;
    while (value >>= 1) r++;
    return r;
}

int loopErrorLed()
{
    pinMode(LED_PORT, OUTPUT);

    // test 2sec on
    digitalWritePeriod(LED_PORT, HIGH, 2000);

    for (;;)
    {
        if (s_error == ERROR_OK)
        {
            delay(100);
            continue;
        }

        int highestBit = getHighestBit(s_error);
        int delayMs = 1000 / (highestBit + 1);
        digitalWritePeriod(LED_PORT, LOW, delayMs);
        delay(delayMs);
    }

    return 0;
}

int loopCAN()
{
    for (int i = 0; i < 5; ++i)
    {
        if (CAN_OK == mcpCAN.begin(CAN_500KBPS))  // init can bus : baudrate = 500k
        {
            Serial.println("CAN BUS Shield init ok!");
            s_error &= ~ERROR_CAN;
            break;
        }
        else
        {
            Serial.println("CAN BUS Shield init fail");
            Serial.println("Init CAN BUS Shield again");
            s_error |= ERROR_CAN;
            delay(500);
            continue;
        }
    }

    for (;;)
    {
        static unsigned char stmp[8] = {0, 1, 2, 3, 4, 5, 6, 7};
        // send data:  id = 0x00, standrad flame, data len = 8, stmp: data buf
        mcpCAN.sendMsgBuf(0x12345678, 1, 8, stmp);
        yield();
    }

    return 0;
}

int loopCANCheck()
{
    for (;;)
    {
        delay(1000);
        int error = mcpCAN.checkError();
        if (error != 0 && !(s_error & ERROR_CAN))
        {
            Serial.print("CAN error: ");
            Serial.println(error);
            s_error |= ERROR_CAN;
        }
    }
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
CoopTask<> taskCANCheck("canCheck", loopCANCheck);
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
    taskCANCheck.scheduleTask();
    taskButton.scheduleTask();
    taskStepper.scheduleTask();
}

void loop()
{
    runCoopTasks();
}
