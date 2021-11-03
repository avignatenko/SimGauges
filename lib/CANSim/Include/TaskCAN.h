#pragma once

#include "Common.h"

#include <Array.h>

class MCP_CAN;

// CAM-Sim message format
// CAN id (high bit -> low bit, total 29 bits, only extended format supported):
// 4 bits: (25 .. 28) priority (0 .. 15, less is more priority)
// 5 bits: (20 .. 24) port
// 10 bits (9 .. 19): dst address (0 .. 1023)
// 10 bits (0 .. 9): src address (0 .. 1023)
// + CAN payload (0 - 8 bytes)
//
// Address reservation:
// 0 - unknown
// 1 - 15 master
// 16 - 1023 clients
class TaskCAN
{
public:
    static void init(Scheduler& sh, byte spiPort, byte intPort, uint16_t simaddress);
    static TaskCAN& instance();
    void start();

    void sendMessage(uint32_t idTo, byte priority, byte port, uint16_t dstAddress, byte len, byte* payload);

    using MessageCallback = void (*)(byte, byte*, void*);
    void setReceiveCallback(MessageCallback callback, byte port, void* data = nullptr);

private:
    TaskCAN(Scheduler& sh, byte spiPort, byte intPort, uint16_t simaddress);

    void loopCANReceiveCallback();
    static void loopCANReceiveStatic();

    void loopCANCheckCallback();
    static void loopCANCheckStatic();

    void parseBuffer(uint32_t id, byte len, byte* buffer);

private:
    static TaskCAN* instance_;
    Task taskCANReceive_;
    Task taskCANCheckError_;
    MCP_CAN* mcpCAN_;
    int intPort_;
    uint16_t simaddress_;  // 0 .. 1023

    struct CallBack
    {
        MessageCallback callback = nullptr;
        void* data = nullptr;
    };

    Array<CallBack, 32> callbacks_;
};