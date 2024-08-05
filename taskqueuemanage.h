#ifndef TASKQUEUEMANAGE_H
#define TASKQUEUEMANAGE_H

#include <QObject>
#include <QStandardItemModel>
#include <queue>
#include <unordered_map>
#include <pthread.h>
#include <QTableView>
#include <QMetaType>
#include <QThread>
#include "tasklistview.h"
#include "ordinarytransfertask.h"
#include "supertransfertask.h"
class FolderTransferTask;
class WorkThreadManage;
using namespace std;
class TaskQueueManage : public QObject
{
    Q_OBJECT
public:
    explicit TaskQueueManage(QObject *parent = nullptr);
    ~TaskQueueManage();
    pthread_spinlock_t transferTaskQueueMutex;
signals:
    //任务从阻塞队列搬到就绪队列，视情况加建线程处理任务
    void processTransferTaskSig(TaskQueueManage* taskquema);

    //工作线程通知主线程调整ui界面
    void delete_task_schedule_sig(TaskScheduleShowd* schedule_widget);
    void insert_task_schedule_sig(int task_index,TaskScheduleShowd* schedule_widget);

public slots:
//主线程的任务管理(文件传输任务)
    //就绪队列对暂停信号的处理函数    就绪to阻塞
    void slotReadyModelForStopSignal(TransferTask* trantask,TaskScheduleShowd* scheDule);
    //运行队列对进度条暂停请求信号    运行to阻塞
    void slotRunningModelForSchedulStopSignal(TransferTask* trantask,TaskScheduleShowd* scheDule);
    //运行队列对传输任务暂停完成信号   运行to阻塞
    void slotRunningModelForTransferTaskStopSignal(TransferTask* trantask,TaskScheduleShowd* scheDule);
    //运行队列对任务结束信号    运行to结束
    void slotRunningModelForEndSignal(TransferTask* trantask,TaskScheduleShowd* scheDule);
    //阻塞队列对开始信号    阻塞to就绪
    void slotBlockModelForStartSignal(TransferTask* trantask,TaskScheduleShowd* scheDule);
    //阻塞队列对取消信号    阻塞to结束
    void slotBlockModelForCancelSignal(TransferTask* trantask,TaskScheduleShowd* scheDule);

    //暂停运行队列
    void slotStopRunningQueue();
    //暂停就绪队列
    void slotStopReadyQueue();
    //开始阻塞队列
    void slotStartBlockQueue();

public:
//主线程
    //设置model的QTabelView
    void init(TaskListView* runView,TaskListView* readyBlockView);

//主线程和文件夹线程
    //添加任务到就绪队列    to就绪
    TransferTask*
    addTransferTaskToReadyQueue(string taskType,string transfertasktype,string transfertype,string sender,
               string reciver,size_t offset,string length,WorkThreadManage* workthreadmanage=nullptr,
                        SuperTransferTask* parenttask=nullptr);
    TransferTask*
    addTransferTaskToReadyQueue(TransferTask* transfertask);
    //获取就绪队列任务数量
    int getReadyQueueSize(){return readyQueue.size();}

//工作线程
    //从就绪队列里领取任务到运行队列    就绪to运行
    TransferTask* transferTaskFromReadyQueueToRunningQueue();


public:
//传输队列
    //传输运行队列
    deque<std::pair<TransferTask*,TaskScheduleShowd*>> runningQueue;
    //传输就绪队列
    deque<std::pair<TransferTask*,TaskScheduleShowd*>> readyQueue;
    //传输阻塞队列
    deque<std::pair<TransferTask*,TaskScheduleShowd*>> blockQueue;   

//传输视图
    //传输运行视图
    TaskListView* runningView;
    //传输队列视图
    TaskListView* readyAndBlockView;

//ui线程
    QThread* uithread;

//文件系统管理队列
    deque<OrdinaryTransferTask*> fileSystemManageQueue;
    pthread_spinlock_t fileSystemManageTaskQueueMutex;
    //添加文件系统管理任务
    void addFileSystemManageTaskToReadyQueue(OrdinaryTransferTask* filesystemmanagetask);

    //从任务队列里获取任务
    OrdinaryTransferTask* getFileSystemManageTask();
};

#endif // TASKQUEUEMANAGE_H
