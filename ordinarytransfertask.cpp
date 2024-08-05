#include "ordinarytransfertask.h"
#include "supertransfertask.h"
#include "taskqueuemanage.h"
const size_t OrdinaryTransferTask::BUFFERSIZE=1024*4;
const int OrdinaryTransferTask::PORT = 8080;
ListenPort OrdinaryTransferTask::listenport;

FileInFolder::FileInFolder(string folder)
{
    pair<char*,char*> a=OrdinaryTransferTask::getFolderInfoStrings(folder);
    const char* symbols=",()";
    filenamestring=filenameitem=a.first;
    filenamespecialSymbool=strpbrk(filenameitem,symbols);
    filesizestring=filesizeitem=a.second;
    filesizespecialSymbool=strpbrk(filesizeitem,symbols);

    folderStack.push(string(folder.c_str()));
}

FileInFolder::FileInFolder(string folder, char *foldernamestring, char *foldersizestring)
{
    const char* symbols=",()";
    filenamestring=filenameitem=foldernamestring;
    filenamespecialSymbool=strpbrk(filenameitem,symbols);
    filesizestring=filesizeitem=foldersizestring;
    filesizespecialSymbool=strpbrk(filesizeitem,symbols);

    folderStack.push(string(folder.c_str()));
    //建立文件夹作为容器承接数据
    buildFolder(folder,foldernamestring);
}

void FileInFolder::buildFolder(string folder, char *namestring)
{
    const char* symbols=",()";

    string tem=namestring;
    char* foldernamestring=new char[tem.size()+1];
    strcpy(foldernamestring,tem.c_str());

    //char* namestring=foldernamestring;//获取文件名的字符串
    char* nameitem=foldernamestring;//文件名的字符串解析进度(慢指针)
    char* namespecialSymbool=strpbrk(nameitem,symbols);;//文件名的字符串解析进度(快指针)
    stack<string> currentfolderStack;//目录进度栈
    currentfolderStack.push(folder);

    while(namespecialSymbool)
    {
        QDir currentDir(QString(currentfolderStack.top().c_str()));
        switch (*namespecialSymbool)
        {
        case ',':
            *namespecialSymbool='\0';
            if(nameitem!=namespecialSymbool)
            {
                //currentDir.mkdir(QString(nameitem));
                int fd=open((currentfolderStack.top()+"/"+string(nameitem)).c_str(),O_CREAT,0600);
                close(fd);
            }
            nameitem=namespecialSymbool+1;
            namespecialSymbool=strpbrk(nameitem,symbols);
            break;
        case '(':
            *namespecialSymbool='\0';
            currentDir.mkdir(QString(nameitem));
            currentfolderStack.push(currentfolderStack.top()+"/"+string(nameitem));

            nameitem=namespecialSymbool+1;
            namespecialSymbool=strpbrk(nameitem,symbols);
            break;
        case ')':
            *namespecialSymbool='\0';
            if(nameitem!=namespecialSymbool)
            {
                //currentDir.mkdir(QString(nameitem));
                int fd=open((currentfolderStack.top()+"/"+string(nameitem)).c_str(),O_CREAT,0600);
                close(fd);
            }
            currentfolderStack.pop();
            nameitem=namespecialSymbool+1;
            namespecialSymbool=strpbrk(nameitem,symbols);
            break;
        default:
            return ;
        }
    }
    if(nameitem)
    {
        if(nameitem!=namespecialSymbool)
        {
            //currentDir.mkdir(QString(nameitem));
            int fd=open((currentfolderStack.top()+"/"+string(nameitem)).c_str(),O_CREAT,0600);
            close(fd);
        }
    }
    if(foldernamestring)
        delete[]foldernamestring;
}

FileInFolder::~FileInFolder()
{
    if(filenamestring)
        delete[] filenamestring;
    if(filesizestring)
        delete[] filesizestring;
}

ListenPort::ListenPort()
{
    pthread_spin_init(&listenPortMutex,PTHREAD_PROCESS_PRIVATE);
}

ListenPort::~ListenPort()
{
    pthread_spin_destroy(&listenPortMutex);
}

int ListenPort::findUnusedPort()
{
    pthread_spin_lock(&listenPortMutex);
    vector<pair<int,int>>::iterator result=
        find_if(LISTENPORTS.begin(),LISTENPORTS.end(),[](std::pair<int,int> tas)
                {return tas.second==0;});
    result->second=1;
    if(result==LISTENPORTS.end())
    {
        pthread_spin_unlock(&listenPortMutex);
        return 0;
    }
    int port=result->first;
    pthread_spin_unlock(&listenPortMutex);
    return port;
}

void ListenPort::giveBackPort(int port)
{
    pthread_spin_lock(&listenPortMutex);
    vector<pair<int,int>>::iterator result=
        find_if(LISTENPORTS.begin(),LISTENPORTS.end(),[port](std::pair<int,int> tas)
                {return tas.first==port;});
    if(result==LISTENPORTS.end())
    {
        pthread_spin_unlock(&listenPortMutex);
        return;
    }
    result->second=0;
    pthread_spin_unlock(&listenPortMutex);
}

OrdinaryTransferTask::OrdinaryTransferTask(QObject *parent)
    : QObject{parent}
{

}

OrdinaryTransferTask::~OrdinaryTransferTask()
{
    pthread_mutex_destroy(&scheduleMutex);
}

void OrdinaryTransferTask::transferInit(string taskType,string transfertasktype,string transfertype,
                                        string sender,
                                        string reciver,size_t offset,string length)
{
    if(taskType=="FileSegmentTransfer")
        FileSegmTranInit(transfertasktype,transfertype,sender,reciver,offset,length);
    else if(taskType=="FolderSegmentTransfer")
        FolderSegmTranInit(transfertasktype,transfertype,sender,reciver,offset,length);
    pthread_mutex_init(&scheduleMutex,nullptr);
}

void OrdinaryTransferTask::retransferInit()
{
    transferInit(this->m_taskType,this->m_transfer_task_type,this->m_transfer_type,
                 this->m_sender,this->m_reciver,this->m_current_offset,
                 to_string(this->m_current_data_segment_length));
}

void OrdinaryTransferTask::taskExecution()
{
    if(m_taskType=="FileSegmentTransfer")
    {
        if(m_transfer_type=="downloadfile")
            downLoadFileData(this);
        else if(m_transfer_type=="uploadfile")
            upLoadFileData(this);
    }
    else if(m_taskType=="FolderSegmentTransfer")
    {
        if(m_transfer_type=="downloadfolder")
            downLoadFolderData(this);
        else if(m_transfer_type=="uploadfolder")
            upLoadFolderData(this);
    }
    else if(m_taskType=="GetInformation")
    {
        if(m_information_type=="File")
            getFileMessageFromServer(this);
        else if(m_information_type=="Folder")
            getFolderMessageFromServer(this);
        else if(m_information_type=="FileSystem")
            getFileSystemMessageFromServer(this);
    }
    else if(m_taskType=="PushInstruct")
    {
        if(m_instruct_type=="AddEmptyFolder")
            addEmptyFolderToServer(this);
        else if(m_instruct_type=="DeleteFileOrFolder")
            deleteFileOrFolderToServer(this);
    }
    else
        cout<<"init wrong!"<<endl;
}

void OrdinaryTransferTask::lockTaskSchedule()
{
//    int getmetux=1;
//    while(getmetux!=0)
//        getmetux=pthread_mutex_trylock(&scheduleMutex);
    pthread_mutex_lock(&scheduleMutex);
    scheduleIsLock=true;
}

void OrdinaryTransferTask::unlockTaskSchedule()
{
    pthread_mutex_unlock(&scheduleMutex);
    scheduleIsLock=false;
//    if(scheduleIsLock)
//    {
//        pthread_mutex_unlock(&scheduleMutex);
//        scheduleIsLock=false;
//    }
}

bool OrdinaryTransferTask::ifScheduleLocked()
{
    int getmetux=1;
    getmetux=pthread_mutex_trylock(&scheduleMutex);
    //cout<<"getmetux:"<<getmetux<<endl;
    if(getmetux==0)
    {
        pthread_mutex_unlock(&scheduleMutex);
        return false;
    }
    else
        return true;
}

string OrdinaryTransferTask::getTransferTaskType()
{
    //cout<<"OdinaryTask"<<endl;
    return m_transfer_task_type;
}

string OrdinaryTransferTask::getTaskType()
{
    return m_taskType;
}

string OrdinaryTransferTask::getTransferType()
{
    return m_transfer_type;
}

string OrdinaryTransferTask::getTransferSender()
{
    return m_sender;
}

string OrdinaryTransferTask::getTransferReciver()
{
    return m_reciver;
}

string OrdinaryTransferTask::getTransferLength()
{
    return m_data_segment_length;
}

void OrdinaryTransferTask::setTaskType(string tasktype)
{
    m_taskType=tasktype;
}

void OrdinaryTransferTask::setParentTask(SuperTransferTask *parenttask)
{
    m_parent_task=parenttask;
}

SuperTransferTask *OrdinaryTransferTask::getParentTask()
{
    return m_parent_task;
}

void OrdinaryTransferTask::setUiThread(QThread *uithread)
{
    m_uithread=uithread;
}

void OrdinaryTransferTask::GetInfoTaskInit(string informationType, string targetInformationPath)
{
    m_taskType="GetInformation";
    m_information_type=informationType;
    m_target_information_path=targetInformationPath;
}

void OrdinaryTransferTask::PushInstInit(string targetInstructPath)
{
    m_taskType="PushInstruct";
    m_instruct_type="DeleteFileOrFolder";
    m_target_instruct_path=targetInstructPath;
}

void OrdinaryTransferTask::PushInstInit(string targetInstructPath, string emptyFolder)
{
    m_taskType="PushInstruct";
    m_instruct_type="AddEmptyFolder";
    m_target_instruct_path=targetInstructPath;
    m_empty_folder=emptyFolder;
}

void OrdinaryTransferTask::FileSegmTranInit(string transfertasktype,string transfertype, string sender, string reciver,
                                            size_t offset, string length)
{
    m_taskType="FileSegmentTransfer";
    m_transfer_task_type=transfertasktype;
    m_transfer_type=transfertype;

    m_sender=sender;
    m_reciver=reciver;
    m_begin_offset=offset;
    m_this_time_begin_offset=offset;
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
}

void OrdinaryTransferTask::FolderSegmTranInit(string transfertasktype,string transfertype, string sender, string reciver,
                                              size_t offset, string length)
{
    m_taskType="FolderSegmentTransfer";
    m_transfer_task_type=transfertasktype;
    m_transfer_type=transfertype;

    m_sender=sender;
    m_reciver=reciver;
    m_begin_offset=offset;
    m_current_offset=offset;
    m_data_segment_length=length;

    if(length!="OffsetToEnd")
        sscanf(length.c_str(),"%zu",&m_current_data_segment_length);
    if(transfertype=="uploadfolder"&&length=="OffsetToEnd")
    {
        m_current_data_segment_length=getFolderSize(sender.c_str())-m_current_offset;
        m_data_segment_length=to_string(m_current_data_segment_length);
    }
}

