#include "taskqueuemanage.h"
#include "workthreadmanage.h"
#include <QListWidget>
TaskQueueManage::TaskQueueManage(QObject *parent)
    : QObject{parent}
{
    qRegisterMetaType<size_t>("size_t");
    qRegisterMetaType<TaskScheduleShowd *>("TaskScheduleShowd *");
    qRegisterMetaType<OrdinaryTransferTask *>("TransferTask *");
    pthread_spin_init(&transferTaskQueueMutex,PTHREAD_PROCESS_PRIVATE);
    pthread_spin_init(&fileSystemManageTaskQueueMutex,PTHREAD_PROCESS_PRIVATE);
    uithread=QThread::currentThread();

}

TaskQueueManage::~TaskQueueManage()
{
    pthread_spin_destroy(&transferTaskQueueMutex);
    pthread_spin_destroy(&fileSystemManageTaskQueueMutex);
}

//就绪队列的任务被暂停    (就绪--->>阻塞)
void TaskQueueManage::slotReadyModelForStopSignal(TransferTask* trantask,TaskScheduleShowd* scheDule)
{
    pthread_spin_lock(&this->transferTaskQueueMutex);

    //查找任务
    deque<std::pair<TransferTask*,TaskScheduleShowd*>>::iterator result;
    if(trantask)
        result=find_if(readyQueue.begin(),readyQueue.end(),
                        [trantask](std::pair<TransferTask*,TaskScheduleShowd*> tas)
                        {return tas.first==trantask;});
    else if(scheDule)
        result=find_if(readyQueue.begin(),readyQueue.end(),
                        [scheDule](std::pair<TransferTask*,TaskScheduleShowd*> tas)
                        {return tas.second==scheDule;});
    TransferTask* t=result->first;
    TaskScheduleShowd* ts=result->second;
    string transfertasktype=t->getTransferTaskType();

    if(transfertasktype=="SuperTransferTask")
    {
        //调整通信机制
        disconnect(ts,&TaskScheduleShowd::task_stop,
                   this,&TaskQueueManage::slotReadyModelForStopSignal);
        connect(ts,&TaskScheduleShowd::task_start,
                this,&TaskQueueManage::slotBlockModelForStartSignal,Qt::QueuedConnection);
        connect(ts,&TaskScheduleShowd::task_delete,
                this,&TaskQueueManage::slotBlockModelForCancelSignal,Qt::QueuedConnection);

        //进度条状态调整
        ts->setButtom(2);

        //调整进度条列表
        readyAndBlockView->insert_task_schedule(readyQueue.size()+blockQueue.size(),ts);

        //调整任务队列
        readyQueue.erase(result);
        std::pair<TransferTask *, TaskScheduleShowd *> taskandsche{t,ts};
        blockQueue.push_back(taskandsche);
    }
    else if(transfertasktype=="NormalTransferTask")
    {
        //调整通信机制
        disconnect(ts,&TaskScheduleShowd::task_stop,
                   this,&TaskQueueManage::slotReadyModelForStopSignal);
        connect(ts,&TaskScheduleShowd::task_start,
                this,&TaskQueueManage::slotBlockModelForStartSignal,Qt::QueuedConnection);
        connect(ts,&TaskScheduleShowd::task_delete,
                this,&TaskQueueManage::slotBlockModelForCancelSignal,Qt::QueuedConnection);

        //进度条状态调整
        ts->setButtom(2);

        //任务状态调整
        dynamic_cast<OrdinaryTransferTask*>(t)->lockTaskSchedule();

        //调整进度条列表
        readyAndBlockView->insert_task_schedule(readyQueue.size()+blockQueue.size(),ts);

        //调整任务队列
        readyQueue.erase(result);
        std::pair<TransferTask *, TaskScheduleShowd *> taskandsche{t,ts};
        blockQueue.push_back(taskandsche);
    }
    pthread_spin_unlock(&this->transferTaskQueueMutex);
}

