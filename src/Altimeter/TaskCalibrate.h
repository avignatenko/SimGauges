#pragma once

#include <Common.h>
#include <FastDelegate.h>

class TaskStepperTMC2208;

class TaskCalibrate : private Task
{
public:
    TaskCalibrate(TaskStepperTMC2208& stepper, int32_t calibration, unsigned long aInterval = 0, long aIterations = 0,
                  Scheduler* aScheduler = NULL, bool aEnable = false);

    virtual bool Callback() override;

    void setCalibration(int32_t calibration);

    void start();

    using Task::isEnabled;

   void setFinishedCallback(fastdelegate::FastDelegate0<void> callback) {finishedCallback_  = callback;}

private:
    virtual bool OnEnable() override;
    virtual void OnDisable() override;

    bool initSensor();

    float readSensorFiltered();
    float readSensor2Filtered();

    bool sensorFinalize();
    bool sensorFinalizeInit();
    bool sensorFindMarkerFast();
    bool sensorFindMarkerFastInit();
    bool sensorFindMarketFastFinalize();
    bool sensorMoveAwayFromMarker();
    bool sensorMoveAwayFromMarkerInit();

    bool sensorFindMarker2();
    bool sensorFindMarker2Init();
    bool sensorFindMarket2Finalize();

private:
    TaskStepperTMC2208& stepper_;
    bool (TaskCalibrate::*callback_)() = &TaskCalibrate::initSensor;
    int32_t calibration_;

    fastdelegate::FastDelegate0<void> finishedCallback_;
};
