#pragma once

#include <Arduino.h>

#define _TASK_MICRO_RES
#define _TASK_INLINE
#define _TASK_TIMECRITICAL
#include <TaskScheduler.h>

//#define DISABLE_LOGGING
#define LOGLEVEL LOG_LEVEL_NOTICE
//#define LOGLEVEL LOG_LEVEL_VERBOSE
#include <ArduinoLog.h>