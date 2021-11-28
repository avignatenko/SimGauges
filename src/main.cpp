
#include <Common.h>

#include "TaskMenu.h"

#include <TaskButton.h>
#include <TaskCAN.h>
#include <TaskErrorLed.h>
#include <TaskStepperX27Driver.h>

#include <InterpolationLib.h>

Scheduler taskManager;

// hardware speficics
const int BUTTON_PORT = 7;
const int LED_PORT = 3;
const int MCP2515_SPI_PORT = 10;
const int MCP2515_INT_PIN = 2;

const int STEPPER_STEP = A0;
const int STEPPER_DIR = A1;
const int STEPPER_RESET = A2;

void initSerial()
{
    Serial.begin(115200);
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

class StoredLUT
{
public:
    StoredLUT() {}

    ~StoredLUT() {}

    void resize(byte size)
    {
        clear();
        create(size);

        size_ = size;
    }

    byte size() const { return size_; }

    double* x() { return xValues_; }
    double* y() { return yValues_; }

private:
    void create(byte size)
    {
        xValues_ = new double[size];
        yValues_ = new double[size];
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
    uint16_t newPos = constrain(stp.position() + delta, 0, stp.totalSteps() - 1);
    stp.setPosition(newPos);
    Serial.println(newPos);
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

    const uint16_t kSimAddress = 16;
    TaskCAN::init(taskManager, MCP2515_SPI_PORT, MCP2515_INT_PIN, kSimAddress);

    // port 0: set arrow position
    TaskCAN::instance().setReceiveCallback(onSetValue, 0);

    // set some fake LUT
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
    s_lut.y()[4] = 1846;

    s_lut.x()[5] = 200;
    s_lut.y()[5] = 3657;


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
