#include "taskscheduleshowd.h"
#include "ui_taskscheduleshowd.h"
#include "transfertask.h"
int TaskScheduleShowd::buttonW=16;
int TaskScheduleShowd::buttonH=16;
static const size_t BYTESIZE=1024;
TaskScheduleShowd::TaskScheduleShowd(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TaskScheduleShowd)
{
    ui->setupUi(this);


    setPalette(QPalette(Qt::white));
    but=nullptr;
    but1=nullptr;
    setAutoFillBackground(true);

    connect(this,&TaskScheduleShowd::resetButtomSig,this,&TaskScheduleShowd::resetButtom);//Qt::QueuedConnection
        this->setMaximumSize(600,80);
        this->setMinimumSize(1500,80);
    this->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred));
}

TaskScheduleShowd::~TaskScheduleShowd()
{
    delete ui;
}
void TaskScheduleShowd::init(string file,int state)
{
    this->filename=file;
    this->taskstate=state;
    this->filesize=0; //BYTESIZE*BYTESIZE*BYTESIZE*size_t(1023)
    this->sendsize=0; //BYTESIZE*BYTESIZE*BYTESIZE*size_t(580)
    setButtom(this->taskstate);
}

QString TaskScheduleShowd::textSchedule()
{
    return getSizeText(sendsize).append(QString("/")).append(getSizeText(filesize));
}

QString TaskScheduleShowd::textTransferRate()
{
    return getSizeText(SendSize).append(QString("/S"));
}

void TaskScheduleShowd::setButtom(int state)
{
    int W=this->width();
    int H=this->height();

    if(but!=nullptr)
    {
        but->setParent(nullptr);
        delete but;
        //but->deleteLater();
    }
    if(but1!=nullptr)
    {
        but1->setParent(nullptr);
        delete but1;
        //but1->deleteLater();
    }
    if(state==0)
    {
        but=new QPushButton(this);
        but->setIcon(QIcon(":/image/stop.png"));
        but->resize(14,14);
        but->move(W-38,(2*H/5)+(H/10)-7);
        but->show();
        but1=nullptr;
        connect(but,&QPushButton::clicked,this,&TaskScheduleShowd::slot_task_stop);
    }
    else if(state==1)
    {
        but=new QPushButton(this);
        but->setIcon(QIcon(":/image/wait.png"));
        but->resize(16,16);
        but->move(W-38,(2*H/5)+(H/10)-7);
        but->show();
        but1=nullptr;
        connect(but,&QPushButton::clicked,this,&TaskScheduleShowd::slot_task_stop);
    }
    else
    {
        but=new QPushButton(this);
        but->setIcon(QIcon(":/image/start.png"));
        but1=new QPushButton(this);
        but1->setIcon(QIcon(":/image/deletw.png"));
        but->resize(16,16);
        but1->resize(16,16);
        but->move(W-38,(2*H/5)+(H/10)-7);
        but1->move(W-22,(2*H/5)+(H/10)-7);
        but->show();
        but1->show();
        connect(but,&QPushButton::clicked,this,&TaskScheduleShowd::slot_task_start);
        connect(but1,&QPushButton::clicked,this,&TaskScheduleShowd::slot_task_delete);
    }


}

QString TaskScheduleShowd::getSizeText(size_t sizes)
{
    int integepart=0;
    double decimpart=0;

    if(sizes<BYTESIZE)
    {
        return QString("%1B").arg(sizes);
    }
    else if(sizes<BYTESIZE*BYTESIZE)
    {
        integepart=sizes/BYTESIZE;
        decimpart=double(sizes%BYTESIZE)/double(BYTESIZE);
        decimpart=double(int(decimpart*100))/100;
        return QString("%1KB").arg(double(integepart)+decimpart);
    }
    else if(sizes<BYTESIZE*BYTESIZE*BYTESIZE)
    {
        integepart=sizes/(BYTESIZE*BYTESIZE);
        decimpart=double(sizes%(BYTESIZE*BYTESIZE))/double(BYTESIZE*BYTESIZE);
        decimpart=double(int(decimpart*10))/10;
        return QString("%1MB").arg(double(integepart)+decimpart);
    }
    else
    {
        integepart=sizes/(BYTESIZE*BYTESIZE*BYTESIZE);
        decimpart=double(sizes%(BYTESIZE*BYTESIZE*BYTESIZE))/double(BYTESIZE*BYTESIZE*BYTESIZE);
        decimpart=double(int(decimpart*100))/100;
        return QString("%1GB").arg(double(integepart)+decimpart);
    }
}

void TaskScheduleShowd::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    int W=this->width();
    int H=this->height();
//绘制文件名
    QRect namerect(40,H/10,W-80,4*H/10);
    QFont namefont;
    namefont.setPointSize(13);
    namefont.setBold(true);
    painter.setFont(namefont);
    painter.drawText(namerect,QString(filename.c_str()));
//绘制进度条
    QRect allscherect(40,(2*H/5)+(H/10)-5,W-80,10);
    QPen allschepen;
    allschepen.setColor(Qt::black);
    painter.setPen(allschepen);
    QBrush allschebrush;
    allschebrush.setColor(Qt::white);
    allschebrush.setStyle(Qt::SolidPattern);
    painter.setBrush(allschebrush);
    painter.drawRoundedRect(allscherect,5,5);

    double sche=double(sendsize)/double(filesize);
    //qDebug()<<"SendSize:"<<sendsize<<" FileSize:"<<filesize;
    //qDebug()<<"sche:"<<sche;
    //sche=0.4;
    QRect scherect(40,(2*H/5)+(H/10)-5,sche*(W-80),10);
    QPen schepen;
    schepen.setColor(Qt::white);
    painter.setPen(schepen);
    QBrush schebrush;
    schebrush.setColor(Qt::blue);
    schebrush.setStyle(Qt::SolidPattern);
    painter.setBrush(schebrush);
    painter.drawRoundedRect(scherect,5,5);
//绘制文字进度条
    QRect nametextrect(40,3*H/5,W-80,2*H/5);//40,3*H/5,W-80,H/5
    QFont nametextfont;
    nametextfont.setPointSize(12);
    nametextfont.setBold(true);
    painter.setFont(nametextfont);
    //qDebug()<<textSchedule();
    QPen nametextpen;
    nametextpen.setColor(Qt::black);
    painter.setPen(nametextpen);
    painter.drawText(nametextrect,textSchedule());
//绘制传输速率
    QRect transferratetextrect(W-110,3*H/5,W-80,2*H/5);//40,3*H/5,W-80,H/5
    QFont transferratetextfont;
    transferratetextfont.setPointSize(12);
    transferratetextfont.setBold(true);
    painter.setFont(transferratetextfont);
    //qDebug()<<textSchedule();
    QPen transferratetextpen;
    transferratetextpen.setColor(Qt::black);
    painter.setPen(transferratetextpen);
    painter.drawText(transferratetextrect,textTransferRate());
}

void TaskScheduleShowd::processScheduleSig(size_t file_size, size_t send_size)
{
    SendSize=send_size-sendsize;
    filesize=file_size;
    sendsize=send_size;
    textSchedule();
    update();
}
