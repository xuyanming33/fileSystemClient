#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);



    QFile file(":/qss/person.qss");
    file.open(QFile::ReadOnly);
    QTextStream filetext(&file);
    QString stylesheet=filetext.readAll();
    this->setStyleSheet(stylesheet);

    evthread_use_pthreads();
    qRegisterMetaType<QStandardItemModel *>("QStandardItemModel *");
    qRegisterMetaType<size_t>("size_t");
    qRegisterMetaType<TaskScheduleShowd *>("TaskScheduleShowd *");
    qRegisterMetaType<OrdinaryTransferTask *>("TransferTask *");

    //文件上传管理器
    uploadTaskManage=new TaskQueueManage();
    uploadTaskManage->init(ui->uploadTaskRunningListView,ui->uploadTaskReadyAndBlockListView);
    //总开关
    connect(ui->upLoadRunningStopPushButton,&QPushButton::clicked,
            uploadTaskManage,&TaskQueueManage::slotStopRunningQueue);
    connect(ui->upLoadReadyStopPushButton,&QPushButton::clicked,
            uploadTaskManage,&TaskQueueManage::slotStopReadyQueue);
    connect(ui->upLoadBlockStartPushButton,&QPushButton::clicked,
            uploadTaskManage,&TaskQueueManage::slotStartBlockQueue);

    //文件下载管理器
    downloadTaskManage=new TaskQueueManage();
    downloadTaskManage->init(ui->downloadTaskRunningListView,ui->downloadTaskReadyAndBlockListView);
    //总开关
    connect(ui->downLoadRunningStopPushButton,&QPushButton::clicked,
            downloadTaskManage,&TaskQueueManage::slotStopRunningQueue);
    connect(ui->downLoadReadyStopPushButton,&QPushButton::clicked,
            downloadTaskManage,&TaskQueueManage::slotStopReadyQueue);
    connect(ui->downLoadBlockStartPushButton,&QPushButton::clicked,
            downloadTaskManage,&TaskQueueManage::slotStartBlockQueue);

     //文件系统管理器
    fileSystemManage=new TaskQueueManage();
    downloadTaskManage->init(ui->downloadTaskRunningListView,ui->downloadTaskReadyAndBlockListView);

    //线程类对象
    workthreadmanage=new WorkThreadManage();
    workthreadmanage->init(uploadTaskManage,downloadTaskManage,fileSystemManage);

    updateFileSystemMessage();

    //初始化文件系统
    const char*path0="/xuyanming";

    //初始化已下载文件
    downloadSystem=new QFileSystemModel;
    downloadSystem->setRootPath("");
    auto rootModelIndex = downloadSystem->index(QString("/home/xuyanming33/qttest/fileSystem/FileSystem"));
    ui->downloadFileTreeView->setModel(downloadSystem);
    ui->downloadFileTreeView->setRootIndex(rootModelIndex);
    ui->downloadFileTreeView->setColumnWidth(0,600);
    ui->downloadFileTreeView->show();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::addNormalFileDownLoadTransferTask(string sender, string reciver)
{
    TransferTask* transfertask=new OrdinaryTransferTask();
    transfertask->transferInit("FileSegmentTransfer",
                               "NormalTransferTask",
                               "downloadfile",
                               sender,
                               reciver,
                               0,
                               string("OffsetToEnd"));
//    connect(dynamic_cast<OrdinaryTransferTask*>(transfertask),&OrdinaryTransferTask::task_end,
//            this,&MainWindow::slot_down_load_task_end_update_file_system,Qt::QueuedConnection);
    this->downloadTaskManage->addTransferTaskToReadyQueue(transfertask);
}

