#include "supertransfertask.h"
#include "workthreadmanage.h"

SuperTransferTask::SuperTransferTask(QObject *parent)
    : QObject{parent}
{
    pthread_mutex_init(&transmissionVolumeMutex,nullptr);
    pthread_mutex_init(&scheduleMutex,nullptr);

}

SuperTransferTask::~SuperTransferTask()
{
    pthread_mutex_destroy(&scheduleMutex);
    pthread_mutex_destroy(&transmissionVolumeMutex);
}

void SuperTransferTask::setTimer()
{
    fTimer=new QTimer(this);
    fTimer->stop();
    fTimer->setInterval(1000);
    connect(fTimer,&QTimer::timeout,this,&SuperTransferTask::sendScheduleOnTime);
    fTimer->start();
}

void SuperTransferTask::transferInit(string taskType, string transfertasktype,
                    string transfertype, string sender, string reciver, size_t offset, string length)
{
    m_taskType=taskType;
    m_transfer_task_type=transfertasktype;
    m_transfer_type=transfertype;

    m_sender=sender;
    m_reciver=reciver;
    m_begin_offset=offset;
    m_current_offset=offset;
    m_data_segment_length=length;

    if(length!="OffsetToEnd")
        sscanf(length.c_str(),"%zu",&m_current_data_segment_length);
    if(transfertype=="uploadfile"&&length=="OffsetToEnd")
    {
        QFileInfo fileinfo(QString(sender.c_str()));
        m_current_data_segment_length=fileinfo.size()-m_current_offset;
        m_data_segment_length=to_string(m_current_data_segment_length);
        cout<<"currentlength:"<<m_current_data_segment_length<<endl;
    }
    if(transfertype=="uploadfolder"&&length=="OffsetToEnd")
    {
        m_current_data_segment_length=OrdinaryTransferTask::getFolderSize(sender.c_str())-m_current_offset;
        m_data_segment_length=to_string(m_current_data_segment_length);
    }
    m_data_segment_length_size=m_current_data_segment_length;
    cout<<"m_current_data_segment_length:"<<m_current_data_segment_length<<endl;

}

void SuperTransferTask::taskExecution()
{
    cout<<"SuperTransferTask exec!"<<endl;
    //cout<<"m_data_segment_length_size:"<<m_data_segment_length_size<<endl;
//    if(m_data_segment_length_size==0)
//    {
//        //splittingSuperTaskToSubtask();
//        cout<<"segment length wrong!"<<endl;
//    }
//    else
//    {
//        cout<<"segment length right!"<<endl;
//    }
}

string SuperTransferTask::getTransferTaskType()
{
    return m_transfer_task_type;
}

string SuperTransferTask::getTaskType()
{
    return m_taskType;
}

string SuperTransferTask::getTransferType()
{
    return m_transfer_type;
}

string SuperTransferTask::getTransferSender()
{
    return m_sender;
}

string SuperTransferTask::getTransferReciver()
{
    return m_reciver;
}

string SuperTransferTask::getTransferLength()
{
    return m_data_segment_length;
}

bool SuperTransferTask::readyToAddReadyQueue()
{
    if(subtasksVector.begin()==subtasksVector.end())
    {
        splittingSuperTaskToSubtask();
        return false;
    }
    else
        return true;
}

void SuperTransferTask::turnOnAllSubtask()
{
    cout<<"turnOnAllSubtask exec!"<<endl;
    for(auto &s:subtasksVector)
        s.second="on";
    for(auto s:subtasksVector)
        cout<<s.second<<" ";
    cout<<endl;
}

void SuperTransferTask::connectSubtaskStopMessage()
{
    for(auto s:subtasksVector)
    {
        connect(dynamic_cast<OrdinaryTransferTask*>(s.first),&OrdinaryTransferTask::task_break,
                this,&SuperTransferTask::slotForBreakSignal,Qt::QueuedConnection);
    }
}

void SuperTransferTask::disconnectSubtaskStopMessage()
{
    for(auto s:subtasksVector)
    {
        disconnect(dynamic_cast<OrdinaryTransferTask*>(s.first),&OrdinaryTransferTask::task_break,
                this,&SuperTransferTask::slotForBreakSignal);
    }
}

void SuperTransferTask::lockAllSubtask()
{
    for(auto s:subtasksVector)
    {
        if(!dynamic_cast<OrdinaryTransferTask*>(s.first)->ifScheduleLocked())
            dynamic_cast<OrdinaryTransferTask*>(s.first)->lockTaskSchedule();
    }
}

void SuperTransferTask::unlockAllSubtask()
{
    for(auto s:subtasksVector)
    {
        if(!dynamic_cast<OrdinaryTransferTask*>(s.first)->ifScheduleLocked())
            cout<<"please wait for workthread exit!"<<endl;
        dynamic_cast<OrdinaryTransferTask*>(s.first)->unlockTaskSchedule();
    }
}

void SuperTransferTask::deleteAllSubtask()
{
    for(auto s:subtasksVector)
    {
        delete s.first;
    }
}

vector<pair<TransferTask *, string>> SuperTransferTask::getsubTaskVector()
{
    return this->subtasksVector;
}

