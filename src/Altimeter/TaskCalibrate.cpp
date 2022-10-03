#include "TaskCalibrate.h"

#include <GyverFilters.h>
#include <TaskStepperTMC2208.h>

TaskCalibrate::TaskCalibrate(TaskStepperTMC2208& stepper, int32_t calibration, unsigned long aInterval,
                             long aIterations, Scheduler* aScheduler, bool aEnable)
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
    stepper_.resetPosition(0);
    return true;
}

void TaskCalibrate::OnDisable()
{
    stepper_.setSpeed(7000);
    stepper_.setAcceleration(8000);
}

bool TaskCalibrate::initSensor()
{
   //Serial.println("initSensor");
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

float TaskCalibrate::readSensor2Filtered()
{
    int res_raw = analogRead(A5);
    static GFilterRA filter(0.8);
    int res = filter.filtered(res_raw);

    return res * 5.0 / 1023.0;
}

bool TaskCalibrate::sensorFinalize()
{
    if (stepper_.position() == stepper_.targetPosition())
    {
        // stepper_.resetPosition(0);
        disable();
        if (finishedCallback_) finishedCallback_();
    }

    return true;
}

bool TaskCalibrate::sensorFinalizeInit()
{
   // Serial.println("sensorFinalizeInit");

    Serial.print("calibration pos: ");
    Serial.println(stepper_.position());
    
    stepper_.resetPosition(calibration_);
    stepper_.setPosition(0);

    callback_ = &TaskCalibrate::sensorFinalize;

    return true;
}


bool TaskCalibrate::sensorFindMarkerFastInit()
{
    stepper_.setSpeed(1500);
    stepper_.setAcceleration(4000);
    //Serial.println("sensorFindMarkerFastInit");
    stepper_.setPosition(stepper_.position() + 200L * 10 * 10 * 16);  // full circle
    callback_ = &TaskCalibrate::sensorFindMarkerFast;

    return true;
}

bool TaskCalibrate::sensorFindMarkerFast()
{
    float sensor = readSensorFiltered();

    // Serial.print("sn 2: ");
    // Serial.println(sensor);

    if (sensor > 2)
    {
        stepper_.stop();
        callback_ = &TaskCalibrate::sensorFindMarketFastFinalize;
    }

    return true;
}

bool TaskCalibrate::sensorFindMarketFastFinalize()
{
    if (!stepper_.isRunning()) callback_ = &TaskCalibrate::sensorFindMarker2Init;
}

bool TaskCalibrate::sensorFindMarker2Init()
{
    stepper_.setSpeed(1500);
    stepper_.setAcceleration(4000);
    Serial.println("sensorFindMarker2Init");
    stepper_.setPosition(stepper_.position() + 200L * 16 * 2);  // full circle
    callback_ = &TaskCalibrate::sensorFindMarker2;

    return true;
}

bool TaskCalibrate::sensorFindMarker2()
{
    float sensor = readSensor2Filtered();

    //Serial.print("sn 2: ");
    //Serial.println(sensor);

    if (sensor > 2)
    {
        stepper_.stop();
        callback_ = &TaskCalibrate::sensorFindMarket2Finalize;
    }

    return true;
}

bool TaskCalibrate::sensorFindMarket2Finalize()
{
    if (!stepper_.isRunning()) callback_ = &TaskCalibrate::sensorFinalizeInit;
}

bool TaskCalibrate::sensorMoveAwayFromMarker()
{
    float sensor = readSensorFiltered();

    if (sensor < 1) callback_ = &TaskCalibrate::sensorFindMarkerFastInit;

    return true;
}

bool TaskCalibrate::sensorMoveAwayFromMarkerInit()
{
    //Serial.println("sensorMoveAwayFromMarkerInit");
    stepper_.setPosition(stepper_.position() - 200L * 10 * 10 * 16);  // full circle

    callback_ = &TaskCalibrate::sensorMoveAwayFromMarker;

    return true;
}
