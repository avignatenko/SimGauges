#include "TaskCalibrate.h"

#include <GyverFilters.h>
#include <TaskStepperTMC2208.h>

TaskCalibrate::TaskCalibrate(TaskStepperTMC2208& stepper, int32_t calibration,
                             unsigned long aInterval, long aIterations, Scheduler* aScheduler,
                             bool aEnable)
    : Task(aInterval, aIterations, aScheduler, aEnable), stepper_(stepper), calibration_(calibration)
{
}

bool TaskCalibrate::Callback()
{
    return (this->*callback_)();
}

void TaskCalibrate::setCalibration(int32_t calibration)
{
    calibration_ = calibration;
}

void TaskCalibrate::start()
{
    enable();
}

bool TaskCalibrate::OnEnable()
{
    stepper_.setSpeed(2500);
    stepper_.setAcceleration(5000);
    return true;
}

void TaskCalibrate::OnDisable()
{
    stepper_.setSpeed(7000);
    stepper_.setAcceleration(8000);
}

bool TaskCalibrate::initSensor()
{
    Serial.println("initSensor");
    pinMode(A3, INPUT);

    callback_ = &TaskCalibrate::sensorMoveAwayFromMarkerInit;

    return true;
}

float TaskCalibrate::readSensorFiltered()
{
    int res_raw = analogRead(A3);
    static GFilterRA filter(0.8);
    int res = filter.filtered(res_raw);

    return res * 5.0 / 1023.0;
}

bool TaskCalibrate::sensorFinalize()
{
    if (stepper_.position() == stepper_.targetPosition())
    {
        stepper_.resetPosition(0);
        disable();
    }

    return true;
}

bool TaskCalibrate::sensorFinalizeInit()
{
    Serial.println("sensorFinalizeInit");
    // 10863
    stepper_.setPosition(stepper_.position() + calibration_);

    callback_ = &TaskCalibrate::sensorFinalize;

    return true;
}

bool TaskCalibrate::sensorFindMarkerFast()
{
    if (readSensorFiltered() > 2) callback_ = &TaskCalibrate::sensorFinalizeInit;

    return true;
}

bool TaskCalibrate::sensorFindMarkerFastInit()
{
    Serial.println("sensorFindMarkerFastInit");
    stepper_.setPosition(stepper_.position() - 200L * 10 * 10 * 16);  // full circle
    callback_ = &TaskCalibrate::sensorFindMarkerFast;

    return true;
}

bool TaskCalibrate::sensorMoveAwayFromMarker()
{
    if (readSensorFiltered() < 1) callback_ = &TaskCalibrate::sensorFindMarkerFastInit;

    return true;
}

bool TaskCalibrate::sensorMoveAwayFromMarkerInit()
{
    Serial.println("sensorMoveAwayFromMarkerInit");
    stepper_.setPosition(stepper_.position() - 200L * 10 * 10 * 16);  // full circle

    callback_ = &TaskCalibrate::sensorMoveAwayFromMarker;

    return true;
}
