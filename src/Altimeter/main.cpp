
#include <Common.h>

#include "TaskAltimeterMenu.h"

#include <TaskButton.h>
#include <TaskCAN.h>
#include <TaskErrorLed.h>
#include <TaskStepperTMC2208.h>

#include <EEPROM.h>
#include <InterpolationLib.h>
#include <StoredLUT.h>

#include <GyverFilters.h>

Scheduler taskManager;

// hardware speficics
const int BUTTON_PORT = 7;
const int LED_PORT = 3;
const int MCP2515_SPI_PORT = 10;
const int MCP2515_INT_PIN = 2;

const int STEPPER_STEP = A0;
const int STEPPER_DIR = A1;
const int STEPPER_RESET = A2;

const int kEEPROMAddrIndex = 0;
const int kEEPROMLUTIndex = 5;

void initSerial()
{
    Serial.begin(9600);
    while (!Serial)
        ;
}

void onButtonPressed(bool pressed)
{
    if (pressed)
        TaskErrorLed::instance().addError(TaskErrorLed::ERROR_TEST_LED);
    else
        TaskErrorLed::instance().removeError(TaskErrorLed::ERROR_TEST_LED);
}

StoredLUT<kEEPROMLUTIndex> s_lut;

void onSetValue(byte priority, byte port, uint16_t srcAddress, uint16_t dstAddress, byte len, byte* payload, void* data)
{
    if (len != 4)
        {
            return;
        }

    float pos = *reinterpret_cast<float*>(payload);

    TaskStepperTMC2208::instance().setPosition(
        static_cast<uint16_t>(Interpolation::ConstrainedSpline(s_lut.x(), s_lut.y(), s_lut.size(), pos)));
}

void onPosCommand(int32_t pos)
{
    Serial.println(pos);
    TaskStepperTMC2208::instance().setPosition(pos);
}

void onLPosCommand(float pos)
{
    uint16_t hwPos = static_cast<uint16_t>(Interpolation::ConstrainedSpline(s_lut.x(), s_lut.y(), s_lut.size(), pos));
    Serial.print(hwPos);
    TaskStepperTMC2208::instance().setPosition(hwPos);
}

void onInteractiveCommand(int16_t delta)
{
    TaskStepperTMC2208& stp = TaskStepperTMC2208::instance();
    int32_t newPos = stp.position() + delta;
    stp.setPosition(newPos);
    Serial.println(newPos);
}

uint16_t onSimAddrCommand(uint16_t addr)
{
    if (addr == 0) return TaskCAN::instance().simAddress();

    // write to eeprom
    EEPROM.put(kEEPROMAddrIndex, addr);
    TaskCAN::instance().setSimAddress(addr);
    return addr;
}

void loopTest2()
{
    pinMode(A5, INPUT);
    int resRaw = analogRead(A5);
    // Serial.println(resRaw);
}
void loopKnob()
{
    pinMode(A4, INPUT);
    int resRaw = analogRead(A4);

    //Serial.println(resRaw);

    // 31.0 -> 0
    // 28 -> 820

    float p = (resRaw - 0) * (28.0 - 31.0) / (820 - 0) + 31.0;

    float p0 = 29.92;
    // float p = 31;
    // float h = 44330 * (1 - pow(p / p0, (1 / 5.255))) / 3.28084;

    // https://en.wikipedia.org/wiki/Pressure_altitude
    float h = 145366.45 * (1 - pow(p / p0, 0.190284));
    // Serial.println(h);

    int posRaw = h * 16 * 200 / 1000;

    // exp filter
    static GFilterRA filter(0.01);
    int pos = filter.filtered(posRaw);

    // int32_t pos = TaskStepperTMC2208::instance().position();
    TaskStepperTMC2208::instance().setPosition(pos);
    //Serial.println(p);
}
Task taskKnob(1 * TASK_MILLISECOND, TASK_FOREVER, &loopKnob, &taskManager, false);

bool initSensor();