void OrdinaryTransferTask::downLoadFileData(OrdinaryTransferTask *transfertask)
{
    //创建libevent的上下文
    event_base * base = event_base_new();
    if (base)
    {
        cout<< "event_base_new success!"<<endl ;
    }

    cbSet cbset;
    cbset.base=base;
    cbset.transfertask=transfertask;

    //创建文件
    string tem=transfertask->m_reciver+string("/")
                 +string(getFileNameFromUri(transfertask->m_sender.c_str()));
    cbset.filename=tem;
    cout<<"filename:"<<tem<<endl;
    cbset.getfilefd=open(tem.c_str(),O_WRONLY|O_CREAT,0600);
    cout<<"filefd: "<<cbset.getfilefd<<endl;

    //创建套接字绑定接收连接请求
    //获取监听端口，IP地址
    int listeport=listenport.findUnusedPort();
    cbset.listeport=listeport;
    QList<QHostAddress> ipAddresses = QNetworkInterface::allAddresses();
    uint32_t listenaddress=(*ipAddresses.cbegin()).toIPv4Address();
    struct sockaddr_in serv;
    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_port = htons(listeport);
    serv.sin_addr.s_addr = htonl(INADDR_ANY);

    struct evconnlistener* listener= evconnlistener_new_bind(base, down_load_file_data_listen_cb, &cbset,
                                                              LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE |
                                                              LEV_OPT_THREADSAFE,
                                                              36, (struct sockaddr*)&serv, sizeof(serv));
    evconnlistener_set_error_cb(listener,down_load_file_data_listen_erro_cb);
    if(listener)
        cout<<"bind sucess!"<<endl;
    else
        cout<<"bind failed!"<<endl;

    // bufferevent  连接http服务器
    bufferevent *bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);

    //创建一个 evhttp_connection 对象，表示基于网络连接的 HTTP 客户端
    evhttp_connection *evcon = evhttp_connection_base_bufferevent_new(base,NULL, bev, addr, PORT);
    cbset.evconnect=evcon;

    //设置回调
    evhttp_request *req = evhttp_request_new(down_load_file_data_http_cb, &cbset);
    evhttp_request_set_error_cb(req,reqerro_cb);
    evhttp_connection_set_closecb(evcon,connclose_cb,&cbset);

    // 设置请求的head 消息报头 信息
    evkeyvalq *output_headers = evhttp_request_get_output_headers(req);
    evhttp_add_header(output_headers,"MessageType","FileSegmentTransfer");
    evhttp_add_header(output_headers,"FileOffset",to_string(transfertask->m_current_offset).c_str());

    //判断是否是暂停后重启的任务
    if(transfertask->m_begin_offset==transfertask->m_current_offset)//初次启动
        evhttp_add_header(output_headers, "FileSegmentLength",transfertask->m_data_segment_length.c_str());
    else
        evhttp_add_header(output_headers, "FileSegmentLength",
                          to_string(transfertask->m_current_data_segment_length).c_str());
    evhttp_add_header(output_headers,"Client-ip",to_string(listenaddress).c_str());
    evhttp_add_header(output_headers,"Client-port",to_string(listeport).c_str());

    //向指定的 evhttp_connection 对象发起 HTTP 请求
    evhttp_make_request(evcon, req, EVHTTP_REQ_GET, transfertask->m_sender.c_str());

    //设置定时器
    timeval tv={1,0};
    struct event* timeout_event=evtimer_new(base,timeout_cb,&cbset);
    cbset.timeout_event=timeout_event;
    event_add(timeout_event,&tv);

    //事件分发处理
    if(base)
        event_base_dispatch(base);

    if(cbset.transfertask->m_current_data_segment_length==0)
        transferTaskEndProcess(&cbset);
    else
        transferTaskBreakOffProcess(&cbset);
//    if(evcon)
//        evhttp_connection_free(evcon);
//    if(listener)
//        evconnlistener_free(listener);
    if(base)
        event_base_free(base);
    //qDebug()<<"task end!";
    return ;
}

void OrdinaryTransferTask::down_load_file_data_listen_cb(evconnlistener *listener, int fd, sockaddr *addr, int len, void *ptr)
{
    cout<<"connect new client!"<<endl;
    cbSet *cb_set=static_cast<cbSet*>(ptr);

    // 通信操作
    // 添加新事件
    struct bufferevent *bev= bufferevent_socket_new(cb_set->base, fd,
                                                     BEV_OPT_CLOSE_ON_FREE|BEV_OPT_DEFER_CALLBACKS);
    cb_set->bev=bev;
    bufferevent_setwatermark(bev,EV_READ,0,BUFFERSIZE);

    // 给bufferevent缓冲区设置回调
    bufferevent_setcb(bev, down_load_file_data_bufferevent_read_cb, nullptr,
                      down_load_file_data_bufferevent_event_cb,cb_set);
    bufferevent_enable(bev, EV_WRITE|EV_READ);

    if(listener)
        evconnlistener_free(listener);
}

void OrdinaryTransferTask::down_load_file_data_listen_erro_cb(evconnlistener *listener, void *arg)
{
    cout<<"listen failed!"<<endl;
}

void OrdinaryTransferTask::down_load_file_data_bufferevent_read_cb(bufferevent *bev, void *arg)
{
    cbSet *cb_set=static_cast<cbSet*>(arg);
    struct evbuffer* inbuf=bufferevent_get_input(bev);
    if(cb_set->transfertask->m_current_offset==cb_set->transfertask->m_this_time_begin_offset)
        return;

    //把缓冲区数据写入文件
    cb_set->getfilefd=open(cb_set->filename.c_str(),O_WRONLY|O_CREAT,0600);
    lseek(cb_set->getfilefd,cb_set->transfertask->m_current_offset,SEEK_SET);
    size_t recivelen=evbuffer_get_length(inbuf);
    evbuffer_write(inbuf,cb_set->getfilefd);
    close(cb_set->getfilefd);

    //更新进度
    cb_set->transfertask->m_current_offset+=recivelen;
    cb_set->transfertask->m_current_data_segment_length=
        cb_set->datasegmentlength-(cb_set->transfertask->m_current_offset-cb_set->transfertask->m_begin_offset);
    if(cb_set->transfertask->m_transfer_task_type=="FolderTransferSubtask")
        cb_set->addsize+=recivelen;



    //测试是否加锁决定任务要不要继续
    if(cb_set->transfertask->ifScheduleLocked())
    {
        if(bev)
            bufferevent_free(bev);
        event_del(cb_set->timeout_event);
        //transferTaskBreakOffProcess(cb_set);
        //event_base_loopbreak(cb_set->base);

//        qDebug()<<"m_current_offset:"<<cb_set->transfertask->m_current_offset;
//        qDebug()<<"m_current_data_segment_length:"<<cb_set->transfertask->m_current_data_segment_length;
        cout<<"lock!"<<endl;
        return;
    }

    //任务结束
    if(cb_set->transfertask->m_current_data_segment_length==0)
    {
        if(bev)
            bufferevent_free(bev);
        cout<<"task end!"<<endl;

        //发送进度信号、结束信号
        //transferTaskEndProcess(cb_set);

        //event_base_loopbreak(cb_set->base);
    }
}

void OrdinaryTransferTask::down_load_file_data_bufferevent_event_cb(bufferevent *bev, short events, void *arg)
{
    if (events & BEV_EVENT_EOF)
    {
        cbSet *cb_set=static_cast<cbSet*>(arg);       

        //测试是否加锁决定任务要不要继续
        if(cb_set->transfertask->ifScheduleLocked())
        {
            //transferTaskBreakOffProcess(cb_set);
            if(bev)
                bufferevent_free(bev);

            //event_base_loopbreak(cb_set->base);
            return;
        }

        //任务结束
        if(cb_set->transfertask->m_current_data_segment_length==0)
        {
            if(bev)
                bufferevent_free(bev);
            cout<<"task end!"<<endl;

            //发送进度信号、结束信号
            //transferTaskEndProcess(cb_set);

            //event_base_loopbreak(cb_set->base);
        }

        struct evbuffer* inbuf=bufferevent_get_input(bev);

        if(cb_set->transfertask->m_current_offset==cb_set->transfertask->m_begin_offset)
            return;

        //把缓冲区数据写入文件
        cb_set->getfilefd=open(cb_set->filename.c_str(),O_WRONLY|O_CREAT,0600);
        lseek(cb_set->getfilefd,cb_set->transfertask->m_current_offset,SEEK_SET);
        size_t recivelen=evbuffer_get_length(inbuf);
        if(recivelen==0)
            return;
        cout<<"recivelen: "<<recivelen<<endl;
        evbuffer_write(inbuf,cb_set->getfilefd);
        close(cb_set->getfilefd);

        //更新进度
        cb_set->transfertask->m_current_offset+=recivelen;
        //cout<<"currentoffset: "<<cb_set->transfertask->m_current_offset<<endl;
        cb_set->transfertask->m_current_data_segment_length=
            cb_set->datasegmentlength-(cb_set->transfertask->m_current_offset-cb_set->transfertask->m_begin_offset);
        cout<<"currentfilesegmentlength: "<<cb_set->currentdatasegmentlength<<endl;
        if(cb_set->transfertask->m_transfer_task_type=="FolderTransferSubtask")
            cb_set->addsize+=recivelen;


        cout<<"connection closed\n"<<endl;
    }
    else if(events & BEV_EVENT_ERROR)
    {
        printf("some other error\n");
    }
    else if(events & BEV_EVENT_CONNECTED)
    {
        cout<<"已经连接...\\(^o^)/..."<<endl;
        return;
    }
    bufferevent_free(bev);
    printf("buffevent 资源已经被释放...\n");
}

void OrdinaryTransferTask::down_load_file_data_http_cb(evhttp_request *req, void *ctx)
{
    cout<<"down_load_file_date_http_cb have call!"<<endl;
    cbSet *cb_set=static_cast<cbSet*>(ctx);

    //获取状态
    if(!req)
        cout<<"failed to get respond"<<endl;
    //evconnect=evhttp_request_get_connection(req);
    //应答报文基本信息
    //响应状态码
    cout << "Response :" << evhttp_request_get_response_code(req)<<endl;
    //响应状态码行
    cout <<"Line:"<< evhttp_request_get_response_code_line(req) << endl;
    //uri
    const char *uri = evhttp_request_get_uri(req);
    cout<<"uri:"<<string(uri)<<endl;


    //应答报文首部行、缓冲区
    evkeyvalq *headers = evhttp_request_get_input_headers(req);
    evbuffer *inbuf = evhttp_request_get_input_buffer(req);

    //获取文件段的大小
    if(!evhttp_find_header(headers,"FileSegmentTransferMode"))
        return;
    if(!evhttp_find_header(headers,"FileSize"))
        return;
    size_t filesize=0;
    const char* filesizestr=evhttp_find_header(headers,"FileSize");
    sscanf(filesizestr,"%zu",&filesize);
    if(cb_set->transfertask->m_data_segment_length=="OffsetToEnd")
    {
        cb_set->transfertask->m_data_segment_length=to_string(filesize-cb_set->transfertask->m_begin_offset);
        cb_set->datasegmentlength=filesize-cb_set->transfertask->m_begin_offset;
    }
    else
    {
        sscanf(cb_set->transfertask->m_data_segment_length.c_str(),"%zu",&cb_set->datasegmentlength);
    }
    cout<<"filesegmentlength: "<<cb_set->datasegmentlength<<endl;

    //把缓冲区数据写入文件
    lseek(cb_set->getfilefd,cb_set->transfertask->m_current_offset,SEEK_SET);
    size_t recivelen=evbuffer_get_length(inbuf);
    cout<<"recivelen: "<<recivelen<<endl;
    evbuffer_write(inbuf,cb_set->getfilefd);

    //更新进度
    cb_set->transfertask->m_current_offset+=recivelen;
    cb_set->transfertask->m_current_data_segment_length=
        cb_set->datasegmentlength-(cb_set->transfertask->m_current_offset-cb_set->transfertask->m_begin_offset);
    if(cb_set->transfertask->m_transfer_task_type=="FolderTransferSubtask")
        cb_set->addsize+=recivelen;

    //HTTP模式的处理
    if(evhttp_find_header(headers,"FileSegmentTransferMode")==string("HTTPTransfer"))
    {
        cout<<"HTTPTransfer!"<<endl;
        //文件未传输完毕，发起新的请求
        if(cb_set->transfertask->m_current_data_segment_length!=0)
        {
            //测试是否加锁决定任务要不要继续
            if(cb_set->transfertask->ifScheduleLocked())
                //event_base_loopbreak(cb_set->base);
                return;

            //组装请求报文 回调函数设置
            evhttp_request *request = evhttp_request_new(down_load_file_data_http_cb, cb_set);

            // 设置请求的head 消息报头 信息
            QList<QHostAddress> ipAddresses = QNetworkInterface::allAddresses();
            string s=to_string((*ipAddresses.cbegin()).toIPv4Address());
            evkeyvalq *output_headers = evhttp_request_get_output_headers(req);
            evhttp_add_header(output_headers,"MessageType","FileSegmentTransfer");
            evhttp_add_header(output_headers,"FileOffset",to_string(cb_set->transfertask->m_current_offset).c_str());
            evhttp_add_header(output_headers, "FileSegmentLength",
                              to_string(cb_set->transfertask->m_current_data_segment_length).c_str());
            evhttp_add_header(output_headers,"Client-ip",s.c_str());
            evhttp_add_header(output_headers,"Client-port",to_string(listenport.findUnusedPort()).c_str());

            //向指定的 evhttp_connection 对象发起 HTTP 请求
            evhttp_make_request(cb_set->evconnect, request, EVHTTP_REQ_GET, uri);
        }
        //文件传输完毕
        else
        {
            //发送进度信号、结束信号
            //transferTaskEndProcess(cb_set);

            //event_base_loopbreak(cb_set->base);
        }
    }
    //TCP模式的处理
    else if(evhttp_find_header(headers,"FileSegmentTransferMode")==string("TCPTransfer"))
    {
        cout<<"TCPTransfer!"<<endl;
        //event_base_loopbreak(cb_set->base);

    }
    if(cb_set->evconnect)
        evhttp_connection_free(cb_set->evconnect);
}

