#include "TaskKnob.h"

#include <GyverFilters.h>
#include <Interpolation.h>
#include <StoredLUT.h>

TaskKnob::TaskKnob(unsigned long aInterval = 0, long aIterations = 0, Scheduler* aScheduler = NULL,
                   bool aEnable = false)
    : Task(aInterval, aIterations, aScheduler, aEnable)
{
}

void TaskKnob::setLUT(StoredLUT* lut)
{
    lut_ = lut;
}

int32_t TaskKnob::knobValue()
{
    pinMode(A4, INPUT);
    int resRaw = analogRead(A4);
    // exp filter
    static GFilterRA filter(0.1);
    int res = filter.filtered(resRaw);
    return res;
}

float TaskKnob::pressure()
{
    return catmullSplineInterpolate<double, double>(lut_->x(), lut_->y(), lut_->size(), knobValue());
}

bool TaskKnob::Callback()
{
    const float kThreshold = 0.01;  // 0.01 inHg
    float lastPressure = pressure_;
    float newPressure = pressure();
    if (fabs(lastPressure - newPressure) > kThreshold)
    {
        pressure_ = newPressure;
        if (pressureCallback_) pressureCallback_(newPressure);
    }
    return true;
}

void TaskKnob::forceUpdate()
{
     if (pressureCallback_) pressureCallback_(pressure());
}

void TaskKnob::start()
{
    enable();
}

void TaskKnob::setPressureCallback(fastdelegate::FastDelegate1<float> callback)
{
    pressureCallback_ = callback;
}
