#ifndef ORDINARYTRANSFERTASK_H
#define ORDINARYTRANSFERTASK_H

#include <string>
#include <iostream>
using namespace std;
#include <QObject>
#include <pthread.h>
#include <event2/event.h>
#include <event2/listener.h>
#include <event2/http.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/thread.h>
#include <event2/visibility.h>
#include <event2/event-config.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#ifndef _WIN32
#include <signal.h>
#endif
#include <QDebug>
#include <QStandardItemModel>
#include <string>
#include <vector>
#include <string.h>
#include <stack>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <QDir>
#include <unistd.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <QMetaType>
#include <QtNetwork/QNetworkInterface>
#include <QFileInfo>
#include <QString>
#include <QDateTime>
#include "transfertask.h"
using namespace std;
static const char*fileRootDir="/home/xuyanming33/qttest/fileSystem/FileSystem";
static char* addr="127.0.0.1";
static const int SCHEDULESENDCOUNT=20;
class SuperTransferTask;
class OrdinaryTransferTask;
class TaskQueueManage;
struct FileInFolder;
struct cbSet
{
    event_base* base;
    OrdinaryTransferTask* transfertask=nullptr;
    evhttp_connection *evconnect=nullptr;
    evhttp_connection *stopevconnect=nullptr;
    int getfilefd=-1;
    string filename="";
    string foldername="";
    size_t addsize;
//重构的变量
    size_t currentdatasegmentlength=0;
    size_t datasegmentlength=0;
    evbuffer *buff=nullptr;
    int listeport=0;
    uint32_t listenaddress=0;

    //文件夹传输
    char* folderFileName=nullptr;
    char* folderFileSize=nullptr;
    FileInFolder *fileinfolder=nullptr;
    string currentfilename;
    int currentfilefd=-1;

    bool iftaskcontinue=true;

    struct event* timeout_event=nullptr;

        //设置初始状态
    size_t currentfileoffset=0;
    size_t currentfilesize=0;

    //此次传输所使用的tcp连接编号
    int tcplinknum=-1;
    bool ifuploadsuspend=false;
    bufferevent* bev=nullptr;
//兼容旧接口
    bool fileOrDir;
    size_t filesize=0;
    size_t transfersize=0;    
    string uri;
    const char* fileOrDirName;
    int sigCount=0;
    string storagepath;
    bool systemorfolder;
    vector<char*> dirinfor;
    ~cbSet()
    {
        if(buff)
            evbuffer_free(buff);
        if(fileinfolder)
            delete fileinfolder;
    }
};

struct FileInFolder
{
    //作为发送方辅助生成文件系列以发送数据
    FileInFolder(string folder);
    //作为接收方建立文件夹和辅助生成文件系列以接收数据
    FileInFolder(string folder,char* foldernamestring,char* foldersizestring);
    //建立文件夹作为容器承接数据
    void buildFolder(string folder, char* namestring);
    ~FileInFolder();
    char* filenamestring;//获取文件名的字符串
    char* filenameitem=nullptr;//文件名的字符串解析进度(慢指针)
    char* filenamespecialSymbool=nullptr;//文件名的字符串解析进度(快指针)

    char* filesizestring;//获取文件大小的字符串
    char* filesizeitem=nullptr;//文件名的字符串解析进度(慢指针)
    char* filesizespecialSymbool=nullptr;//文件名的字符串解析进度(快指针)

    stack<string> folderStack;//目录进度栈
};

class ListenPort
{
private:
    vector<pair<int,int>> LISTENPORTS={{8001,0},{8002,0},{8003,0},{8004,0},{8005,0},{8006,0}};
    pthread_spinlock_t listenPortMutex;
public:
    ListenPort();
    ~ListenPort();
    int findUnusedPort();
    void giveBackPort(int port);
};

class OrdinaryTransferTask : public QObject,public TransferTask
{
    Q_OBJECT
public:
    explicit OrdinaryTransferTask(QObject *parent = nullptr);
    virtual ~OrdinaryTransferTask();

    //传输任务初始化
    virtual void transferInit(string taskType,string transfertasktype,string transfertype,string sender,
                              string reciver,size_t offset,string length) override;
    void retransferInit();
    //任务执行
    virtual void taskExecution() override;

    //传输级别(是超级任务还是普通任务)
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

    //设置任务类型
    void setTaskType(string tasktype);

    //设置父任务
    void setParentTask(SuperTransferTask* parenttask);
    //获得父任务
    SuperTransferTask* getParentTask();

    //设置ui线程
    void setUiThread(QThread* uithread);