//运行队列的任务被暂停    (运行--->>阻塞)
void TaskQueueManage::slotRunningModelForSchedulStopSignal(TransferTask* trantask,TaskScheduleShowd* scheDule)
{
    pthread_spin_lock(&this->transferTaskQueueMutex);

    //查找任务
    deque<std::pair<TransferTask*,TaskScheduleShowd*>>::iterator result;
    if(trantask)
        result=find_if(runningQueue.begin(),runningQueue.end(),
                        [trantask](std::pair<TransferTask*,TaskScheduleShowd*> tas)
                        {return tas.first==trantask;});
    else if(scheDule)
        result=find_if(runningQueue.begin(),runningQueue.end(),
                        [scheDule](std::pair<TransferTask*,TaskScheduleShowd*> tas)
                        {return tas.second==scheDule;});
    TransferTask* t=result->first;
    TaskScheduleShowd* ts=result->second;
    string transfertasktype=t->getTransferTaskType();

    if(transfertasktype=="SuperTransferTask")
    {
        //调整通信机制
        disconnect(ts,&TaskScheduleShowd::task_stop,
                   this,&TaskQueueManage::slotRunningModelForSchedulStopSignal);
        connect(ts,&TaskScheduleShowd::task_start,
                this,&TaskQueueManage::slotBlockModelForStartSignal,Qt::QueuedConnection);
        connect(ts,&TaskScheduleShowd::task_delete,
                this,&TaskQueueManage::slotBlockModelForCancelSignal,Qt::QueuedConnection);

        connect(dynamic_cast<SuperTransferTask*>(t),&SuperTransferTask::task_break,
                this,&TaskQueueManage::slotRunningModelForTransferTaskStopSignal,Qt::QueuedConnection);
        dynamic_cast<SuperTransferTask*>(t)->connectSubtaskStopMessage();

        dynamic_cast<SuperTransferTask*>(t)->lockAllSubtask();
    }
    else if(transfertasktype=="NormalTransferTask")
    {
        //调整通信机制
        disconnect(ts,&TaskScheduleShowd::task_stop,
                   this,&TaskQueueManage::slotRunningModelForSchedulStopSignal);
        connect(ts,&TaskScheduleShowd::task_start,
                this,&TaskQueueManage::slotBlockModelForStartSignal,Qt::QueuedConnection);
        connect(ts,&TaskScheduleShowd::task_delete,
                this,&TaskQueueManage::slotBlockModelForCancelSignal,Qt::QueuedConnection);

        connect(dynamic_cast<OrdinaryTransferTask*>(t),&OrdinaryTransferTask::task_break,
                this,&TaskQueueManage::slotRunningModelForTransferTaskStopSignal,Qt::QueuedConnection);

        //任务状态调整
        if(!dynamic_cast<OrdinaryTransferTask*>(t)->ifScheduleLocked())
            dynamic_cast<OrdinaryTransferTask*>(t)->lockTaskSchedule();
    }

    pthread_spin_unlock(&this->transferTaskQueueMutex);
}

void TaskQueueManage::slotRunningModelForTransferTaskStopSignal(TransferTask *trantask, TaskScheduleShowd *scheDule)
{
    pthread_spin_lock(&this->transferTaskQueueMutex);

    //查找任务
    deque<std::pair<TransferTask*,TaskScheduleShowd*>>::iterator result;
    if(trantask)
        result=find_if(runningQueue.begin(),runningQueue.end(),
                         [trantask](std::pair<TransferTask*,TaskScheduleShowd*> tas)
                         {return tas.first==trantask;});
    else if(scheDule)
        result=find_if(runningQueue.begin(),runningQueue.end(),
                         [scheDule](std::pair<TransferTask*,TaskScheduleShowd*> tas)
                         {return tas.second==scheDule;});
    if(result==runningQueue.end())
        cout<<"can not find task!"<<endl;
    TransferTask* t=result->first;
    TaskScheduleShowd* ts=result->second;
    string transfertasktype=t->getTransferTaskType();

    if(transfertasktype=="SuperTransferTask")
    {
        disconnect(dynamic_cast<SuperTransferTask*>(t),&SuperTransferTask::task_break,
                this,&TaskQueueManage::slotRunningModelForTransferTaskStopSignal);
        dynamic_cast<SuperTransferTask*>(t)->disconnectSubtaskStopMessage();

        //进度条状态调整
        ts->setButtom(2);

        //确定要插入的位置(保证超级传输任务的优先级比普通传输任务高)
        int a=0;
        deque<std::pair<TransferTask*,TaskScheduleShowd*>>::iterator blockresult=
            find_if(blockQueue.begin(),blockQueue.end(),[&a](std::pair<TransferTask*,TaskScheduleShowd*> tas)
                    {++a;return tas.first->getTransferTaskType()=="NormalTransferTask";});
        a--;

        //调整任务队列
        runningQueue.erase(result);

        cout<<"quit runningQueue size:"<<runningQueue.size()<<endl;

        std::pair<TransferTask *, TaskScheduleShowd *> taskandsche{t,ts};
        blockQueue.insert(blockresult,taskandsche);

        //调整进度条列表
        readyAndBlockView->insert_task_schedule(a,ts);
    }
    else if(transfertasktype=="NormalTransferTask")
    {
        disconnect(dynamic_cast<OrdinaryTransferTask*>(t),&OrdinaryTransferTask::task_break,
                this,&TaskQueueManage::slotRunningModelForTransferTaskStopSignal);

        //进度条状态调整
        ts->setButtom(2);

        //调整进度条列表
        readyAndBlockView->insert_task_schedule(readyQueue.size()+blockQueue.size(),ts);

        //调整任务队列
        runningQueue.erase(result);
        std::pair<TransferTask *, TaskScheduleShowd *> taskandsche{t,ts};
        blockQueue.push_back(taskandsche);
    }

    pthread_spin_unlock(&this->transferTaskQueueMutex);
}

