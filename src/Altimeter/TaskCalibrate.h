#pragma once

#include <Common.h>

class TaskStepperTMC2208;

class TaskCalibrate : private Task
{
public:
    TaskCalibrate(TaskStepperTMC2208& stepper, Task& taskKnob, int32_t calibration, unsigned long aInterval = 0,
                  long aIterations = 0, Scheduler* aScheduler = NULL, bool aEnable = false);

    virtual bool Callback() override;

    void setCalibration(int32_t calibration);

    void start();

private:
    virtual bool OnEnable() override;
    virtual void OnDisable() override;

    bool initSensor();

    float readSensorFiltered();

    bool sensorFinalize();
    bool sensorFinalizeInit();
    bool sensorFindMarkerFast();
    bool sensorFindMarkerFastInit();
    bool sensorMoveAwayFromMarker();
    bool sensorMoveAwayFromMarkerInit();

private:
    TaskStepperTMC2208& stepper_;
    Task& taskKnob_;
    bool (TaskCalibrate::*callback_)() = &TaskCalibrate::initSensor;
    int32_t calibration_;
};