void MainWindow::addNormalFileUpLoadTransferTask(string sender, string reciver)
{
    TransferTask* transfertask=new OrdinaryTransferTask();
    transfertask->transferInit("FileSegmentTransfer",
                               "NormalTransferTask",
                               "uploadfile",
                               sender,
                               reciver,
                               0,
                               string("OffsetToEnd"));
    connect(dynamic_cast<OrdinaryTransferTask*>(transfertask),&OrdinaryTransferTask::task_end,
            this,&MainWindow::slot_down_load_task_end_update_file_system,Qt::QueuedConnection);
    this->uploadTaskManage->addTransferTaskToReadyQueue(transfertask);
    //emit dynamic_cast<OrdinaryTransferTask*>(transfertask)->task_end(transfertask,nullptr);
}

void MainWindow::addNormalFolderDownLoadTransferTask(string sender, string reciver)
{
    TransferTask* transfertask=new OrdinaryTransferTask();
    transfertask->transferInit("FolderSegmentTransfer",
                               "NormalTransferTask",
                               "downloadfolder",
                               sender,
                               reciver,
                               0,
                               string("OffsetToEnd"));
//    connect(dynamic_cast<OrdinaryTransferTask*>(transfertask),&OrdinaryTransferTask::task_end,
//            this,&MainWindow::slot_down_load_task_end_update_file_system,Qt::QueuedConnection);
    this->downloadTaskManage->addTransferTaskToReadyQueue(transfertask);
}

void MainWindow::addNormalFolderUpLoadTransferTask(string sender, string reciver)
{
    TransferTask* transfertask=new OrdinaryTransferTask();
    transfertask->transferInit("FolderSegmentTransfer",
                               "NormalTransferTask",
                               "uploadfolder",
                               sender,
                               reciver,
                               0,
                               string("OffsetToEnd"));
    connect(dynamic_cast<OrdinaryTransferTask*>(transfertask),&OrdinaryTransferTask::task_end,
            this,&MainWindow::slot_down_load_task_end_update_file_system,Qt::QueuedConnection);
    this->uploadTaskManage->addTransferTaskToReadyQueue(transfertask);
}

void MainWindow::addSuperFileDownLoadTransferTask(string sender, string reciver)
{
    OrdinaryTransferTask* superfiledownloadtask=new OrdinaryTransferTask();
    superfiledownloadtask->transferInit("FileSegmentTransfer",
                                      "SuperTransferTask",
                                      "downloadfile",
                                      sender,
                                      reciver,
                                      0,
                                      string("OffsetToEnd"));
    superfiledownloadtask->GetInfoTaskInit("File",sender);
    superfiledownloadtask->setUiThread(QThread::currentThread());
    connect(superfiledownloadtask,&OrdinaryTransferTask::task_end,
            this,&MainWindow::slot_super_down_load_transfer_task,Qt::QueuedConnection);
    fileSystemManage->addFileSystemManageTaskToReadyQueue(superfiledownloadtask);


}

void MainWindow::addSuperFileUpLoadTransferTask(string sender, string reciver)
{
    SuperTransferTask* transfertask=new SuperTransferTask();
    transfertask->transferInit("FileSegmentTransfer",
                               "SuperTransferTask",
                               "uploadfile",
                               sender,
                               reciver,
                               0,
                               string("OffsetToEnd"));
    transfertask->setTimer();
    connect(dynamic_cast<SuperTransferTask*>(transfertask),&SuperTransferTask::task_end,
            this,&MainWindow::slot_down_load_task_end_update_file_system,Qt::QueuedConnection);
    uploadTaskManage->addTransferTaskToReadyQueue(transfertask);
}

void MainWindow::addSuperFolderDownLoadTransferTask(string sender, string reciver)
{
    OrdinaryTransferTask* superfolderdownloadtask=new OrdinaryTransferTask();
    superfolderdownloadtask->transferInit("FolderSegmentTransfer",
                                          "SuperTransferTask",
                                          "downloadfolder",
                                          sender,
                                          reciver,
                                          0,
                                          string("OffsetToEnd"));
    superfolderdownloadtask->GetInfoTaskInit("Folder",sender);
    superfolderdownloadtask->setUiThread(QThread::currentThread());
    connect(superfolderdownloadtask,&OrdinaryTransferTask::task_end,
            this,&MainWindow::slot_super_down_load_transfer_task,Qt::QueuedConnection);
    fileSystemManage->addFileSystemManageTaskToReadyQueue(superfolderdownloadtask);

}