void OrdinaryTransferTask::upLoadFileData(OrdinaryTransferTask *transfertask)
{
    //创建libevent的上下文
    event_base * base = event_base_new();
    if (base)
    {
        //qDebug() << "event_base_new success!\n" ;
    }

    cbSet cbset;
    cbset.base=base;
    cbset.transfertask=transfertask;

    //打开文件
    string tem=transfertask->m_sender;
    cbset.filename=tem;
    //cout<<"filename:"<<cbset.filename<<endl;
    cbset.buff=evbuffer_new();

    // 创建套接字 绑定 接收连接请求
    int listeport=listenport.findUnusedPort();
    cbset.listeport=listeport;
    QList<QHostAddress> ipAddresses = QNetworkInterface::allAddresses();
    uint32_t listenaddress=(*ipAddresses.cbegin()).toIPv4Address();
    struct sockaddr_in serv;
    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_port = htons(listeport);
    serv.sin_addr.s_addr = htonl(INADDR_ANY);

    struct evconnlistener* listener= evconnlistener_new_bind(base, up_load_file_data_listen_cb, &cbset,
                                                              LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE,
                                                              36, (struct sockaddr*)&serv, sizeof(serv));
    evconnlistener_set_error_cb(listener,up_load_file_data_listen_erro_cb);
    if(listener)
        cout<<"bind sucess!"<<endl;
    else
        cout<<"bind failed!"<<endl;

    // bufferevent  连接http服务器
    bufferevent *bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);

    //创建一个 evhttp_connection 对象，表示基于网络连接的 HTTP 客户端
    evhttp_connection *evcon = evhttp_connection_base_bufferevent_new(base,NULL, bev, addr, PORT);
    cbset.evconnect=evcon;

    //设置回调
    evhttp_request *req = evhttp_request_new(up_load_file_data_http_cb, &cbset);
    evhttp_request_set_error_cb(req,reqerro_cb);
    evhttp_connection_set_closecb(evcon,connclose_cb,&cbset);

    // 设置请求的head 消息报头 信息
    evkeyvalq *output_headers = evhttp_request_get_output_headers(req);
    evhttp_add_header(output_headers,"MessageType","FileSegmentTransfer");

    //HTTP模式没有任何优势，所以原先设计的双模式废止，使用TCP模式进行传输
    evhttp_add_header(output_headers,"FileSegmentTransferMode","TCPTransfer");
    evhttp_add_header(output_headers,"FileOffset",to_string(transfertask->m_current_offset).c_str());

    //判断是否是暂停后重启的任务
    if(transfertask->m_begin_offset==transfertask->m_current_offset)//初次启动
        evhttp_add_header(output_headers, "FileSegmentLength",transfertask->m_data_segment_length.c_str());
    else
        evhttp_add_header(output_headers, "FileSegmentLength",
                          to_string(transfertask->m_current_data_segment_length).c_str());
    evhttp_add_header(output_headers,"Client-ip",to_string(listenaddress).c_str());
    evhttp_add_header(output_headers,"Client-port",to_string(listeport).c_str());

    //报文携带数据
//    size_t len=min(BUFFERSIZE,transfertask->m_current_data_segment_length);
//    //cout<<"len:"<<len<<endl;
//    cbset.getfilefd=open(cbset.filename.c_str(),O_RDONLY);
//    //cout<<"filefd:"<<cbset.getfilefd<<endl;
//    evbuffer *output = evhttp_request_get_output_buffer(req);
//    evbuffer_add_file(output,cbset.getfilefd,cbset.transfertask->m_current_offset,len);

    //向指定的 evhttp_connection 对象发起 HTTP 请求
    QFileInfo fileinfo(QString(cbset.filename.c_str()));
    string s=fileinfo.fileName().toStdString();
    evhttp_make_request(evcon, req, EVHTTP_REQ_POST, (transfertask->m_reciver+"/"+s).c_str());

    //设置定时器
    timeval tv={1,0};
    struct event* timeout_event=evtimer_new(base,timeout_cb,&cbset);
    cbset.timeout_event=timeout_event;
    event_add(timeout_event,&tv);

    //事件分发处理
    if(base)
        event_base_dispatch(base);
    if(cbset.transfertask->m_current_data_segment_length==0)
        transferTaskEndProcess(&cbset);
    else
        transferTaskBreakOffProcess(&cbset);

//    if(evcon)
//        evhttp_connection_free(evcon);
//    if(listener)
//        evconnlistener_free(listener);
    if(base)
        event_base_free(base);
    return ;
}

void OrdinaryTransferTask::up_load_file_data_listen_cb(evconnlistener *listener, int fd, sockaddr *addr, int len, void *ptr)
{
    cout<<"connect new client!"<<endl;
    cbSet *cb_set=static_cast<cbSet*>(ptr);

    // 通信操作
    // 添加新事件
    struct bufferevent *bev= bufferevent_socket_new(cb_set->base, fd,
                                                     BEV_OPT_CLOSE_ON_FREE|BEV_OPT_DEFER_CALLBACKS);
    cb_set->bev=bev;
    bufferevent_setwatermark(bev,EV_WRITE,BUFFERSIZE/4,0);

    // 给bufferevent缓冲区设置回调
    bufferevent_setcb(bev, nullptr, up_load_file_data_bufferevent_write_cb,
                      up_load_file_data_bufferevent_event_cb,cb_set);
    bufferevent_enable(bev, EV_WRITE|EV_READ);


    //等待HTTP报文回传确认进度，开始传输数据
//    while(cb_set->transfertask->m_begin_offset!=cb_set->transfertask->m_current_offset)
//    {
//        cout<<"not ready!"<<endl;
//    }

    size_t leng=min(BUFFERSIZE*3/4,cb_set->transfertask->m_current_data_segment_length);
    cb_set->getfilefd=open(cb_set->filename.c_str(),O_RDONLY);
    //qDebug()<<"workerthread "<<QThread::currentThreadId()<<cb_set->getfilefd;
    evbuffer_add_file(cb_set->buff,cb_set->getfilefd,cb_set->transfertask->m_current_offset,leng);
    bufferevent_write_buffer(bev,cb_set->buff);
    //close(cb_set->getfilefd);

    //更新进度
    cb_set->transfertask->m_current_offset+=leng;
    cb_set->transfertask->m_current_data_segment_length-=leng;

    if(listener)
        evconnlistener_free(listener);
}

void OrdinaryTransferTask::up_load_file_data_listen_erro_cb(evconnlistener *listener, void *arg)
{
    cout<<"listen failed!"<<endl;
}

void OrdinaryTransferTask::up_load_file_data_bufferevent_write_cb(bufferevent *bev, void *arg)
{
    cbSet *cb_set=static_cast<cbSet*>(arg);

    //任务结束
    if(cb_set->transfertask->m_current_data_segment_length==0)
    {
        return;
    }

    //测试是否加锁决定任务要不要继续
    if(cb_set->transfertask->ifScheduleLocked())
    {
        //transferTaskBreakOffProcess(cb_set);
        //event_base_loopbreak(cb_set->base);
        if(!cb_set->ifuploadsuspend)
        {
            cout<<"have send:"<<cb_set->transfertask->m_current_offset<<endl;
            //QThread::msleep(2000);
            cb_set->transfertask->suspendUploadTranferTask(cb_set->tcplinknum,
                                     cb_set->transfertask->m_current_data_segment_length,
                                     cb_set);
            cb_set->ifuploadsuspend=true;
        }
        return;
    }

    //传输数据
    size_t len=min(BUFFERSIZE*3/4,cb_set->transfertask->m_current_data_segment_length);
    cb_set->getfilefd=open(cb_set->filename.c_str(),O_RDONLY);
    evbuffer_add_file(cb_set->buff,cb_set->getfilefd,cb_set->transfertask->m_current_offset,len);
    bufferevent_write_buffer(bev,cb_set->buff);
    //close(cb_set->getfilefd);

    //更新进度
    cb_set->transfertask->m_current_offset+=len;
    cb_set->transfertask->m_current_data_segment_length-=len;
}

void OrdinaryTransferTask::up_load_file_data_bufferevent_event_cb(bufferevent *bev, short events, void *arg)
{
    cbSet *cb_set=static_cast<cbSet*>(arg);
    if (events & BEV_EVENT_EOF)
    {
        cout<<"connection closed\n"<<endl;
        //若已经结束传送则退出事件循环
//        if(cb_set->transfertask->m_current_data_segment_length==0)
//        {
//            //任务结束的处理
//            //发送进度信号、结束信号
//            transferTaskEndProcess(cb_set);

//            //event_base_loopbreak(cb_set->base);
//        }
//        else
//            transferTaskBreakOffProcess(cb_set);
    }
    else if(events & BEV_EVENT_ERROR)
    {
        printf("some other error\n");
    }
    else if(events & BEV_EVENT_CONNECTED)
    {
        cout<<"已经连接...\\(^o^)/..."<<endl;
        return;
    }

    if(bev)
        bufferevent_free(bev);
    cout<<"buffevent 资源已经被释放..."<<endl;
}

void OrdinaryTransferTask::up_load_file_data_http_cb(evhttp_request *req, void *ctx)
{
    cout<<"down_load_file_date_http_cb have call!"<<endl;
    cbSet *cb_set=static_cast<cbSet*>(ctx);

    //获取状态
    if(!req)
        cout<<"failed to get respond"<<endl;
    //evconnect=evhttp_request_get_connection(req);
    //应答报文基本信息
    //响应状态码
    cout << "Response :" << evhttp_request_get_response_code(req)<<endl;
    //响应状态码行
    cout <<"Line:"<< evhttp_request_get_response_code_line(req) << endl;
    //uri
    const char *uri = evhttp_request_get_uri(req);
    cout<<"uri:"<<string(uri)<<endl;


    //应答报文首部行、缓冲区
    evkeyvalq *headers = evhttp_request_get_input_headers(req);

    if(!evhttp_find_header(headers,"TcpLinkNumber"))
        return;
    sscanf(evhttp_find_header(headers,"TcpLinkNumber"),"%zu",&cb_set->tcplinknum);
    qDebug()<<"TcpLinkNumber:"<<cb_set->tcplinknum;

//    if(!evhttp_find_header(headers,"ReciveSize"))
//        return;

//    //更新进度
//    size_t len=0;
//    const char* lenstr=evhttp_find_header(headers,"ReciveSize");
//    sscanf(lenstr,"%zu",&len);
//    cout<<"recivesize:"<<len<<endl;
//    cb_set->transfertask->m_current_offset+=len;
//    cb_set->transfertask->m_current_data_segment_length-=len;
//    cout<<"m_current_offset:"<<cb_set->transfertask->m_current_offset<<endl;
//    cout<<"m_current_data_segment_length:"<<cb_set->transfertask->m_current_data_segment_length<<endl;

//    //若已经结束传送则退出事件循环
//    if(cb_set->transfertask->m_current_data_segment_length==0)
//    {
//        //任务结束的处理
//        //发送进度信号、结束信号
//        transferTaskEndProcess(cb_set);

//        event_base_loopbreak(cb_set->base);
//    }

        if(cb_set->evconnect)
            evhttp_connection_free(cb_set->evconnect);
}

std::pair<char *, char *> OrdinaryTransferTask::getFolderInfoStrings(string folder)
{
    char* first=OrdinaryTransferTask::fileTreeInfoToString(folder.c_str(),0);
    char* second=OrdinaryTransferTask::fileTreeInfoToString(folder.c_str(),4);
    return make_pair(first,second);
}

