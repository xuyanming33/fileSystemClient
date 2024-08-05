#include "workthreadmanage.h"
WorkThreadManage::WorkThreadManage(QObject *parent)
    : QObject{parent}
{

}

void WorkThreadManage::init(TaskQueueManage *uptqm, TaskQueueManage* downtqm, TaskQueueManage *filesysm)
{
    this->upLoadTaskManage=uptqm;
    this->downloadTaskManage=downtqm;
    this->fileSystemManage=filesysm;

    //启动阻塞任务时管理器向线程类要资源
    connect(upLoadTaskManage,&TaskQueueManage::processTransferTaskSig,
            this,&WorkThreadManage::processTransferTask);    
    connect(downloadTaskManage,&TaskQueueManage::processTransferTaskSig,
            this,&WorkThreadManage::processTransferTask);
    connect(fileSystemManage,&TaskQueueManage::processTransferTaskSig,
            this,&WorkThreadManage::processTransferTask);
}

TransferTask* WorkThreadManage::addTransferTask
    (string taskType, string transfertasktype, string transfertype,
        string sender, string reciver, size_t offset, string length, SuperTransferTask *parenttask)
{
    TaskQueueManage* taskquema=nullptr;
    if(transfertype=="uploadfolder"||transfertype=="uploadfile")
        taskquema=this->upLoadTaskManage;
    else if(transfertype=="downloadfile"||transfertype=="downloadfolder")
        taskquema=this->downloadTaskManage;
    else
    {
        cout<<"addTransferTask wrong!"<<endl;
        return nullptr;
    }

    if(taskquema==nullptr)
        cout<<"taskquema wrong!"<<endl;

    pthread_spin_lock(&taskquema->transferTaskQueueMutex);

    //添加任务
    TransferTask* addtask=
        taskquema->addTransferTaskToReadyQueue
           (taskType,transfertasktype,transfertype,sender,reciver,offset,length,this,parenttask);
    int taskcount=taskquema->readyQueue.size();
    if(taskcount>0)
    {
        if(threadscount<TASKTHREADMAXCOUNT)
        {


            int addcount=min(taskcount,int(TASKTHREADMAXCOUNT-threadscount));
            if(transfertype=="uploadfolder"||transfertype=="uploadfile")
                addcount=min(addcount,int(UPLOADTASKTHREADMAXCOUNT-uploadthreadscount));
            else if(transfertype=="downloadfile"||transfertype=="downloadfolder")
                addcount=min(addcount,int(DOWNLOADTASKTHREADMAXCOUNT-downloadthreadscount));
            while(addcount>0)
            {
                pthread_t t;
                pair<TaskQueueManage*,WorkThreadManage*> *workthreadarg=
                    new pair<TaskQueueManage*,WorkThreadManage*>(taskquema,this);
                pthread_create
                    (&t,nullptr,&WorkThreadManage::taskWorkerProcess,static_cast<void*>(workthreadarg));
                pthread_detach(t);
                threadscount++;
                addcount--;
                if(transfertype=="uploadfolder"||transfertype=="uploadfile")
                    uploadthreadscount++;
                else if(transfertype=="downloadfile"||transfertype=="downloadfolder")
                    downloadthreadscount++;
            }
        }
    }

    pthread_spin_unlock(&taskquema->transferTaskQueueMutex);
    return addtask;
}