void MainWindow::addSuperFolderUpLoadTransferTask(string sender, string reciver)
{
    SuperTransferTask* transfertask=new SuperTransferTask();
    transfertask->transferInit("FolderSegmentTransfer",
                               "SuperTransferTask",
                               "uploadfolder",
                               sender,
                               reciver,
                               0,
                               string("OffsetToEnd"));
    transfertask->setTimer();
    connect(dynamic_cast<SuperTransferTask*>(transfertask),&SuperTransferTask::task_end,
            this,&MainWindow::slot_down_load_task_end_update_file_system,Qt::QueuedConnection);
    this->uploadTaskManage->addTransferTaskToReadyQueue(transfertask);
}

void MainWindow::updateFileSystemMessage()
{
    OrdinaryTransferTask* updatetask=new OrdinaryTransferTask();
    updatetask->GetInfoTaskInit("FileSystem","/xuyanming");
    connect(updatetask,&OrdinaryTransferTask::dirMessage,this,
            &MainWindow::slot_update_file_system,Qt::QueuedConnection);
    fileSystemManage->addFileSystemManageTaskToReadyQueue(updatetask);
}

void MainWindow::on_fileSystemTreeView_doubleClicked(const QModelIndex &index)
{
    FileManageDialog* filemanagedialog=new FileManageDialog(this);


    //方法、uri、文件/目录名、文件/目录
    if(index.column()!=0)
        return;
    QModelIndex m_index=index;
    string uri=m_index.data().toString().toStdString();//被选择的线上路径
    QString selectname=m_index.data().toString();//被选中的名字
    string path="";//被选择的线上父路径
    if(!this->ifSelectItemIsFileOrFolder(selectname))
        filemanagedialog->addChoose();
    filemanagedialog->exec();//以模态方式显示对话框

    int callselection=filemanagedialog->callselection;//选择的操作   
    string fileOrDirname=filemanagedialog->fileOrDirname;//对话框里得到的线下路径

    //模态对话框对变量初始化
    QModelIndex sindex=m_index;
    while(sindex.parent()!=fileSystemmodel->invisibleRootItem()->index())
    {
        sindex=sindex.parent();
        uri=sindex.data().toString().toStdString()+"/"+uri;
        path=sindex.data().toString().toStdString()+"/"+path;
    }
    uri="/xuyanming/"+uri;
    if(path=="")
        path="/xuyanming"+path;
    else
        path="/xuyanming/"+path;

    cout<<"uri:"<<uri<<endl;
    cout<<"path:"<<path<<endl;

    OrdinaryTransferTask* filesysmanagetask=new OrdinaryTransferTask();

    //string transferlevel="";
    QString dlgTitle="选择传输级别";
    QString strInfo="是否使用超级传输";
    QMessageBox::StandardButton defaultBtn=QMessageBox::NoButton;
    QMessageBox::StandardButton result;

    //执行任务
    switch (callselection) {

    //更新文件系统
    case 1:
        updateFileSystemMessage();
        break;

    //添加空目录
    case 2:
        filesysmanagetask->PushInstInit(path.c_str(),fileOrDirname.c_str());
        fileSystemManage->addFileSystemManageTaskToReadyQueue(filesysmanagetask);
        updateFileSystemMessage();
        break;

    //上传文件夹(该目录下)
    case 3:
        result=QMessageBox::question(this,dlgTitle,strInfo,QMessageBox::Yes|QMessageBox::No,defaultBtn);
        if(result==QMessageBox::No)
            addNormalFolderUpLoadTransferTask(fileOrDirname.c_str(),uri.c_str());
        else
            addSuperFolderUpLoadTransferTask(fileOrDirname.c_str(),uri.c_str());
        break;

    //上传文件(该目录下)
    case 4:
        result=QMessageBox::question(this,dlgTitle,strInfo,QMessageBox::Yes|QMessageBox::No,defaultBtn);
        if(result==QMessageBox::No)
            addNormalFileUpLoadTransferTask(fileOrDirname.c_str(),uri.c_str());
        else
            addSuperFileUpLoadTransferTask(fileOrDirname.c_str(),uri.c_str());
        break;

    //上传文件夹(同级目录)
    case 5:
        result=QMessageBox::question(this,dlgTitle,strInfo,QMessageBox::Yes|QMessageBox::No,defaultBtn);
        if(result==QMessageBox::No)
            addNormalFolderUpLoadTransferTask(fileOrDirname.c_str(),path.c_str());
        else
            addSuperFolderUpLoadTransferTask(fileOrDirname.c_str(),path.c_str());
        break;

    //上传文件(同级目录)
    case 6:
        result=QMessageBox::question(this,dlgTitle,strInfo,QMessageBox::Yes|QMessageBox::No,defaultBtn);
        if(result==QMessageBox::No)
            addNormalFileUpLoadTransferTask(fileOrDirname.c_str(),path.c_str());
        else
            addSuperFileUpLoadTransferTask(fileOrDirname.c_str(),path.c_str());
        break;

    //下载文件夹/文件
    case 7:
        if(ifSelectItemIsFileOrFolder(selectname))
        {
            result=QMessageBox::question(this,dlgTitle,strInfo,QMessageBox::Yes|QMessageBox::No,defaultBtn);
            if(result==QMessageBox::No)
                addNormalFileDownLoadTransferTask(uri.c_str(),fileOrDirname.c_str());
            else
                addSuperFileDownLoadTransferTask(uri.c_str(),fileOrDirname.c_str());
        }
        else
        {
            result=QMessageBox::question(this,dlgTitle,strInfo,QMessageBox::Yes|QMessageBox::No,defaultBtn);
            if(result==QMessageBox::No)
                addNormalFolderDownLoadTransferTask(uri.c_str(),fileOrDirname.c_str());
            else
                addSuperFolderDownLoadTransferTask(uri.c_str(),fileOrDirname.c_str());
        }
        break;

    //删除文件夹/文件
    case 8:
        filesysmanagetask->PushInstInit(uri.c_str());
        fileSystemManage->addFileSystemManageTaskToReadyQueue(filesysmanagetask);
        updateFileSystemMessage();
        break;
    default:
        break;
    }

    delete filemanagedialog;
}