std::pair<string, size_t> OrdinaryTransferTask::getFileInfoFromFolder(FileInFolder *fileinfolder)
{
    const char* symbols=",()";
    string obtainedFileName="";
    size_t obtainedFileSize=0;

    while(fileinfolder->filenamespecialSymbool)
    {
        string currentDir=fileinfolder->folderStack.top();
        switch (*fileinfolder->filenamespecialSymbool)
        {
        case ',':
            *fileinfolder->filenamespecialSymbool='\0';
            *fileinfolder->filesizespecialSymbool='\0';
            if(fileinfolder->filenameitem!=fileinfolder->filenamespecialSymbool)
            {
                obtainedFileName=currentDir+"/"+string(fileinfolder->filenameitem);
                sscanf(fileinfolder->filesizeitem,"%zu",&obtainedFileSize);
                fileinfolder->filenameitem=fileinfolder->filenamespecialSymbool+1;
                fileinfolder->filenamespecialSymbool=strpbrk(fileinfolder->filenameitem,symbols);
                fileinfolder->filesizeitem=fileinfolder->filesizespecialSymbool+1;
                fileinfolder->filesizespecialSymbool=strpbrk(fileinfolder->filesizeitem,symbols);
                return make_pair(obtainedFileName,obtainedFileSize);
            }
            fileinfolder->filenameitem=fileinfolder->filenamespecialSymbool+1;
            fileinfolder->filenamespecialSymbool=strpbrk(fileinfolder->filenameitem,symbols);
            fileinfolder->filesizeitem=fileinfolder->filesizespecialSymbool+1;
            fileinfolder->filesizespecialSymbool=strpbrk(fileinfolder->filesizeitem,symbols);
            break;
        case '(':
            *fileinfolder->filenamespecialSymbool='\0';
            *fileinfolder->filesizespecialSymbool='\0';
            fileinfolder->folderStack.push(currentDir+"/"+string(fileinfolder->filenameitem));

            fileinfolder->filenameitem=fileinfolder->filenamespecialSymbool+1;
            fileinfolder->filenamespecialSymbool=strpbrk(fileinfolder->filenameitem,symbols);
            fileinfolder->filesizeitem=fileinfolder->filesizespecialSymbool+1;
            fileinfolder->filesizespecialSymbool=strpbrk(fileinfolder->filesizeitem,symbols);
            break;
        case ')':
            *fileinfolder->filenamespecialSymbool='\0';
            *fileinfolder->filesizespecialSymbool='\0';
            fileinfolder->folderStack.pop();
            if(fileinfolder->filenameitem!=fileinfolder->filenamespecialSymbool)
            {
                obtainedFileName=currentDir+"/"+string(fileinfolder->filenameitem);
                sscanf(fileinfolder->filesizeitem,"%zu",&obtainedFileSize);
                fileinfolder->filenameitem=fileinfolder->filenamespecialSymbool+1;
                fileinfolder->filenamespecialSymbool=strpbrk(fileinfolder->filenameitem,symbols);
                fileinfolder->filesizeitem=fileinfolder->filesizespecialSymbool+1;
                fileinfolder->filesizespecialSymbool=strpbrk(fileinfolder->filesizeitem,symbols);
                return make_pair(obtainedFileName,obtainedFileSize);
            }
            fileinfolder->filenameitem=fileinfolder->filenamespecialSymbool+1;
            fileinfolder->filenamespecialSymbool=strpbrk(fileinfolder->filenameitem,symbols);
            fileinfolder->filesizeitem=fileinfolder->filesizespecialSymbool+1;
            fileinfolder->filesizespecialSymbool=strpbrk(fileinfolder->filesizeitem,symbols);
            break;
        default:
            return make_pair("",0);

        }
    }
    if(fileinfolder->filenameitem)
    {
        string currentDir=fileinfolder->folderStack.top();
        obtainedFileName=currentDir+"/"+string(fileinfolder->filenameitem);
        sscanf(fileinfolder->filesizeitem,"%zu",&obtainedFileSize);
        fileinfolder->filenameitem=nullptr;
        return make_pair(obtainedFileName,obtainedFileSize);
    }
    if((!fileinfolder->filenameitem)&&(!fileinfolder->filenamespecialSymbool))
        return make_pair("",0);
}

void OrdinaryTransferTask::downLoadFolderData(OrdinaryTransferTask *transfertask)
{
    //创建libevent的上下文
    event_base * base = event_base_new();
    if (base)
    {
        //qDebug() << "event_base_new success!\n" ;
    }

    cbSet cbset;
    cbset.base=base;
    cbset.transfertask=transfertask;

    //创建文件夹
    string tem=transfertask->m_reciver+string("/")+string(getFileNameFromUri(transfertask->m_sender.c_str()));
    cbset.foldername=tem;
    //cout<<"foldername:"<<cbset.foldername<<endl;
    int dirfd=mkdir(tem.c_str(),S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH);
    //cout<<"dirfd: "<<dirfd<<endl;

    // 创建套接字 绑定 接收连接请求
    int listeport=listenport.findUnusedPort();
    cbset.listeport=listeport;
    QList<QHostAddress> ipAddresses = QNetworkInterface::allAddresses();
    uint32_t listenaddress=(*ipAddresses.cbegin()).toIPv4Address();
    cbset.listenaddress=listenaddress;
    struct sockaddr_in serv;
    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_port = htons(listeport);
    serv.sin_addr.s_addr = htonl(INADDR_ANY);

    struct evconnlistener* listener= evconnlistener_new_bind(base, down_load_folder_data_listen_cb, &cbset,
                                                              LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE,
                                                              36, (struct sockaddr*)&serv, sizeof(serv));
    evconnlistener_set_error_cb(listener,down_load_folder_data_listen_erro_cb);
    if(listener)
        cout<<"bind sucess!"<<endl;
    else
        cout<<"bind failed!"<<endl;

    // bufferevent连接http服务器
    bufferevent *bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);

    //创建一个 evhttp_connection 对象，表示基于网络连接的 HTTP 客户端
    evhttp_connection *evcon = evhttp_connection_base_bufferevent_new(base,NULL, bev, addr, PORT);
    cbset.evconnect=evcon;

    //设置回调
    evhttp_request *req = evhttp_request_new(down_load_folder_data_http_cb, &cbset);
    evhttp_request_set_error_cb(req,reqerro_cb);
    evhttp_connection_set_closecb(evcon,connclose_cb,&cbset);

    // 设置请求的head 消息报头 信息
    evkeyvalq *output_headers = evhttp_request_get_output_headers(req);
    evhttp_add_header(output_headers,"MessageType","FolderSegmentTransfer");
    evhttp_add_header(output_headers,"FileOffset",to_string(transfertask->m_current_offset).c_str());

    //判断是否是暂停后重启的任务
    if(transfertask->m_begin_offset==transfertask->m_current_offset)//初次启动
        evhttp_add_header(output_headers, "FileSegmentLength",
                          transfertask->m_data_segment_length.c_str());
    else
        evhttp_add_header(output_headers, "FileSegmentLength",
                          to_string(cbset.transfertask->m_current_data_segment_length).c_str());
    evhttp_add_header(output_headers,"Client-ip",to_string(listenaddress).c_str());
    evhttp_add_header(output_headers,"Client-port",to_string(listeport).c_str());
    evhttp_add_header(output_headers,"FolderInfo","FolderName");

    //向指定的 evhttp_connection 对象发起 HTTP 请求
    evhttp_make_request(evcon, req, EVHTTP_REQ_GET, transfertask->m_sender.c_str());

    //设置定时器
    timeval tv={1,0};
    struct event* timeout_event=evtimer_new(base,timeout_cb,&cbset);
    cbset.timeout_event=timeout_event;
    event_add(timeout_event,&tv);

    //事件分发处理
    if(base)
        event_base_dispatch(base);

    if(cbset.transfertask->m_current_data_segment_length==0)
        transferTaskEndProcess(&cbset);
    else
        transferTaskBreakOffProcess(&cbset);

    if(base)
        event_base_free(base);
    return ;
}

void OrdinaryTransferTask::down_load_folder_data_listen_cb(evconnlistener *listener, int fd, sockaddr *addr, int len, void *ptr)
{
    cout<<"connect new client!"<<endl;
    cbSet *cb_set=static_cast<cbSet*>(ptr);

    // 通信操作
    // 添加新事件
    struct bufferevent *bev= bufferevent_socket_new(cb_set->base, fd,
                                                     BEV_OPT_CLOSE_ON_FREE|BEV_OPT_DEFER_CALLBACKS);
    cb_set->bev=bev;
    bufferevent_setwatermark(bev,EV_READ,0,BUFFERSIZE);

    // 给bufferevent缓冲区设置回调
    bufferevent_setcb(bev, down_load_folder_data_bufferevent_read_cb, nullptr,
                      down_load_folder_data_bufferevent_event_cb,cb_set);
    bufferevent_enable(bev, EV_WRITE|EV_READ);
    if(listener)
        evconnlistener_free(listener);
}

void OrdinaryTransferTask::down_load_folder_data_listen_erro_cb(evconnlistener *listener, void *arg)
{
    cout<<"listen failed!"<<endl;
}

void OrdinaryTransferTask::down_load_folder_data_bufferevent_read_cb(bufferevent *bev, void *arg)
{
    cbSet *cb_set=static_cast<cbSet*>(arg);

    //测试是否加锁决定任务要不要继续
    if(cb_set->transfertask->ifScheduleLocked())
    {
        //transferTaskBreakOffProcess(cb_set);
        cout<<"lock!"<<endl;
        bufferevent_free(bev);
        event_del(cb_set->timeout_event);
        //event_base_loopbreak(cb_set->base);
        return;
    }

    struct evbuffer* inbuf=bufferevent_get_input(bev);

    //等待两个字符串到位后开始读取数据
    if((!cb_set->folderFileName)||(!cb_set->folderFileSize))
    {
        //cout<<"wait!"<<endl;
        return;
    }

    //单次接收的字节数
    size_t recivelen=evbuffer_get_length(inbuf);
    //cout<<endl;
    //cout<<"recivelen:"<<recivelen<<endl;
    size_t writensize=0;

    //移动数据到缓冲区
    char recivebuff[recivelen];
    evbuffer_remove(inbuf,recivebuff,recivelen);

    //单次写入使用的evbuffer
    evbuffer* writebuff=evbuffer_new();

    //把缓冲区数据写入文件
    while(writensize<recivelen)
    {
        //获取文件填充字节
        while(cb_set->currentfileoffset==cb_set->currentfilesize)
        {
            auto s=OrdinaryTransferTask::getFileInfoFromFolder(cb_set->fileinfolder);
            cb_set->currentfilename=s.first;
            cb_set->currentfilesize=s.second;
            cb_set->currentfileoffset=0;
        }

        //打开文件
        cb_set->currentfilefd=open(cb_set->currentfilename.c_str(),O_WRONLY|O_CREAT,0600);
        lseek(cb_set->currentfilefd,cb_set->currentfileoffset,SEEK_SET);

        //尽可能地填充数据
        size_t fillsize=min(recivelen-writensize,cb_set->currentfilesize-cb_set->currentfileoffset);
        //cout<<"fillsize:"<<fillsize<<endl;
        evbuffer_add(writebuff,&recivebuff[writensize],fillsize);
        evbuffer_write(writebuff,cb_set->currentfilefd);
        close(cb_set->currentfilefd);

        //更新单次发送的进度
        cb_set->currentfileoffset+=fillsize;
        writensize+=fillsize;
    }
    evbuffer_free(writebuff);

    //更新整个任务的进度
    cb_set->transfertask->m_current_offset+=recivelen;
    cb_set->transfertask->m_current_data_segment_length-=recivelen;
    //cout<<"currentoffset:"<<cb_set->transfertask->m_current_offset<<endl;
    //cout<<"currentfilesegmentlength: "<<cb_set->transfertask->m_current_data_segment_length<<endl;
    if(cb_set->transfertask->m_transfer_task_type=="FolderTransferSubtask")
        cb_set->addsize+=recivelen;

    //cout<<"length:"<<cb_set->transfertask->m_current_data_segment_length<<endl;
    //任务结束
    if(cb_set->transfertask->m_current_data_segment_length==0)
    {        
        //发送进度信号、结束信号
        //transferTaskEndProcess(cb_set);
        cout<<"finsh!"<<endl;

        if(bev)
            bufferevent_free(bev);
        event_del(cb_set->timeout_event);
        //event_base_loopbreak(cb_set->base);
    }

}

void OrdinaryTransferTask::down_load_folder_data_bufferevent_event_cb(bufferevent *bev, short events, void *arg)
{
    if (events & BEV_EVENT_EOF)
    {
        cout<<"connection closed\n"<<endl;
    }
    else if(events & BEV_EVENT_ERROR)
    {
        printf("some other error\n");
    }
    else if(events & BEV_EVENT_CONNECTED)
    {
        cout<<"已经连接...\\(^o^)/..."<<endl;
        return;
    }
    bufferevent_free(bev);
    printf("buffevent 资源已经被释放...\n");
}

