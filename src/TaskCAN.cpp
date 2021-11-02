#include "TaskCAN.h"

#include "TaskErrorLed.h"

#include <mcp_can.h>

void TaskCAN::loopCANReceiveStatic()
{
    TaskCAN& me = TaskCAN::instance();
    me.loopCANReceiveCallback();
}

void TaskCAN::loopCANReceiveCallback()
{
    if (mcpCAN_->checkReceive() == CAN_MSGAVAIL)
    {
        static byte len = 0;
        static byte buf[8];
        // check if data coming
        mcpCAN_->readMsgBuf(&len, buf);  // read data,  len: data length, buf: data buf

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

    if (error != 0 && !(TaskErrorLed::instance().error() & TaskErrorLed::ERROR_CAN))
    {
        Log.errorln("CAN error: %d", error);
        TaskErrorLed::instance().addError(TaskErrorLed::ERROR_CAN);
    }
}

void TaskCAN::loopCANCheckStatic()
{
    TaskCAN& me = TaskCAN::instance();
    me.loopCANCheckCallback();
}

TaskCAN* TaskCAN::instance_ = nullptr;

TaskCAN::TaskCAN(Scheduler& sh, byte spiPort, byte intPort)
    : taskCANReceive_(TASK_IMMEDIATE, TASK_FOREVER, &loopCANReceiveStatic, &sh, false),
      taskCANCheckError_(1000 * TASK_MILLISECOND, TASK_FOREVER, &loopCANCheckStatic, &sh, false),
      mcpCAN_(new MCP_CAN(spiPort))

{
    Log.traceln("TaskCAN::TaskCAN()");
}

void TaskCAN::init(Scheduler& sh, byte spiPort, byte intPort)
{
    instance_ = new TaskCAN(sh, spiPort, intPort);
}

void TaskCAN::start()
{
    for (int i = 0; i < 5; ++i)
    {
        if (CAN_OK == mcpCAN_->begin(CAN_500KBPS))  // init can bus : baudrate = 500k
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

    taskCANReceive_.enable();
    taskCANCheckError_.enable();
}
TaskCAN& TaskCAN::instance()
{
    return *instance_;
}