    //主线程对进度加锁（暂停）
    void lockTaskSchedule() ;
    //主线程对进度解锁（开始）
    void unlockTaskSchedule() ;
    //工作线程尝试加锁开始工作
    bool ifScheduleLocked() ;

    //获取信息(GetInformation)
    void GetInfoTaskInit(string informationType,string targetInformationPath);
    //提交指令(PushInstruct)
    void PushInstInit(string targetInstructPath);
    void PushInstInit(string targetInstructPath,string emptyFolder);

signals:
    //发送进度
    void sendSchedule(size_t file_size,size_t send_size);
    //暂停信号
    void task_break(TransferTask*trantask,TaskScheduleShowd* scheDule);
    //发送任务已经结束的信号
    void task_end(TransferTask*trantask,TaskScheduleShowd* scheDule);
    //发送目录信息给主页面
    void dirMessage(QStandardItemModel *model);
    //发送文件夹信息给主页面
    void folderMessage(QString folder,QString rootdirname);

private:
    //各类型任务所需成员变量
    //文件数据段传输(FileSegmentTransfer) 文件夹数据段传输(FolderSegmentTransfer) 获取信息(GetInformation) 提交指令(PushInstruct)
    string m_taskType;//任务类型

    //文件数据段传输
        //传输类型 上传文件(uploadfile) 上传文件夹(uploadfolder) 下载文件(downloadfile) 下载传文件夹(downloadfolder)
        string m_transfer_type="";
        //传输任务类型  超级传输任务子任务(SuperTransferSubtask) 普通传输任务(NormalTransferTask)
        string m_transfer_task_type="NormalTransferTask";
        //传输过程变量
        string m_sender;//文件传输的发送者
        string m_reciver;//文件传输的接收者
        size_t m_total_transfer_bytes;//文件传输的总字节数
        size_t m_transferred_bytes;//已传输字节数
        //任务暂停--启动
        pthread_mutex_t m_scheduleMutex;//暂停启动锁
        bool m_scheduleIsLock;//暂停 可启动状态
        //文件数据段 文件夹数据段(重启机制)
        size_t m_begin_offset=0;
        size_t m_this_time_begin_offset=0;
        size_t m_current_offset=0;
        size_t m_current_data_segment_length=0;
        string m_data_segment_length="OffsetToEnd";
        //超级传输任务子任务
        SuperTransferTask* m_parent_task;

    //获取信息
        string m_information_type;//想要获取的信息类型: 文件(File) 文件夹(Folder) 文件系统(FileSystem)
        string m_target_information_path;//目标信息路径
        //文件
        size_t m_file_size;//文件大小
        //文件夹
        string m_folder_string;//文件夹字符串
        string m_folder_name;//文件夹名字
        size_t m_folder_size;//文件夹大小
        //文件系统
        vector<char*> m_file_system_vector;//文件系统各项信息
        //ui线程
        QThread* m_uithread;

    //提交指令
        string m_instruct_type;//指令类型 添加空目录(AddEmptyFolder) 删除文件/文件夹(DeleteFileOrFolder)
        string m_target_instruct_path;//目标指令路径
        //添加的空目录名
        string m_empty_folder;

        static const int FILESIZE=1024;
        //网络传输缓冲区和端口参数
        static const size_t BUFFERSIZE;
        static const int PORT;
        static ListenPort listenport;
        //决定进度管理的锁
        pthread_mutex_t scheduleMutex;
        bool scheduleIsLock;

public:
    //初始化不同类型的任务
    //文件数据段传输(FileSegmentTransfer)
    void FileSegmTranInit(string transfertasktype,string transfertype, string sender, string reciver,
                          size_t offset, string length);
    //文件夹数据段传输(FolderSegmentTransfer)
    void FolderSegmTranInit(string transfertasktype,string transfertype, string sender, string reciver,
                            size_t offset, string length);

    //文件数据传输
    //下载文件数据
    void downLoadFileData(OrdinaryTransferTask* transfertask);
    static void down_load_file_data_listen_cb(struct evconnlistener *listener,
                                              evutil_socket_t fd,
                                              struct sockaddr *addr,
                                              int len, void *ptr);
    static void down_load_file_data_listen_erro_cb(evconnlistener* listener,void* arg);
    static void down_load_file_data_bufferevent_read_cb(struct bufferevent *bev, void *arg);
    static void down_load_file_data_bufferevent_event_cb(struct bufferevent *bev, short events, void *arg);
    static void down_load_file_data_http_cb(struct evhttp_request *req, void *ctx);