//运行队列对任务结束信号  (运行--->>)
void TaskQueueManage::slotRunningModelForEndSignal(TransferTask *trantask,TaskScheduleShowd* scheDule)
{
    pthread_spin_lock(&this->transferTaskQueueMutex);

    //查找任务
    deque<std::pair<TransferTask*,TaskScheduleShowd*>>::iterator result=runningQueue.end();
    if(trantask)
        result=find_if(runningQueue.begin(),runningQueue.end(),
                          [trantask](std::pair<TransferTask*,TaskScheduleShowd*> tas)
                          {return tas.first==trantask;});
    if(result==runningQueue.end())
        qDebug()<<"task eraser wrong!";
    TransferTask* t=result->first;
    TaskScheduleShowd* ts=result->second;
    string transfertasktype=t->getTransferTaskType();

    if(transfertasktype=="SuperTransferTask")
    {
        //调整通信机制
        disconnect(ts,&TaskScheduleShowd::task_stop,
                   this,&TaskQueueManage::slotRunningModelForSchedulStopSignal);

        //调整进度条列表
        runningView->delete_task_schedule(ts);

        //调整任务队列
        runningQueue.erase(result);
    }
    else if(transfertasktype=="NormalTransferTask")
    {
        //调整通信机制
        disconnect(ts,&TaskScheduleShowd::task_stop,
                   this,&TaskQueueManage::slotRunningModelForSchedulStopSignal);

        //调整进度条列表
        runningView->delete_task_schedule(ts);

        //调整任务队列
        runningQueue.erase(result);
    }
    if(t)
        delete t;
    if(ts)
        delete ts;

    pthread_spin_unlock(&this->transferTaskQueueMutex);
}

