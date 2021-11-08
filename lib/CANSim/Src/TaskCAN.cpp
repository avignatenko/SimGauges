#include "TaskCAN.h"

#include "TaskErrorLed.h"

#include "mcp_can.h"

void TaskCAN::loopCANReceiveStatic()
{
    TaskCAN& me = TaskCAN::instance();
    me.loopCANReceiveCallback();
}

void TaskCAN::loopCANReceiveCallback()
{
    if (!digitalRead(intPort_))
    {
        static byte len = 0;
        static byte buf[8];
        // check if data coming
        uint32_t id;
        int result = mcpCAN_->readMsgBuf(&id, &len, buf);  // read data,  len: data length, buf: data buf
        if (result != CAN_OK)
        {
            Log.warningln("Can failed read buf");
            return;
        }

        parseBuffer(id & 0x1FFFFFFF, len, buf);
#ifndef DISABLE_LOGGING
        if (Log.getLevel() >= LOG_LEVEL_VERBOSE)
        {
            for (int i = 0; i < len; i++)  // print the data
            {
                Serial.print(buf[i]);
                Serial.print("\t");
            }
            Serial.println();
        }
    }
#endif
}

void TaskCAN::loopCANCheckCallback()
{
    Log.traceln("TaskCAN::loopCANCheckCallback()");

    int error = mcpCAN_->checkError();

    if (error != 0)
    {
        Log.errorln("CAN error: %d", error);
        TaskErrorLed::instance().addError(TaskErrorLed::ERROR_CAN);
    }
    else
    {
        Log.verboseln("CAN no error");
        TaskErrorLed::instance().removeError(TaskErrorLed::ERROR_CAN);
    }
}

void TaskCAN::loopCANCheckStatic()
{
    TaskCAN& me = TaskCAN::instance();
    me.loopCANCheckCallback();
}

TaskCAN* TaskCAN::instance_ = nullptr;

TaskCAN::TaskCAN(Scheduler& sh, byte spiPort, byte intPort, uint16_t simaddress)
    : taskCANReceive_(TASK_IMMEDIATE, TASK_FOREVER, &loopCANReceiveStatic, &sh, false),
      taskCANCheckError_(1000 * TASK_MILLISECOND, TASK_FOREVER, &loopCANCheckStatic, &sh, false),
      mcpCAN_(new MCP_CAN(spiPort)),
      simaddress_(simaddress),
      intPort_(intPort)

{
    Log.traceln("TaskCAN::TaskCAN()");
}

void TaskCAN::init(Scheduler& sh, byte spiPort, byte intPort, uint16_t simaddress)
{
    instance_ = new TaskCAN(sh, spiPort, intPort, simaddress);
}

void TaskCAN::start()
{
    pinMode(intPort_, INPUT);

    for (int i = 0; i < 5; ++i)
    {
        if (CAN_OK == mcpCAN_->begin(MCP_STDEXT, CAN_500KBPS, MCP_16MHZ))  // init can bus : baudrate = 500k
        {
            Log.infoln("CAN BUS Shield init ok!");
            TaskErrorLed::instance().removeError(TaskErrorLed::ERROR_CAN);
            break;
        }
        else
        {
            Log.warningln("CAN BUS Shield init fail");
            Log.warningln("Init CAN BUS Shield again");
            TaskErrorLed::instance().addError(TaskErrorLed::ERROR_CAN);
            delay(500);
            continue;
        }
    }

    // mcpCAN_->init_Mask(0, 1, 0b11111111110000000000);  // Init first mask...
    // mcpCAN_->init_Mask(1, 1, 0b11111111110000000000);  // Init second mask...
    // mcpCAN_->init_Filt(0, 1, (simaddress_ << 10));     // Init first filter...

    mcpCAN_->setMode(MCP_NORMAL);

    taskCANReceive_.enable();
    taskCANCheckError_.enable();
}

TaskCAN& TaskCAN::instance()
{
    return *instance_;
}

void TaskCAN::sendMessage(byte priority, byte port, uint16_t dstSimAddress, byte len, byte* payload)
{
    Log.verboseln("send message priority: %d, port: %d, addr %d, len: %d", priority, port, dstSimAddress, len);
    uint32_t msg = 0;
    // 4 bits: (25 .. 28) priority (0 .. 15)
    msg |= (priority & 0b1111) << 25;
    // 5 bits: (20 .. 24) port
    msg |= (port & 0b11111) << 20;
    // 10 bits (9 .. 19): dst address (0 .. 1023)
    msg |= (dstSimAddress & 0b11111) << 9;
    // 10 bits (0 .. 9): src address (0 .. 1023)
    msg |= (simaddress_ & 0b111111) << 0;
    mcpCAN_->sendMsgBuf(msg, 1, len, payload);
}

void TaskCAN::setReceiveCallback(MessageCallback callback, byte port, void* data)
{
    CallBack& c = callbacks_[port];
    c.callback = callback;
    c.data = data;
}

void TaskCAN::parseBuffer(uint32_t id, byte len, byte* buffer)
{
    Log.verboseln("Message id: %l", id);
    // 4 bits: (25 .. 28) priority (0 .. 15)
    // 5 bits: (20 .. 24) port
    // 10 bits (9 .. 19): dst address (0 .. 1023)
    // 10 bits (0 .. 9): src address (0 .. 1023)
    uint16_t srcAddress = id & 0b1111111111;
    uint16_t dstAddress = (id >> 10) & 0b1111111111;
    if (dstAddress != simaddress_)
    {
        Log.verboseln("Wrong address: %d (we need %d)", dstAddress, simaddress_);
        return;
    }

    uint16_t port = (id >> 20) & 0b11111;
    uint16_t priority = (id >> 25) & 0b1111;

    Log.verboseln("CANSIM message: src: %d, dst: %d, port: %d, priority: %d", srcAddress, dstAddress, port, priority);

    if (port < 32 && callbacks_[port].callback != nullptr)
        callbacks_[port].callback(len, buffer, callbacks_[port].data);
}