    //上传文件数据
    void upLoadFileData(OrdinaryTransferTask* transfertask);
    static void up_load_file_data_listen_cb(struct evconnlistener *listener,
                                            evutil_socket_t fd,
                                            struct sockaddr *addr,
                                            int len, void *ptr);
    static void up_load_file_data_listen_erro_cb(evconnlistener* listener,void* arg);
    static void up_load_file_data_bufferevent_write_cb(struct bufferevent *bev, void *arg);
    static void up_load_file_data_bufferevent_event_cb(struct bufferevent *bev, short events, void *arg);
    static void up_load_file_data_http_cb(struct evhttp_request *req, void *ctx);

    //文件夹数据传输
    //辅助函数
        //生成文件夹信息字符串集
    static std::pair<char*,char*> getFolderInfoStrings(string folder);
        //获得文件
    static std::pair<string,size_t> getFileInfoFromFolder(FileInFolder* fileinfolder);
    //下载文件夹数据
    void downLoadFolderData(OrdinaryTransferTask* transfertask);
    static void down_load_folder_data_listen_cb(struct evconnlistener *listener,
                                                evutil_socket_t fd,
                                                struct sockaddr *addr,
                                                int len, void *ptr);
    static void down_load_folder_data_listen_erro_cb(evconnlistener* listener,void* arg);
    static void down_load_folder_data_bufferevent_read_cb(struct bufferevent *bev, void *arg);
    static void down_load_folder_data_bufferevent_event_cb(struct bufferevent *bev, short events, void *arg);
    static void down_load_folder_data_http_cb(struct evhttp_request *req, void *ctx);
    //上传文件夹数据
    void upLoadFolderData(OrdinaryTransferTask* transfertask);
    static void up_load_folder_data_listen_cb(struct evconnlistener *listener,
                                              evutil_socket_t fd,
                                              struct sockaddr *addr,
                                              int len, void *ptr);
    static void up_load_folder_data_listen_erro_cb(evconnlistener* listener,void* arg);
    static void up_load_folder_data_bufferevent_write_cb(struct bufferevent *bev, void *arg);
    static void up_load_folder_data_bufferevent_event_cb(struct bufferevent *bev, short events, void *arg);
    static void up_load_folder_data_http_cb(struct evhttp_request *req, void *ctx);

    //发送报文告知服务器要中断某个上传任务
    void suspendUploadTranferTask(int tcplinknum,size_t expectedtransmissionvolume,cbSet* cb_set);
    static void suspend_up_load_tranfer_task_http_cb(struct evhttp_request *req, void *ctx);

    //文件系统信息获取
    //文件信息
    void getFileMessageFromServer(OrdinaryTransferTask* transfertask);
    static void get_file_message_from_server_cb(struct evhttp_request *req, void *ctx);
    //文件夹信息
    void getFolderMessageFromServer(OrdinaryTransferTask* transfertask);
    static void get_folder_message_from_server_cb(struct evhttp_request *req, void *ctx);
    //文件系统信息
    void getFileSystemMessageFromServer(OrdinaryTransferTask* transfertask);
    static void get_file_system_message_from_server_cb(struct evhttp_request *req, void *ctx);

    //发出指令
    //添加空文件夹
    void addEmptyFolderToServer(OrdinaryTransferTask* transfertask);
    static void add_empty_folder_to_server_cb(struct evhttp_request *req, void *ctx);
    //删除文件、文件夹
    int deleteFileOrFolderToServer(OrdinaryTransferTask* transfertask);
    static void delete_file_or_folder_to_server_cb(struct evhttp_request *req, void *ctx);

    //返回文件树信息
    static char* fileTreeInfoToString(QString path,int inforType);
    //获得文件大小字面值
    static QString getSizeText(size_t sizes);
    //统计文件夹文件字节总和
    static size_t getFolderSize(QString path);
    //响应定时器的回调函数
    static void timeout_cb(int fd,short event,void* argc);
    //传输任务中止
    static void transferTaskBreakOffProcess(cbSet *cb_set);
    //传输任务结束
    static void transferTaskEndProcess(cbSet *cb_set);
    //连接失败回调
    static void connclose_cb(evhttp_connection *evcon,void*arg);
    //报文错误回调
    static void reqerro_cb(enum evhttp_request_error er,void*arg);

    static string getFileNameFromUri(std::string uri);
    //用回传的字符串建立文件系统
    static QStandardItemModel *translateStringToModel(vector<char*> dirinfor);

//旧接口
public:
    int httpClientStart();
};


#endif // ORDINARYTRANSFERTASK_H
