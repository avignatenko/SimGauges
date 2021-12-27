#pragma once

#include <Common.h>

class TaskStepperTMC2208;
class StoredLUT;

class TaskKnob : private Task
{
public:
    TaskKnob(TaskStepperTMC2208& stepper, unsigned long aInterval = 0, long aIterations = 0,
             Scheduler* aScheduler = NULL, bool aEnable = false);

    void setLUT(StoredLUT* lut);
    int knobValue();

    int pos();

    void setStepperLink(bool link);
    void setPressure(float pressure);
    virtual bool Callback() override;
    void start();

    Task& task();

private:
    TaskStepperTMC2208& stepper_;
    bool stepperLink_ = false;
    StoredLUT* lut_ = nullptr;
    float pressure_ = 29.92;
};