//阻塞队列的任务被开始    (阻塞--->>就绪)
void TaskQueueManage::slotBlockModelForStartSignal(TransferTask* trantask,TaskScheduleShowd* scheDule)
{
    pthread_spin_lock(&this->transferTaskQueueMutex);

    //查找任务
    deque<std::pair<TransferTask*,TaskScheduleShowd*>>::iterator result;
    if(trantask)
        result=find_if(blockQueue.begin(),blockQueue.end(),
                          [trantask](std::pair<TransferTask*,TaskScheduleShowd*> tas)
                          {return tas.first==trantask;});
    else if(scheDule)
        result=find_if(blockQueue.begin(),blockQueue.end(),
                         [scheDule](std::pair<TransferTask*,TaskScheduleShowd*> tas)
                         {return tas.second==scheDule;});
    TransferTask* t=result->first;
    TaskScheduleShowd* ts=result->second;
    string transfertasktype=t->getTransferTaskType();

    if(transfertasktype=="SuperTransferTask")
    {
        //调整通信机制
        disconnect(ts,&TaskScheduleShowd::task_start,
                this,&TaskQueueManage::slotBlockModelForStartSignal);
        disconnect(ts,&TaskScheduleShowd::task_delete,
                this,&TaskQueueManage::slotBlockModelForCancelSignal);
        connect(ts,&TaskScheduleShowd::task_stop,
                this,&TaskQueueManage::slotReadyModelForStopSignal,Qt::QueuedConnection);

        //进度条状态调整
        ts->setButtom(1);

        //确定要插入的位置
        int a=0;
        deque<std::pair<TransferTask*,TaskScheduleShowd*>>::iterator insertresult=
            find_if(readyQueue.begin(),readyQueue.end(),[&a](std::pair<TransferTask*,TaskScheduleShowd*> tas)
                    {a++;return tas.first->getTransferTaskType()=="NormalTransferTask";});
        a--;
        //调整进度条列表
        readyAndBlockView->insert_task_schedule(a,ts);

        //调整任务队列
        cout<<"add surpertransfertask"<<endl;
        blockQueue.erase(result);
        std::pair<TransferTask *, TaskScheduleShowd *> taskandsche{t,ts};
        readyQueue.insert(insertresult,taskandsche);

        //加入就绪队列前的准备
        bool ifready=dynamic_cast<SuperTransferTask*>(t)->readyToAddReadyQueue();

        dynamic_cast<SuperTransferTask*>(t)->unlockAllSubtask();

        if(true)
        {
            auto subtaskvector=dynamic_cast<SuperTransferTask*>(t)->getsubTaskVector();
            for(auto s:subtaskvector)
            {
                cout<<"add surpertransfersubtask"<<endl;

                //确定要插入的位置
                deque<std::pair<TransferTask*,TaskScheduleShowd*>>::iterator subtaskresult=
                    find_if(readyQueue.begin(),readyQueue.end(),
                            [t](std::pair<TransferTask*,TaskScheduleShowd*> tas)
                            {return tas.first==t;});
                subtaskresult++;
                //任务添加到队列
                TransferTask *p=s.first;
                TaskScheduleShowd *q=nullptr;
                std::pair<TransferTask *, TaskScheduleShowd *> taskandsche{p,q};
                if(subtaskresult!=readyQueue.end())
                    readyQueue.insert(subtaskresult,taskandsche);
                else
                    readyQueue.push_back(taskandsche);
            }
        }
        for(auto s:readyQueue)
        {

            cout<<s.first->getTransferTaskType()<<",";
            if(s.second==nullptr)
                cout<<"nullptr"<<endl;
            else
                cout<<"1"<<endl;
        }
    }
    else if(transfertasktype=="NormalTransferTask")
    {
        qDebug()<<"add normal task!";
        //调整通信机制
        disconnect(ts,&TaskScheduleShowd::task_start,
                   this,&TaskQueueManage::slotBlockModelForStartSignal);
        disconnect(ts,&TaskScheduleShowd::task_delete,
                   this,&TaskQueueManage::slotBlockModelForCancelSignal);
        connect(ts,&TaskScheduleShowd::task_stop,
                this,&TaskQueueManage::slotReadyModelForStopSignal,Qt::QueuedConnection);

        //进度条状态调整
        ts->setButtom(1);

        //任务状态调整
        while(!dynamic_cast<OrdinaryTransferTask*>(t)->ifScheduleLocked())
            qDebug()<<"please wait for workthread exit!";
        dynamic_cast<OrdinaryTransferTask*>(t)->unlockTaskSchedule();

        //调整进度条列表
        readyAndBlockView->delete_task_schedule(ts);
        readyAndBlockView->insert_task_schedule(readyQueue.size(),ts);

        //调整任务队列
        blockQueue.erase(result);
        std::pair<TransferTask *, TaskScheduleShowd *> taskandsche{t,ts};
        readyQueue.push_back(taskandsche);
    }

    pthread_spin_unlock(&this->transferTaskQueueMutex);

    //发送信号给线程类增加视情况增加线程处理任务
    emit this->processTransferTaskSig(this);
}

//阻塞队列的任务被取消    (阻塞--->>)
void TaskQueueManage::slotBlockModelForCancelSignal(TransferTask* trantask,TaskScheduleShowd* scheDule)
{
    pthread_spin_lock(&this->transferTaskQueueMutex);

    //查找任务
    deque<std::pair<TransferTask*,TaskScheduleShowd*>>::iterator result;
    if(trantask)
        result=find_if(blockQueue.begin(),blockQueue.end(),
                          [trantask](std::pair<TransferTask*,TaskScheduleShowd*> tas)
                          {return tas.first==trantask;});
    else if(scheDule)
        result=find_if(blockQueue.begin(),blockQueue.end(),
                         [scheDule](std::pair<TransferTask*,TaskScheduleShowd*> tas)
                         {return tas.second==scheDule;});
    TransferTask* t=result->first;
    TaskScheduleShowd* ts=result->second;
    string transfertasktype=t->getTransferTaskType();

    if(transfertasktype=="SuperTransferTask")
    {
        //调整通信机制
        disconnect(ts,&TaskScheduleShowd::task_start,
                   this,&TaskQueueManage::slotBlockModelForStartSignal);
        disconnect(ts,&TaskScheduleShowd::task_delete,
                   this,&TaskQueueManage::slotBlockModelForCancelSignal);

        dynamic_cast<SuperTransferTask*>(t)->deleteAllSubtask();

        //调整进度条列表
        readyAndBlockView->delete_task_schedule(ts);

        //调整任务队列
        blockQueue.erase(result);
    }

    else if(transfertasktype=="NormalTransferTask")
    {
        //调整通信机制
        disconnect(ts,&TaskScheduleShowd::task_start,
                   this,&TaskQueueManage::slotBlockModelForStartSignal);
        disconnect(ts,&TaskScheduleShowd::task_delete,
                   this,&TaskQueueManage::slotBlockModelForCancelSignal);

        //调整进度条列表
        readyAndBlockView->delete_task_schedule(ts);

        //调整任务队列
        blockQueue.erase(result);
    }
    if(t)
        delete t;
    if(ts)
        delete ts;

    pthread_spin_unlock(&this->transferTaskQueueMutex);
}

