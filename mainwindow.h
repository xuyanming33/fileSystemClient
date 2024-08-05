#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
#include <QScrollArea>
#include <QMetaType>
#include <QFileSystemModel>
#include <QMessageBox>

#include "workthreadmanage.h"
#include "filemanagedialog.h"
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    //普通文件下载
    void addNormalFileDownLoadTransferTask(string sender,string reciver);

    //普通文件上传
    void addNormalFileUpLoadTransferTask(string sender,string reciver);

    //普通文件夹下载
    void addNormalFolderDownLoadTransferTask(string sender,string reciver);

    //普通文件夹上传
    void addNormalFolderUpLoadTransferTask(string sender,string reciver);

    //超级文件下载
    void addSuperFileDownLoadTransferTask(string sender,string reciver);

    //超级文件上传
    void addSuperFileUpLoadTransferTask(string sender,string reciver);

    //超级文件夹下载
    void addSuperFolderDownLoadTransferTask(string sender,string reciver);

    //超级文件夹上传
    void addSuperFolderUpLoadTransferTask(string sender,string reciver);

    //更新文件系统信息
    void updateFileSystemMessage();

//    //获取文件信息
//    void getFileMessage(string targetInformationPath);

//    //获取文件夹信息
//    void getFolderMessage(string targetInformationPath);

    //添加空目录
    void addEmptyFolder(string targetInstructPath, string emptyFolder);

    //删除文件
    void deleteFile(string targetInstructPath);

    //删除文件夹
    void deleteFolder(string targetInstructPath);

private slots:
    //获取操作页面
    void on_fileSystemTreeView_doubleClicked(const QModelIndex &index);

    //更新文件系统
    void slot_update_file_system(QStandardItemModel *model);

    //响应信号，执行超级下载任务
    void slot_super_down_load_transfer_task(TransferTask*trantask,TaskScheduleShowd* scheDule);

    //传输任务结束，更新文件系统
    void slot_down_load_task_end_update_file_system(TransferTask*trantask,TaskScheduleShowd* scheDule);

private:
    Ui::MainWindow *ui;
    //下载文件管理
    TaskQueueManage* downloadTaskManage;
    //上传文件管理
    TaskQueueManage* uploadTaskManage;
    //文件系统管理器
    TaskQueueManage* fileSystemManage;
    //文件传输
    WorkThreadManage* workthreadmanage;



    //上传目录、下载目录、删除文件、删除目录
    OrdinaryTransferTask* dirAndDeleteManage;
    //已下载文件管理
    QFileSystemModel* downloadSystem;
    //文件系统
    QStandardItemModel *fileSystemmodel;


    bool ifSelectItemIsFileOrFolder(QString selectname);
};
#endif // MAINWINDOW_H