void *WorkThreadManage::taskWorkerProcess(void *arg)
{
    pair<TaskQueueManage*,WorkThreadManage*> *workthreadarg=
        static_cast<pair<TaskQueueManage*,WorkThreadManage*>*>(arg);
    TaskQueueManage*taskqueman=workthreadarg->first;
    cout<<QDateTime::currentDateTime().toString().toStdString()<<"Thread "<<QThread::currentThreadId()<<" is exec!"<<endl;
    while(true)
    {
        TransferTask* transfertask=nullptr;
        //获取任务
        if(taskqueman==workthreadarg->second->downloadTaskManage||taskqueman==workthreadarg->second->upLoadTaskManage)
            transfertask=taskqueman->transferTaskFromReadyQueueToRunningQueue();
        else if(taskqueman==workthreadarg->second->fileSystemManage)
            transfertask=taskqueman->getFileSystemManageTask();
        //执行任务
        if(transfertask)
        {
            transfertask->taskExecution();
            if(taskqueman==workthreadarg->second->fileSystemManage)
                delete transfertask;
        }
        else
        {
            pthread_spin_lock(&taskqueman->transferTaskQueueMutex);
            workthreadarg->second->threadscount--;
            if(taskqueman==workthreadarg->second->upLoadTaskManage)
                workthreadarg->second->uploadthreadscount--;
            else if(taskqueman==workthreadarg->second->downloadTaskManage)
                workthreadarg->second->downloadthreadscount--;
            else if(taskqueman==workthreadarg->second->fileSystemManage)
                workthreadarg->second->filesystemmanagethreadscount--;
            pthread_spin_unlock(&taskqueman->transferTaskQueueMutex);
            cout<<QDateTime::currentDateTime().toString().toStdString()<<"workerthread "<<QThread::currentThreadId()<<" exit!"<<endl;
            pthread_exit(nullptr);
        }
    }
}

void WorkThreadManage::processTransferTask(TaskQueueManage* taskquema)
{

    if(taskquema==upLoadTaskManage||taskquema==downloadTaskManage)
    {
        pthread_spin_lock(&taskquema->transferTaskQueueMutex);

        int taskcount=taskquema->readyQueue.size();
        cout<<"readyQueue size:"<<taskcount<<endl;
        if(taskcount>0)
        {
            if(threadscount<TASKTHREADMAXCOUNT)
            {
                int addcount=min(taskcount,int(TASKTHREADMAXCOUNT-threadscount));
                if(taskquema==upLoadTaskManage)
                    addcount=min(addcount,int(UPLOADTASKTHREADMAXCOUNT-uploadthreadscount));
                else if(taskquema==downloadTaskManage)
                    addcount=min(addcount,int(DOWNLOADTASKTHREADMAXCOUNT-downloadthreadscount));
                while(addcount>0)
                {
                    pthread_t t;
                    pair<TaskQueueManage*,WorkThreadManage*> *workthreadarg
                        =new pair<TaskQueueManage*,WorkThreadManage*>(taskquema,this);
                    pthread_create(&t,nullptr,&WorkThreadManage::taskWorkerProcess,
                                   static_cast<void*>(workthreadarg));
                    pthread_detach(t);
                    threadscount++;
                    addcount--;
                    if(taskquema==upLoadTaskManage)
                        uploadthreadscount++;
                    else if(taskquema==downloadTaskManage)
                        downloadthreadscount++;
                }
            }
        }

        pthread_spin_unlock(&taskquema->transferTaskQueueMutex);
    }
    else if(taskquema==fileSystemManage)
    {
        pthread_spin_lock(&taskquema->fileSystemManageTaskQueueMutex);

        int taskcount=taskquema->fileSystemManageQueue.size();
        cout<<"fileSystemManageQueue size:"<<taskcount<<endl;
        if(taskcount>0)
        {
            if(threadscount<TASKTHREADMAXCOUNT)
            {
                int addcount=min(taskcount,int(TASKTHREADMAXCOUNT-threadscount));
                addcount=min(addcount,int(FILESYSTEMMANAGETHREADMAXCOUNT-filesystemmanagethreadscount));
                while(addcount>0)
                {
                    pthread_t t;
                    pair<TaskQueueManage*,WorkThreadManage*> *workthreadarg
                        =new pair<TaskQueueManage*,WorkThreadManage*>(taskquema,this);
                    pthread_create(&t,nullptr,&WorkThreadManage::taskWorkerProcess,
                                   static_cast<void*>(workthreadarg));
                    pthread_detach(t);
                    threadscount++;
                    addcount--;
                    filesystemmanagethreadscount++;
                }
            }
        }

        pthread_spin_unlock(&taskquema->fileSystemManageTaskQueueMutex);
    }
}