//暂停运行队列
void TaskQueueManage::slotStopRunningQueue()
{
    for(auto s:runningQueue)
    {
        slotRunningModelForSchedulStopSignal(s.first,s.second);
    }
}

//暂停就绪队列
void TaskQueueManage::slotStopReadyQueue()
{
    for(auto s:readyQueue)
    {
        if(s.first->getTransferTaskType()=="SuperTransferTask"
            ||s.first->getTransferTaskType()=="NormalTransferTask")
            slotReadyModelForStopSignal(s.first,s.second);
    }
}

//开始阻塞队列
void TaskQueueManage::slotStartBlockQueue()
{
    for(auto s:blockQueue)
    {
        slotBlockModelForStartSignal(s.first,s.second);
    }
}


void TaskQueueManage::init(TaskListView *runView, TaskListView *readyBlockView)
{
    runningView=runView;
    readyAndBlockView=readyBlockView;
}


//主线程添加任务到就绪队列  (--->>就绪)
TransferTask *TaskQueueManage::addTransferTaskToReadyQueue(string taskType, string transfertasktype,
                                                   string transfertype,
                       string sender, string reciver, size_t offset, string length, WorkThreadManage *workthreadmanage,
                        SuperTransferTask *parenttask)
{
    pthread_spin_lock(&this->transferTaskQueueMutex);

    //新建与初始化任务,进度条
    TransferTask* t=nullptr;
    TaskScheduleShowd* ts=nullptr;
    if(transfertasktype=="SuperTransferTask")
    {
        qDebug()<<"add surpertransfertask!";
        //初始化任务
        t=new SuperTransferTask();
        t->transferInit(taskType,transfertasktype,transfertype,sender,reciver,offset,length);
        dynamic_cast<SuperTransferTask*>(t)->setWorkThreadManage(workthreadmanage);

        //初始化进度条
        ts=new TaskScheduleShowd();
        string transfername="";
        if(transfertype=="uploadfile"||transfertype=="uploadfolder")
            transfername=sender;
        else if(transfertype=="downloadfile"||transfertype=="downloadfolder")
            transfername=reciver+"/"+OrdinaryTransferTask::getFileNameFromUri(sender);
        ts->init(string(transfername),1);
        ts->moveToThread(uithread);

        //建立通信机制
        //进度信号
        connect(dynamic_cast<SuperTransferTask*>(t),&SuperTransferTask::sendSchedule,
                ts,&TaskScheduleShowd::processScheduleSig,Qt::QueuedConnection);
        //结束信号
        connect(dynamic_cast<SuperTransferTask*>(t),&SuperTransferTask::task_end,
                this,&TaskQueueManage::slotRunningModelForEndSignal,Qt::QueuedConnection);
        //暂停信号（就绪--->>阻塞）
        connect(ts,&TaskScheduleShowd::task_stop,
                this,&TaskQueueManage::slotReadyModelForStopSignal,Qt::QueuedConnection);

        //确定要插入的位置(保证超级传输任务的优先级比普通传输任务高)
        int a=0;
        deque<std::pair<TransferTask*,TaskScheduleShowd*>>::iterator parenttaskresult=
            find_if(readyQueue.begin(),readyQueue.end(),[&a](std::pair<TransferTask*,TaskScheduleShowd*> tas)
                    {++a;return tas.first->getTransferTaskType()=="NormalTransferTask";});
        a--;

        //进度条添加到列表
        connect(this,&TaskQueueManage::insert_task_schedule_sig,
                readyAndBlockView,&TaskListView::slotInsertTaskSchedule,Qt::QueuedConnection);
        emit this->insert_task_schedule_sig(a,ts);
        disconnect(this,&TaskQueueManage::insert_task_schedule_sig,
                   readyAndBlockView,&TaskListView::slotInsertTaskSchedule);

        //任务添加到队列
        std::pair<TransferTask *, TaskScheduleShowd *> taskandsche{t,ts};
        if(parenttaskresult!=readyQueue.end())
            readyQueue.insert(parenttaskresult,taskandsche);
        else
            readyQueue.push_back(taskandsche);

        //加入就绪队列前的准备
        bool ifready=dynamic_cast<SuperTransferTask*>(t)->readyToAddReadyQueue();
        if(!ifready)
        {
            auto subtaskvector=dynamic_cast<SuperTransferTask*>(t)->getsubTaskVector();
            for(auto s:subtaskvector)
            {
                cout<<"add surpertransfersubtask"<<endl;

                //确定要插入的位置
                deque<std::pair<TransferTask*,TaskScheduleShowd*>>::iterator subtaskresult=
                    find_if(readyQueue.begin(),readyQueue.end(),
                            [t](std::pair<TransferTask*,TaskScheduleShowd*> tas)
                            {return tas.first==t;});
                subtaskresult++;
                //任务添加到队列
                TransferTask *p=s.first;
                TaskScheduleShowd *q=nullptr;
                std::pair<TransferTask *, TaskScheduleShowd *> taskandsche{p,q};
                if(subtaskresult!=readyQueue.end())
                    readyQueue.insert(subtaskresult,taskandsche);
                else
                    readyQueue.push_back(taskandsche);
            }
        }
    }
    else if(transfertasktype=="NormalTransferTask")
    {
        //初始化任务
        t=new OrdinaryTransferTask();
        t->transferInit(taskType,transfertasktype,transfertype,sender,reciver,offset,length);

        //初始化进度条
        ts=new TaskScheduleShowd();
        string transfername="";
        if(transfertype=="uploadfile"||transfertype=="uploadfolder")
            transfername=sender;
        else if(transfertype=="downloadfile"||transfertype=="downloadfolder")
            transfername=reciver+"/"+OrdinaryTransferTask::getFileNameFromUri(sender);
        ts->init(string(transfername),1);
        ts->moveToThread(uithread);

        //建立通信机制
        //进度信号
        connect(dynamic_cast<OrdinaryTransferTask*>(t),&OrdinaryTransferTask::sendSchedule,
                ts,&TaskScheduleShowd::processScheduleSig,Qt::QueuedConnection);
        //结束信号
        connect(dynamic_cast<OrdinaryTransferTask*>(t),&OrdinaryTransferTask::task_end,
                this,&TaskQueueManage::slotRunningModelForEndSignal,Qt::QueuedConnection);
        //暂停信号（就绪--->>阻塞）
        connect(ts,&TaskScheduleShowd::task_stop,
                this,&TaskQueueManage::slotReadyModelForStopSignal,Qt::QueuedConnection);


        //进度条添加到列表
        connect(this,&TaskQueueManage::insert_task_schedule_sig,
                readyAndBlockView,&TaskListView::slotInsertTaskSchedule,Qt::QueuedConnection);
        emit this->insert_task_schedule_sig(readyQueue.size(),ts);
        disconnect(this,&TaskQueueManage::insert_task_schedule_sig,
                   readyAndBlockView,&TaskListView::slotInsertTaskSchedule);

        //任务添加到队列
        std::pair<TransferTask *, TaskScheduleShowd *> taskandsche{t,ts};
        readyQueue.push_back(taskandsche);
    }
    else
        cout<<"transfertasktype wrong!"<<endl;

    pthread_spin_unlock(&this->transferTaskQueueMutex);

    //发送信号给线程类增加视情况增加线程处理任务
    emit this->processTransferTaskSig(this);

    return t;
}