void MainWindow::slot_update_file_system(QStandardItemModel *model)
{
    qDebug()<<"model change!";
    fileSystemmodel=model;
    ui->fileSystemTreeView->setModel(model);
    ui->fileSystemTreeView->setColumnWidth(0,600);
    ui->fileSystemTreeView->setEditTriggers(QTreeView::NoEditTriggers);//设置单元格不能编辑
    ui->fileSystemTreeView->show();
}

void MainWindow::slot_super_down_load_transfer_task(TransferTask *trantask, TaskScheduleShowd *scheDule)
{
    cout<<"super download!"<<endl;
    dynamic_cast<SuperTransferTask*>(trantask)->setTimer();

    connect(dynamic_cast<SuperTransferTask*>(trantask),&SuperTransferTask::task_end,
            this,&MainWindow::slot_down_load_task_end_update_file_system,Qt::QueuedConnection);

    this->downloadTaskManage->addTransferTaskToReadyQueue(trantask);
}

void MainWindow::slot_down_load_task_end_update_file_system(TransferTask *trantask, TaskScheduleShowd *scheDule)
{
    qDebug()<<"transfer task end,update file system!";
    updateFileSystemMessage();
}

bool MainWindow::ifSelectItemIsFileOrFolder(QString selectname)
{
    string s=selectname.toStdString();
    char sn[s.size()+1]={0};
    strcpy(sn,s.c_str());

    char* q=nullptr;
    q=strpbrk(sn,".");

    if(q)
        return true;
    else
        return false;
}


