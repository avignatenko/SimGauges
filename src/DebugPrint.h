#pragma once

#define MPG_PENDANT_DEBUG_PRINT

#ifdef MPG_PENDANT_DEBUG_PRINT
#define debugPrint(...) Serial.print(__VA_ARGS__);
#define debugPrintln(...) Serial.println(__VA_ARGS__);
#else
#define debugPrint(...)
#define debugPrintln(...)
#endif