TransferTask *TaskQueueManage::addTransferTaskToReadyQueue(TransferTask *transfertask)
{
    pthread_spin_lock(&this->transferTaskQueueMutex);

    string taskType=transfertask->getTaskType();
    string transfertasktype=transfertask->getTransferTaskType();
    string transfertype=transfertask->getTransferType();
    string sender=transfertask->getTransferSender();
    string reciver=transfertask->getTransferReciver();
    size_t offset=0;
    string length=transfertask->getTransferLength();
    //return addTransferTaskToReadyQueue(taskType,transfertasktype,transfertype,sender,reciver,offset,length);

    //新建与初始化任务,进度条
    TransferTask* t=transfertask;
    TaskScheduleShowd* ts=nullptr;
    if(transfertasktype=="SuperTransferTask")
    {
        qDebug()<<"add surpertransfertask!";
        //初始化任务
//        t=new SuperTransferTask();
//        t->transferInit(taskType,transfertasktype,transfertype,sender,reciver,offset,length);
//        dynamic_cast<SuperTransferTask*>(t)->setWorkThreadManage(workthreadmanage);

        //初始化进度条
        ts=new TaskScheduleShowd();
        string transfername="";
        if(transfertype=="uploadfile"||transfertype=="uploadfolder")
            transfername=sender;
        else if(transfertype=="downloadfile"||transfertype=="downloadfolder")
            transfername=reciver+"/"+OrdinaryTransferTask::getFileNameFromUri(sender);
        ts->init(string(transfername),1);
        ts->moveToThread(uithread);

        //建立通信机制
        //进度信号
        connect(dynamic_cast<SuperTransferTask*>(t),&SuperTransferTask::sendSchedule,
                ts,&TaskScheduleShowd::processScheduleSig,Qt::QueuedConnection);
        //结束信号
        connect(dynamic_cast<SuperTransferTask*>(t),&SuperTransferTask::task_end,
                this,&TaskQueueManage::slotRunningModelForEndSignal,Qt::QueuedConnection);
        //暂停信号（就绪--->>阻塞）
        connect(ts,&TaskScheduleShowd::task_stop,
                this,&TaskQueueManage::slotReadyModelForStopSignal,Qt::QueuedConnection);

        //确定要插入的位置(保证超级传输任务的优先级比普通传输任务高)
        int a=0;
        deque<std::pair<TransferTask*,TaskScheduleShowd*>>::iterator parenttaskresult=
            find_if(readyQueue.begin(),readyQueue.end(),[&a](std::pair<TransferTask*,TaskScheduleShowd*> tas)
                    {++a;return tas.first->getTransferTaskType()=="NormalTransferTask";});
        a--;

        //进度条添加到列表
        connect(this,&TaskQueueManage::insert_task_schedule_sig,
                readyAndBlockView,&TaskListView::slotInsertTaskSchedule,Qt::QueuedConnection);
        emit this->insert_task_schedule_sig(a,ts);
        disconnect(this,&TaskQueueManage::insert_task_schedule_sig,
                   readyAndBlockView,&TaskListView::slotInsertTaskSchedule);

        //任务添加到队列
        std::pair<TransferTask *, TaskScheduleShowd *> taskandsche{t,ts};
        if(parenttaskresult!=readyQueue.end())
            readyQueue.insert(parenttaskresult,taskandsche);
        else
            readyQueue.push_back(taskandsche);

        //加入就绪队列前的准备
        bool ifready=dynamic_cast<SuperTransferTask*>(t)->readyToAddReadyQueue();
        if(!ifready)
        {
            auto subtaskvector=dynamic_cast<SuperTransferTask*>(t)->getsubTaskVector();
            for(auto s:subtaskvector)
            {
                cout<<"add surpertransfersubtask"<<endl;

                //确定要插入的位置
                deque<std::pair<TransferTask*,TaskScheduleShowd*>>::iterator subtaskresult=
                    find_if(readyQueue.begin(),readyQueue.end(),
                            [t](std::pair<TransferTask*,TaskScheduleShowd*> tas)
                            {return tas.first==t;});
                subtaskresult++;
                //任务添加到队列
                TransferTask *p=s.first;
                TaskScheduleShowd *q=nullptr;
                std::pair<TransferTask *, TaskScheduleShowd *> taskandsche{p,q};
                if(subtaskresult!=readyQueue.end())
                    readyQueue.insert(subtaskresult,taskandsche);
                else
                    readyQueue.push_back(taskandsche);
            }
        }
    }
    else if(transfertasktype=="NormalTransferTask")
    {
        //初始化任务
//        t=new OrdinaryTransferTask();
//        t->transferInit(taskType,transfertasktype,transfertype,sender,reciver,offset,length);

        //初始化进度条
        ts=new TaskScheduleShowd();
        string transfername="";
        if(transfertype=="uploadfile"||transfertype=="uploadfolder")
            transfername=sender;
        else if(transfertype=="downloadfile"||transfertype=="downloadfolder")
            transfername=reciver+"/"+OrdinaryTransferTask::getFileNameFromUri(sender);
        ts->init(string(transfername),1);
        ts->moveToThread(uithread);

        //建立通信机制
        //进度信号
        connect(dynamic_cast<OrdinaryTransferTask*>(t),&OrdinaryTransferTask::sendSchedule,
                ts,&TaskScheduleShowd::processScheduleSig,Qt::QueuedConnection);
        //结束信号
        connect(dynamic_cast<OrdinaryTransferTask*>(t),&OrdinaryTransferTask::task_end,
                this,&TaskQueueManage::slotRunningModelForEndSignal,Qt::QueuedConnection);
        //暂停信号（就绪--->>阻塞）
        connect(ts,&TaskScheduleShowd::task_stop,
                this,&TaskQueueManage::slotReadyModelForStopSignal,Qt::QueuedConnection);


        //进度条添加到列表
        connect(this,&TaskQueueManage::insert_task_schedule_sig,
                readyAndBlockView,&TaskListView::slotInsertTaskSchedule,Qt::QueuedConnection);
        emit this->insert_task_schedule_sig(readyQueue.size(),ts);
        disconnect(this,&TaskQueueManage::insert_task_schedule_sig,
                   readyAndBlockView,&TaskListView::slotInsertTaskSchedule);

        //任务添加到队列
        std::pair<TransferTask *, TaskScheduleShowd *> taskandsche{t,ts};
        readyQueue.push_back(taskandsche);
    }
    else
        cout<<"transfertasktype wrong!"<<endl;

    pthread_spin_unlock(&this->transferTaskQueueMutex);

    //发送信号给线程类增加视情况增加线程处理任务
    emit this->processTransferTaskSig(this);

    return t;
}