void SuperTransferTask::setWorkThreadManage(WorkThreadManage *workthreadmanage)
{
    m_work_thread_manage=workthreadmanage;
}

void SuperTransferTask::slotForScheduleSignal(size_t file_size, size_t send_size)
{
    OrdinaryTransferTask* subtask=static_cast<OrdinaryTransferTask*>(sender());

    pthread_mutex_lock(&this->transmissionVolumeMutex);
    //找到对应任务的位置
    vector<pair<OrdinaryTransferTask*,size_t>>::iterator result=
        find_if(subtaskTransferSize.begin(),subtaskTransferSize.end(),
                [subtask](std::pair<OrdinaryTransferTask*,size_t> tas)
                {return tas.first==subtask;});

    //更新进度
    if(result!=subtaskTransferSize.end())
    {
        m_current_data_segment_length+=send_size-result->second;
        result->second=send_size;
    }
    else
    {
        subtaskTransferSize.push_back(make_pair(subtask,send_size));
        m_current_data_segment_length+=send_size;
    }
    m_current_data_segment_length=0;
    for(auto s:subtaskTransferSize)
        m_current_data_segment_length+=s.second;
    pthread_mutex_unlock(&this->transmissionVolumeMutex);
}

void SuperTransferTask::slotForBreakSignal(TransferTask *trantask)
{

    for(auto s:subtasksVector)
    {
        cout<<s.second<<" ";
    }
    cout<<endl;

    //找到对应任务的位置
    vector<pair<TransferTask*,string>>::iterator result=
        find_if(subtasksVector.begin(),subtasksVector.end(),
                [trantask](pair<TransferTask*,string> tas)
                {return tas.first==trantask;});
    result->second="off";

    cout<<"a subtask off!"<<endl;

    //所有子任务执行完毕，说明超级任务已经执行完毕，通知管理器处理已经自然执行结束的父任务
    bool allsubtaskbreak=true;
    for(auto s:subtasksVector)
    {
        cout<<s.second<<" ";
        if(s.second!="off")
        {

            allsubtaskbreak=false;
            break;
        }
    }
    cout<<endl;
    if(allsubtaskbreak)
    {
        cout<<"super task off!"<<endl;
        emit this->task_break(this,nullptr);
    }
}

void SuperTransferTask::slotForEndSignal(TransferTask *trantask)
{
    //找到对应任务的位置
    vector<pair<TransferTask*,string>>::iterator result=
        find_if(subtasksVector.begin(),subtasksVector.end(),
                [trantask](pair<TransferTask*,string> tas)
                {return tas.first==trantask;});

    //通知管理器处理已经自然执行结束的子任务
    //emit this->task_end(trantask,nullptr);
    delete result->first;
    subtasksVector.erase(result);

    //所有子任务执行完毕，说明超级任务已经执行完毕，通知管理器处理已经自然执行结束的父任务
    if(subtasksVector.begin()==subtasksVector.end())
        emit this->task_end(this,nullptr);
}

void SuperTransferTask::sendScheduleOnTime()
{
    size_t sendsize=0;
    pthread_mutex_lock(&this->transmissionVolumeMutex);
    sendsize=m_current_data_segment_length;
    pthread_mutex_unlock(&this->transmissionVolumeMutex);
    emit this->sendSchedule(m_data_segment_length_size,sendsize);
}

void SuperTransferTask::splittingSuperTaskToSubtask()
{
    size_t length=m_data_segment_length_size/subtaskCount;
    size_t totalTransmissionVolume=0;
    sscanf(m_data_segment_length.c_str(),"%zu",&totalTransmissionVolume);
    for(int i=0;i<subtaskCount;i++)
    {
        TransferTask* subtask=new OrdinaryTransferTask();
        if(i==subtaskCount-1)
        {           
            subtask->transferInit(m_taskType,
                                  "SuperTransferSubtask",
                                  m_transfer_type,
                                  m_sender,
                                  m_reciver,m_begin_offset+(i*length),
                                  to_string(totalTransmissionVolume-(i*length)));
            dynamic_cast<OrdinaryTransferTask*>(subtask)->setParentTask(this);
            subtasksVector.push_back(make_pair(subtask,"off"));
        }
        else
        {
            subtask->transferInit(m_taskType,
                                  "SuperTransferSubtask",
                                  m_transfer_type,
                                  m_sender,
                                  m_reciver,
                                  m_begin_offset+(i*length),
                                  to_string(length));
            dynamic_cast<OrdinaryTransferTask*>(subtask)->setParentTask(this);
            subtasksVector.push_back(make_pair(subtask,"off"));
        }       
    }
    for(auto s:subtasksVector)
    {
        //建立通信机制
        //进度信号
        connect(dynamic_cast<OrdinaryTransferTask*>(s.first),&OrdinaryTransferTask::sendSchedule,
                this,&SuperTransferTask::slotForScheduleSignal,Qt::QueuedConnection);
        //结束信号
        connect(dynamic_cast<OrdinaryTransferTask*>(s.first),&OrdinaryTransferTask::task_end,
                this,&SuperTransferTask::slotForEndSignal,Qt::QueuedConnection);
    }
}


