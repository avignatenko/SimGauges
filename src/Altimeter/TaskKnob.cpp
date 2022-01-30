#include "TaskKnob.h"

#include <GyverFilters.h>
#include <Interpolation.h>
#include <StoredLUT.h>
#include <TaskStepperTMC2208.h>

TaskKnob::TaskKnob(TaskStepperTMC2208& stepper, unsigned long aInterval = 0, long aIterations = 0,
                   Scheduler* aScheduler = NULL, bool aEnable = false)
    : Task(aInterval, aIterations, aScheduler, aEnable), stepper_(stepper)
{
}

void TaskKnob::setLUT(StoredLUT* lut)
{
    lut_ = lut;
}
int TaskKnob::knobValue()
{
    pinMode(A4, INPUT);
    int resRaw = analogRead(A4);
    // exp filter
    static GFilterRA filter(0.01);
    int res = filter.filtered(resRaw);
    return res;
}

int TaskKnob::pos()
{
    float p = cubicInterpolate<double, double>(lut_->x(), lut_->y(), lut_->size(), knobValue());

    // https://en.wikipedia.org/wiki/Pressure_altitude
    float h = 145366.45 * (1 - pow(p / pressure_, 0.190284));
    int pos = -h * 16 * 200 / 1000;

    return pos;
}

void TaskKnob::setStepperLink(bool link)
{
    stepperLink_ = link;
}

void TaskKnob::setPressure(float pressure)
{
    pressure_ = pressure;
}

bool TaskKnob::Callback()
{
    if (stepperLink_)
        stepper_.setPosition(pos());
    else
        knobValue();
    return true;
}

void TaskKnob::start()
{
    enable();
}

Task& TaskKnob::task()
{
    return *this;
}