//工作线程从就绪队列领取任务到运行队列    (就绪--->>运行)
TransferTask* TaskQueueManage::transferTaskFromReadyQueueToRunningQueue()
{
    pthread_spin_lock(&this->transferTaskQueueMutex);

    if(!readyQueue.empty())
    {
        //获得可视化数据传输任务
        auto taskandsche=readyQueue.front();
        TransferTask* t=taskandsche.first;
        TaskScheduleShowd* ts=taskandsche.second;
        string transfertasktype=t->getTransferTaskType();

        if(transfertasktype=="SuperTransferTask"||transfertasktype=="NormalTransferTask")
        {
            //调整通信机制
            disconnect(ts,&TaskScheduleShowd::task_stop,
                       this,&TaskQueueManage::slotReadyModelForStopSignal);
            connect(ts,&TaskScheduleShowd::task_stop,
                    this,&TaskQueueManage::slotRunningModelForSchedulStopSignal,Qt::QueuedConnection);

            //进度条状态调整
            emit ts->resetButtomSig(0);

            //调整进度条列表
            connect(this,&TaskQueueManage::delete_task_schedule_sig,
                    readyAndBlockView,&TaskListView::slotDeleteTaskSchedule,Qt::QueuedConnection);
            emit this->delete_task_schedule_sig(ts);
            disconnect(this,&TaskQueueManage::delete_task_schedule_sig,
                       readyAndBlockView,&TaskListView::slotDeleteTaskSchedule);
            connect(this,&TaskQueueManage::insert_task_schedule_sig,
                    runningView,&TaskListView::slotInsertTaskSchedule,Qt::QueuedConnection);
            emit this->insert_task_schedule_sig(runningQueue.size(),ts);
            disconnect(this,&TaskQueueManage::insert_task_schedule_sig,
                       runningView,&TaskListView::slotInsertTaskSchedule);

            if(transfertasktype=="SuperTransferTask")
                dynamic_cast<SuperTransferTask*>(t)->turnOnAllSubtask();

            //调整任务队列
            readyQueue.pop_front();
            std::pair<TransferTask *, TaskScheduleShowd *> tas{t,ts};
            //runningQueue.push_back(tas);
            cout<<"runningQueue size:"<<runningQueue.size()<<endl;
            runningQueue.push_back(make_pair(t,ts));
            cout<<"runningQueue size:"<<runningQueue.size()<<endl;
        }
        else if(transfertasktype=="SuperTransferSubtask")
        {
            //调整任务队列
            readyQueue.pop_front();

        }
        cout<<"readyQueue size:"<<readyQueue.size()<<endl;;

        pthread_spin_unlock(&this->transferTaskQueueMutex);
        return t;
    }
    pthread_spin_unlock(&this->transferTaskQueueMutex);
    return nullptr;
}

void TaskQueueManage::addFileSystemManageTaskToReadyQueue(OrdinaryTransferTask *filesystemmanagetask)
{

    pthread_spin_lock(&this->fileSystemManageTaskQueueMutex);
    cout<<"add file system manage task!"<<endl;

    fileSystemManageQueue.push_back(filesystemmanagetask);

    pthread_spin_unlock(&this->fileSystemManageTaskQueueMutex);
    emit this->processTransferTaskSig(this);
}

OrdinaryTransferTask *TaskQueueManage::getFileSystemManageTask()
{
    pthread_spin_lock(&this->fileSystemManageTaskQueueMutex);

    if(fileSystemManageQueue.begin()!=fileSystemManageQueue.end())
    {
        auto s=fileSystemManageQueue.front();
        fileSystemManageQueue.pop_front();

        pthread_spin_unlock(&this->fileSystemManageTaskQueueMutex);
        return s;
    }
    pthread_spin_unlock(&this->fileSystemManageTaskQueueMutex);
    return nullptr;
}












