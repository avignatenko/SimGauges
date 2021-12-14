
#include <Common.h>

#include "TaskMenu.h"

#include <TaskButton.h>
#include <TaskCAN.h>
#include <TaskErrorLed.h>
#include <TaskStepperX27Driver.h>

#include <EEPROM.h>
#include <InterpolationLib.h>
#include <StoredLUT.h>

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

    TaskStepperX27Driver::instance().setPosition(
        static_cast<uint16_t>(Interpolation::ConstrainedSpline(s_lut.x(), s_lut.y(), s_lut.size(), pos)));
}

void onPosCommand(int16_t pos)
{
    TaskStepperX27Driver::instance().setPosition(pos);
}

void onLPosCommand(float pos)
{
    uint16_t hwPos = static_cast<uint16_t>(Interpolation::ConstrainedSpline(s_lut.x(), s_lut.y(), s_lut.size(), pos));
    Serial.print(hwPos);
    TaskStepperX27Driver::instance().setPosition(hwPos);
}

void onInteractiveCommand(int16_t delta)
{
    TaskStepperX27Driver& stp = TaskStepperX27Driver::instance();
    int16_t newPos = constrain(stp.position() + delta, 0, stp.totalSteps() - 1);
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

void onLUTCommand(TaskMenu::LUTCommand cmd, float posl, int16_t pos)
{
    if (cmd == TaskMenu::LUTCommand::Show)
    {
        Serial.println(s_lut);
        return;
    }

    if (cmd == TaskMenu::LUTCommand::Load)
    {
        s_lut.load();
        Serial.println(s_lut);
        Serial.println("OK");
        return;
    }

    if (cmd == TaskMenu::LUTCommand::Save)
    {
        s_lut.save();
        Serial.println("OK");
        return;
    }

    if (cmd == TaskMenu::LUTCommand::Clear)
    {
        s_lut.clear();
        Serial.println("OK");
        return;
    }

    if (cmd == TaskMenu::LUTCommand::Set)
    {
        if (pos < 0) pos = TaskStepperX27Driver::instance().position();

        if (s_lut.size() == s_lut.maxSize())
        {
            Serial.println("Error: max LUT capacity reached");
            return;
        }

        s_lut.addValue(posl, pos);

        Serial.print("Set ");
        Serial.print(posl);
        Serial.print("->");
        Serial.print(pos);
        Serial.println(" OK");

        Serial.println(s_lut);
        return;
    }

    if (cmd == TaskMenu::LUTCommand::Remove)
    {
        s_lut.removeValue(posl);

        Serial.print("Remove ");
        Serial.print(posl);
        Serial.println(" OK");

        Serial.println(s_lut);
        return;
    }
}

void setup()
{
    initSerial();

    TaskErrorLed::init(taskManager, LED_PORT);
    TaskStepperX27Driver::init(taskManager, STEPPER_STEP, STEPPER_DIR, STEPPER_RESET);
    TaskButton::init(taskManager, BUTTON_PORT);
    TaskButton::instance().setPressedCallback(onButtonPressed);
    TaskMenu::init(taskManager);
    TaskMenu::instance().setPosCallback(onPosCommand);
    TaskMenu::instance().setInteractiveCallback(onInteractiveCommand);
    TaskMenu::instance().setLPosCallback(onLPosCommand);
    TaskMenu::instance().setSimAddressCallback(onSimAddrCommand);
    TaskMenu::instance().setLUTCallback(onLUTCommand);

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
    TaskStepperX27Driver::instance().start();
    TaskButton::instance().start();
    TaskCAN::instance().start();
    TaskMenu::instance().start();
}

void loop()
{
    taskManager.execute();
}