Task taskTest(TASK_IMMEDIATE, TASK_FOREVER, nullptr, &taskManager, false, initSensor);
Task taskTest2(500 * TASK_MILLISECOND, TASK_FOREVER, &loopTest2, &taskManager, true);

float readSensorFiltered()
{
    int res_raw = analogRead(A3);
    static GFilterRA filter(0.8);
    int res = filter.filtered(res_raw);

    return res * 5.0 / 1023.0;
}

void sensorFinalize()
{
    if (TaskStepperTMC2208::instance().position() == TaskStepperTMC2208::instance().targetPosition())
    {
        TaskStepperTMC2208::instance().resetPosition(0);
        taskTest.disable();
        taskKnob.enable();
    }
}

void sensorFinalizeInit()
{
    Serial.println("sensorFinalizeInit");
    // TaskStepperTMC2208::instance().setSpeed(2500);
    TaskStepperTMC2208::instance().setPosition(TaskStepperTMC2208::instance().position() + 11991 + 19 + 28 - 1018 -
                                               100 - 9 - 48);
    taskTest.setCallback(sensorFinalize);
}

void sensorFindMarkerFast()
{
    if (readSensorFiltered() > 2) taskTest.setCallback(sensorFinalizeInit);
}

void sensorFindMarkerFastInit()
{
    Serial.println("sensorFindMarkerFastInit");
    TaskStepperTMC2208::instance().setPosition(TaskStepperTMC2208::instance().position() -
                                               200L * 10 * 10 * 16);  // full circle
    taskTest.setCallback(sensorFindMarkerFast);
}

void sensorMoveAwayFromMarker()
{
    if (readSensorFiltered() < 1) taskTest.setCallback(sensorFindMarkerFastInit);
}

void sensorMoveAwayFromMarkerInit()
{
    Serial.println("sensorMoveAwayFromMarkerInit");
    TaskStepperTMC2208::instance().setPosition(TaskStepperTMC2208::instance().position() -
                                               200L * 10 * 10 * 16);  // full circle
    taskTest.setCallback(sensorMoveAwayFromMarker);
}

bool initSensor()
{
    Serial.println("initSensor");
    pinMode(A3, INPUT);
    taskTest.setCallback(sensorMoveAwayFromMarkerInit);
    taskTest.enable();

    return true;
}

void setup()
{
    initSerial();

    TaskErrorLed::init(taskManager, LED_PORT);
    TaskStepperTMC2208::init(taskManager, STEPPER_STEP, STEPPER_DIR, STEPPER_RESET, 100, 100);
    TaskButton::init(taskManager, BUTTON_PORT);
    TaskButton::instance().setPressedCallback(onButtonPressed);
    TaskAltimeterMenu::init(taskManager);
    TaskAltimeterMenu::instance().setPosCallback(onPosCommand);
    TaskAltimeterMenu::instance().setInteractiveCallback(onInteractiveCommand);
    TaskMenu::instance().setSimAddressCallback(onSimAddrCommand);
    
    uint16_t kSimAddress = 0;

    EEPROM.get(kEEPROMAddrIndex, kSimAddress);
    if (kSimAddress == 65535)
    {
        kSimAddress = 1023;
        EEPROM.put(kEEPROMAddrIndex, kSimAddress);
    }

    TaskCAN::init(taskManager, MCP2515_SPI_PORT, MCP2515_INT_PIN, kSimAddress);

    // port 0: set arrow position
    TaskCAN::instance().setReceiveCallback(onSetValue, 0);

    // set some fake LUT
    bool hasLut = s_lut.load();

    if (!hasLut)
    {
        s_lut.clear();

        s_lut.addValue(0.0, 0.0);
        s_lut.addValue(40, 380);
        s_lut.addValue(60, 928);
        s_lut.addValue(80, 1417);
        s_lut.addValue(100, 1869);
        s_lut.addValue(200, 3657);
    }

    TaskErrorLed::instance().start();
    TaskStepperTMC2208::instance().start();
    TaskButton::instance().start();
    TaskCAN::instance().start();
    TaskMenu::instance().start();

    taskTest.enable();
}

void loop()
{
    taskManager.execute();
}