void OrdinaryTransferTask::down_load_folder_data_http_cb(evhttp_request *req, void *ctx)
{
    cout<<"down_load_folder_date_http_cb have call!"<<endl;
    cbSet *cb_set=static_cast<cbSet*>(ctx);

    //获取状态
    if(!req)
        cout<<"failed to get respond"<<endl;
    //evconnect=evhttp_request_get_connection(req);
    //应答报文基本信息
    //响应状态码
    cout << "Response :" << evhttp_request_get_response_code(req)<<endl;
    //响应状态码行
    cout <<"Line:"<< evhttp_request_get_response_code_line(req) << endl;
    //uri
    const char *uri = evhttp_request_get_uri(req);
    cout<<"uri:"<<string(uri)<<endl;

    //应答报文首部行、缓冲区
    evkeyvalq *headers = evhttp_request_get_input_headers(req);
    evbuffer *inbuf = evhttp_request_get_input_buffer(req);

    //初始化文件夹数据段大小
    if(!evhttp_find_header(headers,"FileSegmentTransferMode"))
        return;
    if(!evhttp_find_header(headers,"FolderSize"))
        return;
    if(!evhttp_find_header(headers,"FolderInfo"))
        return;

    //文件夹数据段大小
    size_t foldersize=0;
    const char* foldersizestr=evhttp_find_header(headers,"FolderSize");
    sscanf(foldersizestr,"%zu",&foldersize);
    if(cb_set->transfertask->m_data_segment_length=="OffsetToEnd")
    {
        cb_set->transfertask->m_data_segment_length=
            to_string(foldersize-cb_set->transfertask->m_begin_offset);
        cb_set->transfertask->m_current_data_segment_length=
            cb_set->datasegmentlength=foldersize-cb_set->transfertask->m_begin_offset;
    }
    else if(cb_set->transfertask->m_current_offset==cb_set->transfertask->m_begin_offset)
    {

        sscanf(cb_set->transfertask->m_data_segment_length.c_str(),"%zu",&cb_set->datasegmentlength);
        cb_set->transfertask->m_current_data_segment_length=cb_set->datasegmentlength;
    }
    cout<<"filesegmentlength: "<<cb_set->datasegmentlength<<endl;

    if(evhttp_find_header(headers,"FolderInfo")==string("FolderName"))
    {
        //取出字符串
        size_t len=evbuffer_get_length(inbuf);
        cout<<"len:"<<len<<endl;
        cb_set->folderFileName=new char[len];
        evbuffer_remove(inbuf,cb_set->folderFileName,len);
        //cout<<"folderFileName:"<<cb_set->folderFileName<<endl;
        //发起请求以获得文件大小字符串

        //设置回调
        evhttp_request *req = evhttp_request_new(down_load_folder_data_http_cb, cb_set);
        evhttp_request_set_error_cb(req,reqerro_cb);
        evhttp_connection_set_closecb(cb_set->evconnect,connclose_cb,cb_set);

        // 设置请求的head 消息报头 信息
        evkeyvalq *output_headers = evhttp_request_get_output_headers(req);
        evhttp_add_header(output_headers,"MessageType","FolderSegmentTransfer");
        evhttp_add_header(output_headers,"FileOffset",to_string(cb_set->transfertask->m_current_offset).c_str());

        //判断是否是暂停后重启的任务
        if(cb_set->transfertask->m_begin_offset==cb_set->transfertask->m_current_offset)//初次启动
            evhttp_add_header(output_headers, "FileSegmentLength",cb_set->transfertask->m_data_segment_length.c_str());
        else
            evhttp_add_header(output_headers, "FileSegmentLength",
                              to_string(cb_set->transfertask->m_current_data_segment_length).c_str());
        evhttp_add_header(output_headers,"Client-ip",to_string(cb_set->listenaddress).c_str());
        evhttp_add_header(output_headers,"Client-port",to_string(cb_set->listeport).c_str());
        evhttp_add_header(output_headers,"FolderInfo","FolderSize");

        //向指定的 evhttp_connection 对象发起 HTTP 请求
        evhttp_make_request(cb_set->evconnect, req, EVHTTP_REQ_GET, cb_set->transfertask->m_sender.c_str());
        return;
    }
    else if(evhttp_find_header(headers,"FolderInfo")==string("FolderSize"))
    {
        //取出字符串
        size_t len=evbuffer_get_length(inbuf);
        cout<<"len:"<<len<<endl;
        cb_set->folderFileSize=new char[len];
        evbuffer_remove(inbuf,cb_set->folderFileSize,len);
        //cout<<"folderFileSize:"<<cb_set->folderFileSize<<endl;

        //初始化TCP传输需要的参数
        cb_set->fileinfolder=new FileInFolder(cb_set->foldername,cb_set->folderFileName,cb_set->folderFileSize);

        //定位到文件夹相应的数据段偏移位置
        size_t offset=0;
        while(offset!=cb_set->transfertask->m_current_offset)
        {
            //获取文件填充字节
            auto s=OrdinaryTransferTask::getFileInfoFromFolder(cb_set->fileinfolder);
            cb_set->currentfilename=s.first;
            cb_set->currentfilesize=s.second;
            cb_set->currentfileoffset=0;

            //更新进度
            size_t addlen=min(cb_set->transfertask->m_current_offset-offset,cb_set->currentfilesize);
            offset+=addlen;
            cb_set->currentfileoffset+=addlen;
        }
        cout<<"offset:"<<offset<<endl;
    }
    if(cb_set->evconnect)
        evhttp_connection_free(cb_set->evconnect);
}

void OrdinaryTransferTask::upLoadFolderData(OrdinaryTransferTask *transfertask)
{
    //创建libevent的上下文
    event_base * base = event_base_new();
    if (base)
    {
        //qDebug() << "event_base_new success!\n" ;
    }

    cbSet cbset;
    cbset.base=base;
    cbset.transfertask=transfertask;
    cbset.buff=evbuffer_new();

    //文件夹
    cbset.foldername=transfertask->m_sender;
    //cout<<"foldername:"<<cbset.foldername<<endl;

    //文件夹发送器初始化
    cbset.fileinfolder=new FileInFolder(cbset.foldername);

    //文件夹字符串信息
    auto s=OrdinaryTransferTask::getFolderInfoStrings(cbset.foldername);
    int namestringsize=strlen(s.first)+1;
    evbuffer_iovec folderinfostr[2];
    folderinfostr[0].iov_base=s.first;
    folderinfostr[0].iov_len=strlen(s.first)+1;
    folderinfostr[1].iov_base=s.second;
    folderinfostr[1].iov_len=strlen(s.second)+1;
    //cout<<"filename:"<<s.first<<endl;
    //cout<<"filesize:"<<s.second<<endl;

    // 创建套接字 绑定 接收连接请求
    int listeport=listenport.findUnusedPort();
    cbset.listeport=listeport;
    QList<QHostAddress> ipAddresses = QNetworkInterface::allAddresses();
    uint32_t listenaddress=(*ipAddresses.cbegin()).toIPv4Address();
    cbset.listenaddress=listenaddress;
    struct sockaddr_in serv;
    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_port = htons(listeport);
    serv.sin_addr.s_addr = htonl(INADDR_ANY);

    struct evconnlistener* listener= evconnlistener_new_bind(base, up_load_folder_data_listen_cb, &cbset,
                                                              LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE,
                                                              36, (struct sockaddr*)&serv, sizeof(serv));
    evconnlistener_set_error_cb(listener,up_load_folder_data_listen_erro_cb);
    if(listener)
        cout<<"bind sucess!"<<endl;
    else
        cout<<"bind failed!"<<endl;

    // bufferevent  连接http服务器
    bufferevent *bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);

    //创建一个 evhttp_connection 对象，表示基于网络连接的 HTTP 客户端
    evhttp_connection *evcon = evhttp_connection_base_bufferevent_new(base,NULL, bev, addr, PORT);
    cbset.evconnect=evcon;

    //设置回调
    evhttp_request *req = evhttp_request_new(up_load_folder_data_http_cb, &cbset);
    evhttp_request_set_error_cb(req,reqerro_cb);
    evhttp_connection_set_closecb(evcon,connclose_cb,&cbset);

    //报头 发送缓存
    evkeyvalq *output_headers = evhttp_request_get_output_headers(req);
    evbuffer *outbuf = evhttp_request_get_output_buffer(req);

    // 设置请求的head 消息报头 信息
    evhttp_add_header(output_headers,"MessageType","FolderSegmentTransfer");
    evhttp_add_header(output_headers,"FolderSize",to_string
                                                    (OrdinaryTransferTask::getFolderSize(cbset.foldername.c_str())).c_str());
    evhttp_add_header(output_headers,"FileOffset",to_string(transfertask->m_current_offset).c_str());
    evhttp_add_header(output_headers, "FileSegmentLength",
                      to_string(transfertask->m_current_data_segment_length).c_str());
    evhttp_add_header(output_headers,"Client-ip",to_string(listenaddress).c_str());
    evhttp_add_header(output_headers,"Client-port",to_string(listeport).c_str());
    evhttp_add_header(output_headers,"FolderInfo","FolderNameAndSize");
    evhttp_add_header(output_headers,"NameStringSize",to_string(namestringsize).c_str());

    //往发送缓冲区添加文件名字符串和文件大小字符串
    //    evbuffer_add(outbuf,s.first,strlen(s.first));
    //    evbuffer_add(outbuf,s.second,strlen(s.second));
    evbuffer_add_iovec(outbuf,folderinfostr,2);
    if(s.first)
        delete[] s.first;
    if(s.second)
        delete[] s.second;

    //向指定的 evhttp_connection 对象发起 HTTP 请求
    QFileInfo fileinfo(QString(cbset.foldername.c_str()));
    string q=fileinfo.fileName().toStdString();
    evhttp_make_request(evcon, req, EVHTTP_REQ_POST, (transfertask->m_reciver+"/"+q).c_str());

    //设置定时器
    timeval tv={1,0};
    struct event* timeout_event=evtimer_new(base,timeout_cb,&cbset);
    cbset.timeout_event=timeout_event;
    event_add(timeout_event,&tv);

    //事件分发处理
    if(base)
        event_base_dispatch(base);

    if(cbset.transfertask->m_current_data_segment_length==0)
        transferTaskEndProcess(&cbset);
    else
        transferTaskBreakOffProcess(&cbset);

//    if(evcon)
//        evhttp_connection_free(evcon);
//    if(listener)
//        evconnlistener_free(listener);
    if(base)
        event_base_free(base);
    return ;
}

void OrdinaryTransferTask::up_load_folder_data_listen_cb(evconnlistener *listener, int fd, sockaddr *addr, int len, void *ptr)
{
    cout<<"connect new client!"<<endl;
    cbSet *cb_set=static_cast<cbSet*>(ptr);

    // 添加新事件
    struct bufferevent *bev= bufferevent_socket_new(cb_set->base, fd,
                                                     BEV_OPT_CLOSE_ON_FREE|BEV_OPT_DEFER_CALLBACKS);
    cb_set->bev=bev;
    bufferevent_setwatermark(bev,EV_READ,0,BUFFERSIZE);

    // 给bufferevent缓冲区设置回调
    bufferevent_setcb(bev, nullptr, up_load_folder_data_bufferevent_write_cb,
                      up_load_folder_data_bufferevent_event_cb,cb_set);
    bufferevent_enable(bev, EV_WRITE|EV_READ);

    //测试是否加锁决定任务要不要继续
    if(cb_set->transfertask->ifScheduleLocked())
    {
        //transferTaskBreakOffProcess(cb_set);
        //event_base_loopbreak(cb_set->base);
        event_del(cb_set->timeout_event);
        return;
    }

    //定位到文件夹相应的数据段偏移位置
    size_t offset=0;
    while(offset!=cb_set->transfertask->m_current_offset)
    {
        //获取文件填充字节
        auto s=OrdinaryTransferTask::getFileInfoFromFolder(cb_set->fileinfolder);
        cb_set->currentfilename=s.first;
        cb_set->currentfilesize=s.second;
        cb_set->currentfileoffset=0;

        //更新进度
        size_t addlen=min(cb_set->transfertask->m_current_offset-offset,cb_set->currentfilesize);
        offset+=addlen;
        cb_set->currentfileoffset+=addlen;
    }

    //上传文件夹数据段
    if(cb_set->transfertask->m_current_data_segment_length==0)
    {
        return;
    }

    //期望传输量
    size_t length=min(BUFFERSIZE,cb_set->transfertask->m_current_data_segment_length);

    //获取文件填充字节
    while(cb_set->currentfileoffset==cb_set->currentfilesize)
    {
        auto s=OrdinaryTransferTask::getFileInfoFromFolder(cb_set->fileinfolder);
        cb_set->currentfilename=s.first;
        cb_set->currentfilesize=s.second;
        cb_set->currentfileoffset=0;
    }

    //打开文件
    cb_set->currentfilefd=open(cb_set->currentfilename.c_str(),O_RDONLY);
    //cout<<"currentfilename:"<<cb_set->currentfilename<<endl;

    //尽可能地填充数据
    size_t fillsize=min(length,cb_set->currentfilesize-cb_set->currentfileoffset);
    //cout<<"fillsize:"<<fillsize<<endl;
    evbuffer_add_file(cb_set->buff,cb_set->currentfilefd,cb_set->currentfileoffset,fillsize);

    //更新单次发送的进度
    cb_set->currentfileoffset+=fillsize;
//    cout<<"currentfilesize:"<<cb_set->currentfilesize<<" currentfileoffset:"
//         <<cb_set->currentfileoffset<<endl;
    bufferevent_write_buffer(bev,cb_set->buff);

    //更新整个任务的进度
    cb_set->transfertask->m_current_offset+=fillsize;
    cb_set->transfertask->m_current_data_segment_length-=fillsize;
//    cout<<"currentoffset: "<<cb_set->transfertask->m_current_offset<<endl;
//    cout<<"currentlength: "<<cb_set->transfertask->m_current_data_segment_length<<endl;

    if(listener)
        evconnlistener_free(listener);
}

void OrdinaryTransferTask::up_load_folder_data_listen_erro_cb(evconnlistener *listener, void *arg)
{
    cout<<"listen failed!"<<endl;
}

