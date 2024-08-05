#ifndef TRANSFERTASK_H
#define TRANSFERTASK_H

#include <string>
#include "taskscheduleshowd.h"
using namespace std;
class TransferTask
{
public:
    TransferTask();
    //传输任务初始化
    virtual void transferInit(string taskType,string transfertasktype,string transfertype,string sender,
                              string reciver,size_t offset,string length)=0;
    //任务执行
    virtual void taskExecution()=0;

    //获得传输级别(是超级任务还是普通任务)
    //超级传输任务(SuperTransferTask) 超级传输任务子任务(SuperTransferSubtask) 普通传输任务(NormalTransferTask)
    virtual string getTransferTaskType()=0;

    //获取任务类型
    virtual string getTaskType()=0;

    //获取传输类型
    virtual string getTransferType()=0;

    //获取传送者
    virtual string getTransferSender()=0;

    //获取接收者
    virtual string getTransferReciver()=0;

    //获取传输字节数
    virtual string getTransferLength()=0;

    virtual ~TransferTask()=default;
};

#endif // TRANSFERTASK_H
