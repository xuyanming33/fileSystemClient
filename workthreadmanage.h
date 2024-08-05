#ifndef WORKTHREADMANAGE_H
#define WORKTHREADMANAGE_H

#include <QObject>
#include <QDateTime>
#include <deque>
#include <pthread.h>
#include "taskqueuemanage.h"
class FolderTransferTask;
static const int TASKTHREADMAXCOUNT=6;
static const int UPLOADTASKTHREADMAXCOUNT=2;
static const int DOWNLOADTASKTHREADMAXCOUNT=2;
static const int FILESYSTEMMANAGETHREADMAXCOUNT=1;
class WorkThreadManage : public QObject
{
    Q_OBJECT
public:
    explicit WorkThreadManage(QObject *parent = nullptr);
    //初始化
    void init(TaskQueueManage *uptqm,TaskQueueManage* downtqm,TaskQueueManage*filesysm);



    //添加单个文件/文件夹传输任务,视情况决定是否加建线程(调度线程)
    TransferTask* addTransferTask
        (string taskType,string transfertasktype,string transfertype,string sender,
         string reciver,size_t offset,string length,SuperTransferTask* parenttask=nullptr);

    //添加文件管理任务

    //工作线程运行函数(工作线程)
    static void* taskWorkerProcess(void* arg);

signals:

public slots:
//文件传输
    //任务从阻塞队列搬到就绪队列,视情况加建线程处理任务
    void processTransferTask(TaskQueueManage* taskquema);

private:
//文件传输
    //文件传输线程池
    std::deque<pthread_t> threadstool;
    int threadscount=0;
    int uploadthreadscount=0;
    int downloadthreadscount=0;
    int filesystemmanagethreadscount=0;
    TaskQueueManage* upLoadTaskManage;//上传任务管理器
    TaskQueueManage* downloadTaskManage;//下载任务管理器
    TaskQueueManage* fileSystemManage;//文件系统管理器
};

#endif // WORKTHREADMANAGE_H