void OrdinaryTransferTask::up_load_folder_data_bufferevent_write_cb(bufferevent *bev, void *arg)
{
    cbSet *cb_set=static_cast<cbSet*>(arg);

    //上传文件夹数据段
    if(cb_set->transfertask->m_current_data_segment_length==0)
    {
        event_del(cb_set->timeout_event);
        return;
    }

    //测试是否加锁决定任务要不要继续
    if(cb_set->transfertask->ifScheduleLocked())
    {
        //transferTaskBreakOffProcess(cb_set);
        //bufferevent_free(bev);
        //event_base_loopbreak(cb_set->base);
        if(!cb_set->ifuploadsuspend)
        {
            cout<<"have send:"<<cb_set->transfertask->m_current_offset<<endl;
            //QThread::msleep(2000);
            cb_set->transfertask->suspendUploadTranferTask(cb_set->tcplinknum,
                                                           cb_set->transfertask->m_current_data_segment_length,
                                                           cb_set);
            cb_set->ifuploadsuspend=true;
        }
        event_del(cb_set->timeout_event);
        return;
    }

    //期望传输量
    size_t len=min(BUFFERSIZE,cb_set->transfertask->m_current_data_segment_length);

    //获取文件填充字节
    while(cb_set->currentfileoffset==cb_set->currentfilesize)
    {
        auto s=OrdinaryTransferTask::getFileInfoFromFolder(cb_set->fileinfolder);
        cb_set->currentfilename=s.first;
        cb_set->currentfilesize=s.second;
        cb_set->currentfileoffset=0;
    }

    //打开文件
    cb_set->currentfilefd=open(cb_set->currentfilename.c_str(),O_RDONLY);
    if(cb_set->currentfilefd==-1)
        cout<<endl<<"currentfilename:"<<cb_set->currentfilename<<endl;

    //尽可能地填充数据
    size_t fillsize=min(len,cb_set->currentfilesize-cb_set->currentfileoffset);
    //cout<<"fillsize:"<<fillsize<<endl;
    evbuffer_add_file(cb_set->buff,cb_set->currentfilefd,cb_set->currentfileoffset,fillsize);

    //更新单次发送的进度
    cb_set->currentfileoffset+=fillsize;
    //    cout<<"currentfilesize:"<<cb_set->currentfilesize<<" currentfileoffset:"
    //         <<cb_set->currentfileoffset<<endl;
    bufferevent_write_buffer(bev,cb_set->buff);

    //更新整个任务的进度
    cb_set->transfertask->m_current_offset+=fillsize;
    cb_set->transfertask->m_current_data_segment_length-=fillsize;
    //cout<<"currentoffset: "<<cb_set->transfertask->m_current_offset<<endl;
    //cout<<"currentlength: "<<cb_set->transfertask->m_current_data_segment_length<<endl;
}

void OrdinaryTransferTask::up_load_folder_data_bufferevent_event_cb(bufferevent *bev, short events, void *arg)
{
    if (events & BEV_EVENT_EOF)
    {
        cbSet *cb_set=static_cast<cbSet*>(arg);
        cout<<"connection closed\n"<<endl;
        //任务结束
        if(cb_set->transfertask->m_current_data_segment_length==0)
        {
            //发送进度信号、结束信号
//            transferTaskEndProcess(cb_set);
//            event_base_loopbreak(cb_set->base);
        }
    }
    else if(events & BEV_EVENT_ERROR)
    {
        printf("some other error\n");
    }
    else if(events & BEV_EVENT_CONNECTED)
    {
        cout<<"已经连接...\\(^o^)/..."<<endl;
        return;
    }
    bufferevent_free(bev);
    printf("buffevent 资源已经被释放...\n");
}

void OrdinaryTransferTask::up_load_folder_data_http_cb(evhttp_request *req, void *ctx)
{
    //cout<<"up_load_folder_date_http_cb have call!"<<endl;
    cbSet *cb_set=static_cast<cbSet*>(ctx);

    //获取状态
    if(!req)
        cout<<"failed to get respond"<<endl;
    //evconnect=evhttp_request_get_connection(req);
    //应答报文基本信息
    //响应状态码
    //cout << "Response :" << evhttp_request_get_response_code(req)<<endl;
    //响应状态码行
    //cout <<"Line:"<< evhttp_request_get_response_code_line(req) << endl;
    //uri
    const char *uri = evhttp_request_get_uri(req);
    //cout<<"uri:"<<string(uri)<<endl;

    //应答报文首部行、缓冲区
    evkeyvalq *headers = evhttp_request_get_input_headers(req);

    if(!evhttp_find_header(headers,"TcpLinkNumber"))
        return;
    sscanf(evhttp_find_header(headers,"TcpLinkNumber"),"%zu",&cb_set->tcplinknum);
    qDebug()<<"TcpLinkNumber:"<<cb_set->tcplinknum;

    if(cb_set->evconnect)
        evhttp_connection_free(cb_set->evconnect);
}

void OrdinaryTransferTask::suspendUploadTranferTask(int tcplinknum,
                                                    size_t expectedtransmissionvolume,
                                                    cbSet *cb_set)
{
    // bufferevent  连接http服务器
    bufferevent *bev = bufferevent_socket_new(cb_set->base, -1, BEV_OPT_CLOSE_ON_FREE);

    //创建一个 evhttp_connection 对象，表示基于网络连接的 HTTP 客户端
    evhttp_connection *evcon = evhttp_connection_base_bufferevent_new(cb_set->base,NULL, bev, addr, PORT);
    cb_set->stopevconnect=evcon;

    //设置回调
    evhttp_request *req = evhttp_request_new(suspend_up_load_tranfer_task_http_cb, cb_set);
    evhttp_request_set_error_cb(req,reqerro_cb);
    evhttp_connection_set_closecb(evcon,connclose_cb,cb_set);

    //报头 发送缓存
    evkeyvalq *output_headers = evhttp_request_get_output_headers(req);
    evbuffer *outbuf = evhttp_request_get_output_buffer(req);

    // 设置请求的head 消息报头 信息
    evhttp_add_header(output_headers,"MessageType","suspendUploadTranferTask");
    evhttp_add_header(output_headers,"TcpLinkNumber",to_string(tcplinknum).c_str());
    evhttp_add_header(output_headers,"ExpectedTransmissionVolume",
                      to_string(expectedtransmissionvolume).c_str());

    //向指定的 evhttp_connection 对象发起 HTTP 请求
    evhttp_make_request(evcon, req, EVHTTP_REQ_POST,"/xuyanming");

    return ;
}

void OrdinaryTransferTask::suspend_up_load_tranfer_task_http_cb(evhttp_request *req, void *ctx)
{
    cout<<"suspend_up_load_tranfer_task_http_cb have call!"<<endl;
    cbSet *cb_set=static_cast<cbSet*>(ctx);
    evkeyvalq *headers = evhttp_request_get_input_headers(req);
    //获取状态
    if(!req)
        cout<<"failed to get respond"<<endl;
    //evconnect=evhttp_request_get_connection(req);
    //应答报文基本信息
    //响应状态码
    //cout << "Response :" << evhttp_request_get_response_code(req)<<endl;
    //响应状态码行
    //cout <<"Line:"<< evhttp_request_get_response_code_line(req) << endl;
    //uri
    const char *uri = evhttp_request_get_uri(req);
    //cout<<"uri:"<<string(uri)<<endl;

    if(!evhttp_find_header(headers,"TcpLinkStatus"))
        return;
    if(evhttp_find_header(headers,"TcpLinkStatus")==string("Open"))
    {
        qDebug()<<"client close tcp!";
        if(cb_set->bev)
            bufferevent_free(cb_set->bev);
    }

    if(cb_set->stopevconnect)
        evhttp_connection_free(cb_set->stopevconnect);
    event_del(cb_set->timeout_event);
}

void OrdinaryTransferTask::getFileMessageFromServer(OrdinaryTransferTask *transfertask)
{
    //建立连接
    //创建libevent的上下文
    event_base * base = event_base_new();
    if (base)
    {
        qDebug() << "event_base_new success!\n" ;
    }
    // bufferevent  连接http服务器
    bufferevent *bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
    //创建一个 evhttp_connection 对象，表示基于网络连接的 HTTP 客户端
    evhttp_connection *evcon = evhttp_connection_base_bufferevent_new(base,
                                                                      NULL, bev, addr, PORT);

    //设置回调
    //http client  请求 回调函数设置
    cbSet cbset;
    cbset.base=base;
    cbset.transfertask=transfertask;
    cbset.evconnect=evcon;
    evhttp_request *req = evhttp_request_new(get_file_message_from_server_cb, &cbset);
    evhttp_request_set_error_cb(req,reqerro_cb);
    evhttp_connection_set_closecb(evcon,connclose_cb,&cbset);

    //发送报文
    // 设置请求的head 消息报头 信息
    evkeyvalq *output_headers = evhttp_request_get_output_headers(req);
    evhttp_add_header(output_headers, "MessageType","GetInformation");
    evhttp_add_header(output_headers,"GetInformation","File");

    //向指定的 evhttp_connection 对象发起 HTTP 请求
    evhttp_make_request(evcon, req, EVHTTP_REQ_GET, m_target_information_path.c_str());

    //事件分发处理
    if (base)
        event_base_dispatch(base);
    //if (evcon)evhttp_connection_free(evcon);
    if (base)
        event_base_free(base);
    //qDebug()<<"work end!";
}

void OrdinaryTransferTask::get_file_message_from_server_cb(evhttp_request *req, void *ctx)
{
    cbSet *cb_set=static_cast<cbSet*>(ctx);

    //获取状态
    if(!req)
        cout<<"failed to get respond"<<endl;
    const char *uri = evhttp_request_get_uri(req);
    //cout<<"uri:"<<string(uri)<<endl;

    //应答报文首部行、缓冲区
    evkeyvalq *headers = evhttp_request_get_input_headers(req);
    evbuffer *inbuf = evhttp_request_get_input_buffer(req);

    //取出文件大小
    size_t fileSize=0;
    if(!evhttp_find_header(headers,"FileSize"))
        return;
    const char* filesSizeString=evhttp_find_header(headers,"FileSize");
    sscanf(filesSizeString,"%zu",&fileSize);

    cb_set->transfertask->m_file_size=fileSize;
    cout<<"filesize:"<<fileSize<<endl;

    //结束信号
    //transferTaskEndProcess(cb_set);
    OrdinaryTransferTask* transfertask=cb_set->transfertask;
    transfertask->setTaskType("FileSegmentTransfer");

    string taskType=transfertask->getTaskType();
    string transfertasktype=transfertask->getTransferTaskType();
    string transfertype=transfertask->getTransferType();
    string sender=transfertask->getTransferSender();
    string reciver=transfertask->getTransferReciver();
    size_t offset=0;
    string length=filesSizeString;

    SuperTransferTask* superfiledownloadtransfertask=new SuperTransferTask();
    superfiledownloadtransfertask->moveToThread(cb_set->transfertask->m_uithread);
    superfiledownloadtransfertask->transferInit
        (taskType,transfertasktype,transfertype,sender,reciver,offset,length);

    emit cb_set->transfertask->task_end(dynamic_cast<TransferTask*>(superfiledownloadtransfertask),nullptr);

    if (cb_set->evconnect)evhttp_connection_free(cb_set->evconnect);
    //event_base_loopbreak(cb_set->base);
}

void OrdinaryTransferTask::getFolderMessageFromServer(OrdinaryTransferTask *transfertask)
{
    //建立连接
    //创建libevent的上下文
    event_base * base = event_base_new();
    if (base)
    {
        //qDebug() << "event_base_new success!\n" ;
    }

    // bufferevent  连接http服务器
    bufferevent *bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);

    //创建一个 evhttp_connection 对象，表示基于网络连接的 HTTP 客户端
    evhttp_connection *evcon = evhttp_connection_base_bufferevent_new(base,
                                                                      NULL, bev, addr, PORT);

    //设置回调
    //http client  请求 回调函数设置
    cbSet cbset;
    cbset.base=base;
    cbset.transfertask=transfertask;
    cbset.evconnect=evcon;
    evhttp_request *req = evhttp_request_new(get_folder_message_from_server_cb, &cbset);
    evhttp_request_set_error_cb(req,reqerro_cb);
    evhttp_connection_set_closecb(evcon,connclose_cb,&cbset);

    //发送报文
    // 设置请求的head 消息报头 信息
    evkeyvalq *output_headers = evhttp_request_get_output_headers(req);
    evhttp_add_header(output_headers, "MessageType","GetInformation");
    evhttp_add_header(output_headers,"GetInformation","Folder");

    //向指定的 evhttp_connection 对象发起 HTTP 请求
    evhttp_make_request(evcon, req, EVHTTP_REQ_GET, m_target_information_path.c_str());

    //事件分发处理
    if (base)
        event_base_dispatch(base);
//    if (evcon)evhttp_connection_free(evcon);
    if (base)
        event_base_free(base);
}

