
#include <Common.h>

#include "TaskMenu.h"

#include <TaskButton.h>
#include <TaskCAN.h>
#include <TaskErrorLed.h>
#include <TaskStepperX27Driver.h>

#include <InterpolationLib.h>

#include <EEPROM.h>

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

class StoredLUT : public Printable
{
public:
    StoredLUT() {}

    ~StoredLUT() {}

    void resize(byte size)
    {
        clear();
        create(size);
    }

    byte size() const { return size_; }

    double* x() { return xValues_; }
    double* y() { return yValues_; }

    size_t printTo(Print& p) const override
    {
        size_t r = 0;

        for (int i = 0; i < size_; ++i)
        {
            if (i > 0) r += p.println();
            r += p.print(xValues_[i]);
            r += p.print(' ');
            r += p.print(yValues_[i]);
        }
        return r;
    }

    bool load()
    {
        int idx = kEEPROMLUTIndex;

        byte size = 255;
        EEPROM.get(idx, size);
        if (size == 255) return false;

        resize(size);

        idx += sizeof(size_);
        for (int i = 0; i < size_; ++i)
        {
            EEPROM.get(idx, xValues_[i]);
            idx += sizeof(xValues_[i]);

            EEPROM.get(idx, yValues_[i]);
            idx += sizeof(yValues_[i]);
        }

        return true;
    }

    void save()
    {
        int idx = kEEPROMLUTIndex;
        EEPROM.put(idx, size_);
        idx += sizeof(size_);
        for (int i = 0; i < size_; ++i)
        {
            EEPROM.put(idx, xValues_[i]);
            idx += sizeof(xValues_[i]);

            EEPROM.put(idx, yValues_[i]);
            idx += sizeof(yValues_[i]);
        }
    }

private:
    void create(byte size)
    {
        xValues_ = new double[size];
        yValues_ = new double[size];
        size_ = size;
    }
    void clear()
    {
        delete[] xValues_;
        xValues_ = nullptr;
        delete[] yValues_;
        yValues_ = nullptr;

        size_ = 0;
    }

private:
    double* xValues_ = nullptr;
    double* yValues_ = nullptr;
    byte size_ = 0;
};

StoredLUT s_lut;

void onSetValue(byte priority, byte port, uint16_t srcAddress, uint16_t dstAddress, byte len, byte* payload, void* data)
{
    Log.traceln("onSetValue");
    if (len != 4)
    {
        Log.errorln("onSetValue: wrong len");
        return;
    }

    float pos = *reinterpret_cast<float*>(payload);
    Log.verboseln("Value: %f", pos);
    TaskStepperX27Driver::instance().setPosition(
        static_cast<uint16_t>(Interpolation::ConstrainedSpline(s_lut.x(), s_lut.y(), s_lut.size(), pos)));
}

void onPosCommand(uint16_t pos)
{
    TaskStepperX27Driver::instance().setPosition(pos);
}

void onLPosCommand(float pos)
{
    uint16_t hwPos = static_cast<uint16_t>(Interpolation::ConstrainedSpline(s_lut.x(), s_lut.y(), s_lut.size(), pos));
    Serial.print(hwPos);
    TaskStepperX27Driver::instance().setPosition(hwPos);
}

void onInteractiveCommand(uint16_t delta)
{
    TaskStepperX27Driver& stp = TaskStepperX27Driver::instance();
    uint16_t newPos = stp.position() + delta;
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
    }

    if (cmd == TaskMenu::LUTCommand::Load)
    {
        s_lut.load();
        Serial.println("OK");
    }

    if (cmd == TaskMenu::LUTCommand::Save)
    {
        s_lut.save();
        Serial.println("OK");
    }

    if (cmd == TaskMenu::LUTCommand::Clear)
    {
        s_lut.resize(0);
        Serial.println("OK");
    }
}

void setup()
{
    initSerial();

    Log.begin(LOGLEVEL, &Serial);
    Log.infoln("Started. Serial OK");

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
        s_lut.resize(6);
        s_lut.x()[0] = 0.0;
        s_lut.y()[0] = 0.0;

        s_lut.x()[1] = 40;
        s_lut.y()[1] = 380;

        s_lut.x()[2] = 60;
        s_lut.y()[2] = 928;

        s_lut.x()[3] = 80;
        s_lut.y()[3] = 1417;

        s_lut.x()[4] = 100;
        s_lut.y()[4] = 1869;

        s_lut.x()[5] = 200;
        s_lut.y()[5] = 3657;
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
