#ifndef SUPERTRANSFERTASK_H
#define SUPERTRANSFERTASK_H

#include "transfertask.h"
#include "ordinarytransfertask.h"
#include "taskscheduleshowd.h"
#include <vector>
#include <QTimer>
class WorkThreadManage;
class SuperTransferTask : public QObject,public TransferTask
{
    Q_OBJECT
public:
    explicit SuperTransferTask(QObject *parent = nullptr);
    virtual ~SuperTransferTask();
    void setTimer();

    //传输任务初始化
    virtual void transferInit(string taskType,string transfertasktype,string transfertype,string sender,
                              string reciver,size_t offset,string length) override;

    //任务执行
    virtual void taskExecution() override;

    //传输级别(是超级任务还是普通任务)    //超级传输任务(SuperTransferTask)
    virtual string getTransferTaskType() override;

    //获取任务类型
    virtual string getTaskType() override;

    //获取传输类型
    virtual string getTransferType() override;

    //获取传送者
    virtual string getTransferSender() override;

    //获取接收者
    virtual string getTransferReciver() override;

    //获取传输字节数
    virtual string getTransferLength() override;

    //加入就绪队列前的准备
    bool readyToAddReadyQueue();

    //超级任务进入运行状态，所有子任务切换为on状态
    void turnOnAllSubtask();

    //建立接收暂停信号的连接
    void connectSubtaskStopMessage();

    //接触接收暂停信号的连接
    void disconnectSubtaskStopMessage();

    //给所有子任务上锁
    void lockAllSubtask();

    //给所有子任务解锁
    void unlockAllSubtask();

    //释放所有子任务
    void deleteAllSubtask();

    //获取子任务数组
    vector<pair<TransferTask*,string>> getsubTaskVector();

    //设置管理器
    void setWorkThreadManage(WorkThreadManage* workthreadmanage);

public slots:
    //处理子任务进度信号的槽函数
    void slotForScheduleSignal(size_t file_size,size_t send_size);
    //处理子任务暂停信号的槽函数
    void slotForBreakSignal(TransferTask* trantask);
    //处理子任务结束信号的槽函数
    void slotForEndSignal(TransferTask* trantask);
    //定时器定时发送进度信号
    void sendScheduleOnTime();
signals:
    //发送进度信号给管理类
    void sendSchedule(size_t file_size,size_t send_size);
    //发送暂停信号给管理类
    void task_break(TransferTask*trantask,TaskScheduleShowd* scheDule);
    //发送结束信号给管理类
    void task_end(TransferTask*trantask,TaskScheduleShowd* scheDule);

    //发送暂停信号
    void task_stop(TransferTask* trantask,TaskScheduleShowd* scheDule);
    //发送开始信号
    void task_start(TransferTask* trantask,TaskScheduleShowd* scheDule);
    //发送删除信号
    void task_delete(TransferTask* trantask,TaskScheduleShowd* scheDule);

private:
    //文件数据段传输(FileSegmentTransfer) 文件夹数据段传输(FolderSegmentTransfer) 获取信息(GetInformation) 提交指令(PushInstruct)
    string m_taskType;//任务类型

    //文件数据段传输
    //传输类型 上传文件(uploadfile) 上传文件夹(uploadfolder) 下载文件(downloadfile) 下载传文件夹(downloadfolder)
    string m_transfer_type="";
    //传输任务类型  超级传输任务子任务(SuperTransferSubtask) 普通传输任务(NormalTransferTask)
    string m_transfer_task_type="SuperTransferTask";
    //传输过程变量
    string m_sender;//文件传输的发送者
    string m_reciver;//文件传输的接收者
    pthread_mutex_t transmissionVolumeMutex;//传输量锁
    vector<pair<OrdinaryTransferTask*,size_t>> subtaskTransferSize;//传输量数组
    //任务暂停--启动
    pthread_mutex_t scheduleMutex;//暂停启动锁
    bool m_scheduleIsLock;//暂停 可启动状态
    //文件数据段 文件夹数据段(重启机制)
    size_t m_begin_offset=0;
    size_t m_current_offset=0;
    size_t m_current_data_segment_length=0;
    size_t m_data_segment_length_size=0;
    string m_data_segment_length="OffsetToEnd";

    //子任务数组
    static const int subtaskCount=2;
    vector<pair<TransferTask*,string>> subtasksVector;

    //可视化数据传输任务管理器
    WorkThreadManage* m_work_thread_manage=nullptr;

    //定时器
    QTimer *fTimer;

    //切分超级任务为普通的子任务
    void splittingSuperTaskToSubtask();
};

#endif // SUPERTRANSFERTASK_H