void OrdinaryTransferTask::get_folder_message_from_server_cb(evhttp_request *req, void *ctx)
{
    cbSet *cb_set=static_cast<cbSet*>(ctx);

    //获取状态
    if(!req)
        cout<<"failed to get respond"<<endl;
    const char *uri = evhttp_request_get_uri(req);
    //cout<<"uri:"<<string(uri)<<endl;

    //应答报文首部行、缓冲区
    evkeyvalq *headers = evhttp_request_get_input_headers(req);
    evbuffer *inbuf = evhttp_request_get_input_buffer(req);

    //取出字符串
    size_t len=evbuffer_get_length(inbuf);
    char* fileSystem=new char[len];
    evbuffer_remove(inbuf,fileSystem,len);
    cout<<"fileSystem:"<<fileSystem<<endl;

    //取出文件夹大小
    size_t filesSize=0;
    if(!evhttp_find_header(headers,"FolderSize"))
        return;
    const char* filesSizeString=evhttp_find_header(headers,"FolderSize");
    sscanf(filesSizeString,"%zu",&filesSize);
    cout<<"filesize:"<<filesSize<<endl;

    cb_set->transfertask->m_folder_string=fileSystem;
    if(fileSystem)
        delete[] fileSystem;
    cb_set->transfertask->m_folder_name=getFileNameFromUri(uri).c_str();
    cb_set->transfertask->m_folder_size=filesSize;

    //结束信号
    //transferTaskEndProcess(cb_set);
    OrdinaryTransferTask* transfertask=cb_set->transfertask;
    transfertask->setTaskType("FolderSegmentTransfer");

    string taskType=transfertask->getTaskType();
    string transfertasktype=transfertask->getTransferTaskType();
    string transfertype=transfertask->getTransferType();
    string sender=transfertask->getTransferSender();
    string reciver=transfertask->getTransferReciver();
    size_t offset=0;
    string length=filesSizeString;

    SuperTransferTask* superfolderdownloadtransfertask=new SuperTransferTask();
    superfolderdownloadtransfertask->moveToThread(cb_set->transfertask->m_uithread);
    superfolderdownloadtransfertask->transferInit
        (taskType,transfertasktype,transfertype,sender,reciver,offset,length);
    emit cb_set->transfertask->task_end(dynamic_cast<TransferTask*>(superfolderdownloadtransfertask),nullptr);

    //event_base_loopbreak(cb_set->base);
    if(cb_set->evconnect)
        evhttp_connection_free(cb_set->evconnect);
}

void OrdinaryTransferTask::getFileSystemMessageFromServer(OrdinaryTransferTask *transfertask)
{
    //建立连接
    //创建libevent的上下文
    event_base * base = event_base_new();
    if (base)
    {
        //qDebug() << "event_base_new success!\n" ;
    }
    // bufferevent  连接http服务器
    bufferevent *bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
    //创建一个 evhttp_connection 对象，表示基于网络连接的 HTTP 客户端
    evhttp_connection *evcon = evhttp_connection_base_bufferevent_new(base,
                                                                      NULL, bev, addr, PORT);

    //设置回调
    //http client  请求 回调函数设置
    cbSet cbset;
    cbset.base=base;
    cbset.transfertask=transfertask;
    cbset.evconnect=evcon;
    evhttp_request *req = evhttp_request_new(get_file_system_message_from_server_cb, &cbset);
    evhttp_request_set_error_cb(req,reqerro_cb);
    evhttp_connection_set_closecb(evcon,connclose_cb,&cbset);

    //发送报文
    // 设置请求的head 消息报头 信息
    evkeyvalq *output_headers = evhttp_request_get_output_headers(req);
    evhttp_add_header(output_headers, "MessageType","GetInformation");
    evhttp_add_header(output_headers,"GetInformation","FileSystem");
    evhttp_add_header(output_headers,"Information-Type","Name");

    //向指定的 evhttp_connection 对象发起 HTTP 请求
    evhttp_make_request(evcon, req, EVHTTP_REQ_GET, m_target_information_path.c_str());

    //事件分发处理
    if (base)
        event_base_dispatch(base);
//    if (evcon)evhttp_connection_free(evcon);
    if (base)
        event_base_free(base);
}

void OrdinaryTransferTask::get_file_system_message_from_server_cb(evhttp_request *req, void *ctx)
{
    cbSet *cb_set=static_cast<cbSet*>(ctx);
    //获取状态
    if(!req)
        cout<<"failed to get respond"<<endl;
    //uri
    const char *uri = evhttp_request_get_uri(req);
    cout<<"uri:"<<uri<<endl;

    //应答报文首部行、缓冲区
    evkeyvalq *headers = evhttp_request_get_input_headers(req);
    evbuffer *inbuf = evhttp_request_get_input_buffer(req);

    //取出字符串
    size_t len=evbuffer_get_length(inbuf);
    char* fileSystem=new char[len];
    evbuffer_remove(inbuf,fileSystem,len);
    //cout<<fileSystem<<endl;
    if(!evhttp_find_header(headers,"Information-Type"))
        return;

    if(evhttp_find_header(headers,"Information-Type")==string("Name"))
    {
        cb_set->transfertask->m_file_system_vector.push_back(fileSystem);
        //http client  请求 回调函数设置
        evhttp_request *request = evhttp_request_new(get_file_system_message_from_server_cb, cb_set);

        // 设置请求的head 消息报头 信息
        evkeyvalq *outputHeaders = evhttp_request_get_output_headers(request);
        evhttp_add_header(outputHeaders, "MessageType","GetInformation");
        evhttp_add_header(outputHeaders,"GetInformation","FileSystem");
        evhttp_add_header(outputHeaders,"Information-Type","Size");

        //向指定的 evhttp_connection 对象发起 HTTP 请求
        int a=evhttp_make_request(cb_set->evconnect, request, EVHTTP_REQ_GET, uri);
        //cout<<"a:"<<a<<endl;
    }
    else if(evhttp_find_header(headers,"Information-Type")==string("Size"))
    {
        cb_set->transfertask->m_file_system_vector.push_back(fileSystem);
        //http client  请求 回调函数设置
        evhttp_request *request = evhttp_request_new(get_file_system_message_from_server_cb, cb_set);

        // 设置请求的head 消息报头 信息
        evkeyvalq *outputHeaders = evhttp_request_get_output_headers(request);
        evhttp_add_header(outputHeaders, "MessageType","GetInformation");
        evhttp_add_header(outputHeaders,"GetInformation","FileSystem");
        evhttp_add_header(outputHeaders,"Information-Type","Type");

        //向指定的 evhttp_connection 对象发起 HTTP 请求
        evhttp_make_request(cb_set->evconnect, request, EVHTTP_REQ_GET, uri);
    }
    else if(evhttp_find_header(headers,"Information-Type")==string("Type"))
    {
        cb_set->transfertask->m_file_system_vector.push_back(fileSystem);
        //http client  请求 回调函数设置
        evhttp_request *request = evhttp_request_new(get_file_system_message_from_server_cb, cb_set);

        // 设置请求的head 消息报头 信息
        evkeyvalq *outputHeaders = evhttp_request_get_output_headers(request);
        evhttp_add_header(outputHeaders, "MessageType","GetInformation");
        evhttp_add_header(outputHeaders,"GetInformation","FileSystem");
        evhttp_add_header(outputHeaders,"Information-Type","DateModified");

        //向指定的 evhttp_connection 对象发起 HTTP 请求
        evhttp_make_request(cb_set->evconnect, request, EVHTTP_REQ_GET, uri);
    }
    else if(evhttp_find_header(headers,"Information-Type")==string("DateModified"))
    {
        cb_set->transfertask->m_file_system_vector.push_back(fileSystem);
        qDebug()<<"build model start!";
        QStandardItemModel *model=
            translateStringToModel(cb_set->transfertask->m_file_system_vector);
        qDebug()<<"build model end!";
        emit cb_set->transfertask->dirMessage(model);

        //发送进度信号、结束信号
//        transferTaskEndProcess(cb_set);

        //event_base_loopbreak(cb_set->base);
        if(cb_set->evconnect)
            evhttp_connection_free(cb_set->evconnect);
    }
}

void OrdinaryTransferTask::addEmptyFolderToServer(OrdinaryTransferTask *transfertask)
{
    //建立连接
    //创建libevent的上下文
    event_base * base = event_base_new();
    if (base)
    {
        qDebug() << "event_base_new success!\n" ;
    }
    // bufferevent  连接http服务器
    bufferevent *bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
    //创建一个 `evhttp_connection` 对象，表示基于网络连接的 HTTP 客户端
    evhttp_connection *evcon = evhttp_connection_base_bufferevent_new(base,
                                                                      NULL, bev, addr, PORT);
    cbSet cbset;
    cbset.base=base;
    cbset.evconnect=evcon;
    //上传目录
    string uri=string(transfertask->m_target_instruct_path+"/"+transfertask->m_empty_folder);

    //http client  请求 回调函数设置
    evhttp_request *req = evhttp_request_new(add_empty_folder_to_server_cb, &cbset);
    evhttp_request_set_error_cb(req,reqerro_cb);
    // 设置请求的head 消息报头 信息
    evkeyvalq *output_headers = evhttp_request_get_output_headers(req);
    evhttp_add_header(output_headers, "MessageType","PushInstruct");
    evhttp_add_header(output_headers,"PushInstruct","AddEmptyFolder");
    evhttp_add_header(output_headers, "AddEmptyFolder",uri.c_str());

    //向指定的 evhttp_connection 对象发起 HTTP 请求
    evhttp_make_request(evcon, req, EVHTTP_REQ_POST, uri.c_str());

    //事件分发处理
    if (base)
        event_base_dispatch(base);

//    if (evcon)
//        evhttp_connection_free(evcon);
    if (base)
        event_base_free(base);

}

void OrdinaryTransferTask::add_empty_folder_to_server_cb(evhttp_request *req, void *ctx)
{
    cbSet *cb_set=static_cast<cbSet*>(ctx);
    if(!req)
        cout<<"failed to get respond"<<endl;
    const char *uri = evhttp_request_get_uri(req);
    event_base* base=evhttp_connection_get_base(cb_set->evconnect);

    //发送进度信号、结束信号
    //transferTaskEndProcess(cb_set);

    //event_base_loopbreak(base);
    if(cb_set->evconnect)
        evhttp_connection_free(cb_set->evconnect);
}

int OrdinaryTransferTask::deleteFileOrFolderToServer(OrdinaryTransferTask *transfertask)
{
    //建立连接
    //创建libevent的上下文
    event_base * base = event_base_new();
    if (base)
    {
        qDebug() << "event_base_new success!\n" ;
    }
    // bufferevent  连接http服务器
    bufferevent *bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
    //创建一个 `evhttp_connection` 对象，表示基于网络连接的 HTTP 客户端
    evhttp_connection *evcon = evhttp_connection_base_bufferevent_new(base,
                                                                      NULL, bev, addr, PORT);
    cbSet cbset;
    cbset.base=base;
    cbset.evconnect=evcon;
    cbset.transfertask=transfertask;

    //发送请求报文
    //http client  请求 回调函数设置
    evhttp_request *req = evhttp_request_new(delete_file_or_folder_to_server_cb, &cbset);

    //设置首部
    evkeyvalq *output_headers = evhttp_request_get_output_headers(req);
    evhttp_add_header(output_headers, "MessageType","PushInstruct");
    evhttp_add_header(output_headers,"PushInstruct","DeleteFileOrFolder");

    //向指定的 evhttp_connection 对象发起 HTTP 请求
    evhttp_make_request(evcon, req, EVHTTP_REQ_DELETE, transfertask->m_target_instruct_path.c_str());

    //事件分发处理
    if (base)
        event_base_dispatch(base);
    //if (uri)evhttp_uri_free(uri);
//    if (evcon)evhttp_connection_free(evcon);
    if (base)
        event_base_free(base);
}

void OrdinaryTransferTask::delete_file_or_folder_to_server_cb(evhttp_request *req, void *ctx)
{
    cbSet *cb_set=static_cast<cbSet*>(ctx);
    std::cout<<"download_get_http_client_cb had call! "<<std::endl;
    if(!req)
        cout<<"failed to get respond"<<endl;
    //evconnect=evhttp_request_get_connection(req);
    //应答报文基本信息
    //响应状态码
    cout << "Response :" << evhttp_request_get_response_code(req)<<endl;
    //响应状态码行
    cout <<"Line:"<< evhttp_request_get_response_code_line(req) << endl;
    //uri
    const char *uri = evhttp_request_get_uri(req);
    cout<<"uri:"<<string(uri)<<endl;

    //发送进度信号、结束信号
    //transferTaskEndProcess(cb_set);

    //event_base_loopbreak(cb_set->base);
    if(cb_set->evconnect)
        evhttp_connection_free(cb_set->evconnect);
}

