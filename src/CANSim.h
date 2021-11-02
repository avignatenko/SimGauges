#pragma once

#include <Common.h>

#include <Array.h>

// CAM-Sim message format
// CAN id (high bit -> low bit, total 29 bits, only extended format supported):
// 4 bits: (25 .. 28) priority (0 .. 15)
// 5 bits: (20 .. 24) port
// 10 bits (9 .. 19): dst address (0 .. 1023)
// 10 bits (0 .. 9): src address (0 .. 1023)
// + CAN payload (0 - 8 bytes)
//
// Address reservation:
// 0 - unknown
// 1 - 15 master
// 16 - 1023 clients
class CANSim
{
public:
    CANSim(uint16_t thisAddress);

    void processMessage(uint32_t id, byte len, byte* buf);

    using MessageCallback = void (*)(void*, byte, byte, byte, byte, byte*);
    void addReceiveCallback(MessageCallback callback, void* data);
    void removeReceiveCallback(MessageCallback callback);

private:
    Array<MessageCallback, 10> callbacks_;
};