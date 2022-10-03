#pragma once

#include <Common.h>
#include <FastDelegate.h>

class TaskStepperTMC2208;
class StoredLUT;

class TaskKnob : private Task
{
public:
    TaskKnob(unsigned long aInterval = 0, long aIterations = 0, Scheduler* aScheduler = NULL, bool aEnable = false);

    void setLUT(StoredLUT* lut);
    float pressure();
    int32_t knobValue();
    void setPressureCallback(fastdelegate::FastDelegate1<float> callback);

    void start();

    void forceUpdate();
private:
    virtual bool Callback() override;
    Task& task();

private:
    StoredLUT* lut_ = nullptr;
    fastdelegate::FastDelegate1<float> pressureCallback_;
    float pressure_ = -1.0;
};