char *OrdinaryTransferTask::fileTreeInfoToString(QString path, int inforType)
{
    QString files;
    QDir dir(path);

    //列出子目录
    QStringList strDirList=dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    //列出所有文件
    QStringList strFileList=dir.entryList(QDir::Files);

    if(strDirList.count()==0&&strFileList.count()==0)
        return "";

    for(int i=0;i<strDirList.count();i++)
    {
        //name
        if(inforType==0)
            files+=strDirList.at(i);
        //sizetext
        else if(inforType==1)
        {
            files+=QString("0");
        }
        //type
        else if(inforType==2)
        {
            files+=QString("Folder");
        }
        //DateModified
        else if(inforType==3)
        {
            files+=QString("0");
        }
        else if(inforType==4)
        {
            files+=QString("0");
        }
        files+="(";

        files+=fileTreeInfoToString(path+"/"+strDirList.at(i),inforType);
        files+=")";
        if(i==(strDirList.count()-1)&&strFileList.count()==0)
            files+="";
        else
            files+=",";
    }

    for(int i=0;i<strFileList.count();i++)
    {
        QFileInfo fileinfo(path+"/"+strFileList.at(i));
        //name
        if(inforType==0)
            files+=strFileList.at(i);
        //sizetext
        else if(inforType==1)
        {
            files+=getSizeText(fileinfo.size());
        }
        //type
        else if(inforType==2)
        {
            files+=QString("File");
        }
        //DateModified
        else if(inforType==3)
        {
            files+=(fileinfo.lastModified()).toString("yyyy-MM-dd hh:mm:ss");
        }
        else if(inforType==4)
        {
            files+=QString(to_string(fileinfo.size()).c_str());
        }
        if(i==strFileList.count()-1)
            files+="";
        else
            files+=",";
    }

    string tem=files.toStdString();
    int len=strlen(tem.c_str())+1;
    char* Files=new char[len];
    strcpy(Files,tem.c_str());

    return Files;
}

QString OrdinaryTransferTask::getSizeText(size_t sizes)
{
    int integepart=0;
    double decimpart=0;

    if(sizes<FILESIZE)
    {
        return QString("%1B").arg(sizes);
    }
    else if(sizes<FILESIZE*FILESIZE)
    {
        integepart=sizes/FILESIZE;
        decimpart=double(sizes%FILESIZE)/double(FILESIZE);
        decimpart=double(int(decimpart*100))/100;
        return QString("%1KB").arg(double(integepart)+decimpart);
    }
    else if(sizes<FILESIZE*FILESIZE*FILESIZE)
    {
        integepart=sizes/(FILESIZE*FILESIZE);
        decimpart=double(sizes%(FILESIZE*FILESIZE))/double(FILESIZE*FILESIZE);
        decimpart=double(int(decimpart*10))/10;
        return QString("%1MB").arg(double(integepart)+decimpart);
    }
    else
    {
        integepart=sizes/(FILESIZE*FILESIZE*FILESIZE);
        decimpart=double(sizes%(FILESIZE*FILESIZE*FILESIZE))/double(FILESIZE*FILESIZE*FILESIZE);
        decimpart=double(int(decimpart*100))/100;
        return QString("%1GB").arg(double(integepart)+decimpart);
    }
}

size_t OrdinaryTransferTask::getFolderSize(QString path)
{
    size_t allfilessize=0;
    QDir dir(path);

    //列出子目录
    QStringList strDirList=dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    //列出所有文件
    QStringList strFileList=dir.entryList(QDir::Files);

    for(int i=0;i<strFileList.count();i++)
    {
        QFileInfo fileinfo(path+"/"+strFileList.at(i));
        allfilessize+=fileinfo.size();
    }

    for(int i=0;i<strDirList.count();i++)
    {
        allfilessize+=getFolderSize(path+"/"+strDirList.at(i));
    }
    return allfilessize;
}

void OrdinaryTransferTask::timeout_cb(int fd, short event, void *argc)
{
    cbSet *cb_set=static_cast<cbSet*>(argc);

    if(!cb_set->iftaskcontinue)
        return;
    size_t totalTransmissionVolume=0;
    sscanf(cb_set->transfertask->m_data_segment_length.c_str(),"%zu",&totalTransmissionVolume);
    size_t transferredVolume=cb_set->transfertask->m_current_offset-cb_set->transfertask->m_begin_offset;
    //cout<<"transferredVolume:"<<transferredVolume<<endl;
    emit cb_set->transfertask->sendSchedule(totalTransmissionVolume,transferredVolume);
    //cout<<"send message!"<<endl;
    if((!cb_set->transfertask->ifScheduleLocked())&&transferredVolume<totalTransmissionVolume)
    {
        timeval tv={1,0};
        struct event* timeout_event=evtimer_new(cb_set->base,timeout_cb,cb_set);
        cb_set->timeout_event=timeout_event;
        event_add(timeout_event,&tv);
        //cout<<"add timeout!"<<endl;
    }
}

void OrdinaryTransferTask::transferTaskBreakOffProcess(cbSet *cb_set)
{
    //发送进度信号
    size_t totalTransmissionVolume=0;
    sscanf(cb_set->transfertask->m_data_segment_length.c_str(),"%zu",&totalTransmissionVolume);
    size_t transferredVolume=cb_set->transfertask->m_current_offset-cb_set->transfertask->m_begin_offset;
    emit cb_set->transfertask->sendSchedule(totalTransmissionVolume,transferredVolume);
    cb_set->iftaskcontinue=false;
    listenport.giveBackPort(cb_set->listeport);
    cb_set->transfertask->m_this_time_begin_offset=cb_set->transfertask->m_current_offset;
    TransferTask*t=dynamic_cast<TransferTask*>(cb_set->transfertask);
    emit cb_set->transfertask->task_break(t,nullptr);
}

void OrdinaryTransferTask::transferTaskEndProcess(cbSet *cb_set)
{
    //发送进度信号、结束信号
    size_t totalTransmissionVolume=0;
    sscanf(cb_set->transfertask->m_data_segment_length.c_str(),"%zu",&totalTransmissionVolume);
    size_t transferredVolume=cb_set->transfertask->m_current_offset-cb_set->transfertask->m_begin_offset;
    emit cb_set->transfertask->sendSchedule(totalTransmissionVolume,transferredVolume);
    cb_set->iftaskcontinue=false;
    listenport.giveBackPort(cb_set->listeport);
    cb_set->transfertask->m_this_time_begin_offset=cb_set->transfertask->m_current_offset;
    TransferTask*t=dynamic_cast<TransferTask*>(cb_set->transfertask);
    emit cb_set->transfertask->task_end(t,nullptr);
    qDebug()<<"send end sig!";
}

void OrdinaryTransferTask::connclose_cb(evhttp_connection *evcon, void *arg)
{
    cbSet *cb_set=static_cast<cbSet*>(arg);
    if(cb_set->getfilefd!=-1)
        close(cb_set->getfilefd);
}

void OrdinaryTransferTask::reqerro_cb(evhttp_request_error er, void *arg)
{
    if(er==EVREQ_HTTP_TIMEOUT)
        cout<<"time out !"<<endl;
    else if(er==EVREQ_HTTP_EOF)
        cout<<"reached EOF !"<<endl;
    else if(er==EVREQ_HTTP_INVALID_HEADER)
        cout<<"hreader wrong !"<<endl;
    else if(er==EVREQ_HTTP_BUFFER_ERROR)
        cout<<"read or write wrong !"<<endl;
    else if(er==EVREQ_HTTP_REQUEST_CANCEL)
        cout<<"evhttp_cancel_request() called on this request !"<<endl;
    else if(er==EVREQ_HTTP_DATA_TOO_LONG)
        cout<<"Body is greater !"<<endl;
}

string OrdinaryTransferTask::getFileNameFromUri(string uri)
{
    char path[uri.size()+1]={0};
    strcpy(path,uri.c_str());
    char* p=&path[0];

    char* q=p;
    p=strpbrk(p,"/");
    //cout<<"p:"<<p<<endl;
    while(p)
    {
        q=p;
        p++;
        p=strpbrk(p,"/");
    }
    q++;
    return string(q);
}

QStandardItemModel *OrdinaryTransferTask::translateStringToModel(vector<char *> dirinfor)
{
    const char* symbols=",()";

    auto i=dirinfor.cbegin();
    char* Files=*i++;
    //qDebug()<<files;

    vector<pair<char*,pair<char*,char*>>> vInforTypes;
    for(;i!=dirinfor.cend();i++)
    {
        char* types=*i;
        char* vitem=nullptr;
        char* vspecialSymbool=nullptr;
        vitem=types;
        vspecialSymbool=strpbrk(types,symbols);

        vInforTypes.push_back(make_pair(types,make_pair(vitem,vspecialSymbool)));
    }
    char* item=nullptr;
    char* specialSymbool=nullptr;

    stack<QStandardItem*> itemStack;
    QStandardItemModel* model=new QStandardItemModel();
    QStandardItem *rootItem=model->invisibleRootItem();
    itemStack.push(rootItem);

    item=Files;
    specialSymbool=strpbrk(Files,symbols);
    while(specialSymbool)
    {
        QStandardItem* childItem=nullptr;
        QList<QStandardItem*> InforList;
        char* p=nullptr;
        switch (*specialSymbool)
        {
        case ',':
            *specialSymbool='\0';
            for(auto s:vInforTypes)
            {
                *s.second.second='\0';
            }
            if(item!=specialSymbool)
            {

                childItem=new QStandardItem(QString(item));

                InforList<<childItem;
                for(auto s:vInforTypes)
                {
                    QStandardItem* childItems;
                    if(QString(s.second.first)=="0")
                    {
                        childItems=new QStandardItem(QString(""));
                        childItem->setIcon(QIcon(":/image/文件夹.png"));
                    }
                    else
                    {
                        childItems=new QStandardItem(QString(s.second.first));
                        //childItem->setIcon(QIcon(":/image/文件.png"));
                    }
                    InforList<<childItems;
                }
                itemStack.top()->appendRow(InforList);
            }
            p=item;
            item=specialSymbool+1;
            //delete[] p;
            specialSymbool=strpbrk(item,symbols);
            for(auto &s:vInforTypes)
            {
                s.second.first=s.second.second+1;
                s.second.second=strpbrk(s.second.first,symbols);
            }

            break;
        case '(':
            *specialSymbool='\0';
            for(auto s:vInforTypes)
            {
                *s.second.second='\0';
            }
            childItem=new QStandardItem(QString(item));
            InforList<<childItem;
            for(auto s:vInforTypes)
            {
                QStandardItem* childItems;
                if(QString(s.second.first)=="0")
                {
                    childItems=new QStandardItem(QString(""));
                    childItem->setIcon(QIcon(":/image/文件夹.png"));
                }
                else
                {
                    childItems=new QStandardItem(QString(s.second.first));
                    //childItem->setIcon(QIcon(":/image/文件.png"));
                }
                InforList<<childItems;
            }
            itemStack.top()->appendRow(InforList);
            itemStack.push(childItem);

            p=item;
            item=specialSymbool+1;
            //delete[] p;
            specialSymbool=strpbrk(item,symbols);
            for(auto &s:vInforTypes)
            {
                s.second.first=s.second.second+1;
                s.second.second=strpbrk(s.second.first,symbols);
            }
            break;
        case ')':
            *specialSymbool='\0';
            for(auto s:vInforTypes)
            {
                *s.second.second='\0';
            }
            if(item!=specialSymbool)
            {
                childItem=new QStandardItem(QString(item));
                InforList<<childItem;
                for(auto s:vInforTypes)
                {
                    QStandardItem* childItems;
                    if(QString(s.second.first)=="0")
                    {
                        childItems=new QStandardItem(QString(""));
                        childItem->setIcon(QIcon(":/image/文件夹.png"));
                    }
                    else
                    {
                        childItems=new QStandardItem(QString(s.second.first));
                        //childItem->setIcon(QIcon(":/image/文件.png"));
                    }
                    InforList<<childItems;
                }
                itemStack.top()->appendRow(InforList);
            }
            itemStack.pop();
            p=item;
            item=specialSymbool+1;
            //delete[] p;
            specialSymbool=strpbrk(item,symbols);
            for(auto &s:vInforTypes)
            {
                s.second.first=s.second.second+1;
                s.second.second=strpbrk(s.second.first,symbols);
            }
            break;
        default:
            return nullptr;

        }
    }
    if(item)
    {
        QList<QStandardItem*> InforList;
        QStandardItem* childItem=new QStandardItem(QString(item));
        InforList<<childItem;
        for(auto s:vInforTypes)
        {
            QStandardItem* childItems;
            if(QString(s.second.first)=="0")
            {
                childItems=new QStandardItem(QString(""));
                childItem->setIcon(QIcon(":/image/文件夹.png"));
            }
            else
            {
                childItems=new QStandardItem(QString(s.second.first));
                //childItem->setIcon(QIcon(":/image/文件.png"));
            }
            InforList<<childItems;
        }
        itemStack.top()->appendRow(InforList);
    }
    for(auto s:dirinfor)
    {
        if(s)
            delete[] s;
    }
    QStringList list;
    list<<"Name"<<"Size"<<"Type"<<"Date Modified";
    model->setHorizontalHeaderLabels(list);
    return model;
}

int OrdinaryTransferTask::httpClientStart()
{
#ifdef _WIN32
    //初始化socket库
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2),&wsa);
#else \
    //忽略管道信号，发送数据给已关闭的socket
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
        return 1;
#endif
}
