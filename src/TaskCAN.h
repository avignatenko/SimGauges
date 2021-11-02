#pragma once

#include "Common.h"

class MCP_CAN;

class TaskCAN
{
public:
    static void init(Scheduler& sh, byte spiPort, byte intPort);

    static TaskCAN& instance();

    void start();

private:
    TaskCAN(Scheduler& sh, byte spiPort, byte intPort);

    void loopCANReceiveCallback();
    static void loopCANReceiveStatic();

    void loopCANCheckCallback();
    static void loopCANCheckStatic();

private:
    static TaskCAN* instance_;
    Task taskCANReceive_;
    Task taskCANCheckError_;
    MCP_CAN* mcpCAN_;
};