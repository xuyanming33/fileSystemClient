#ifndef TASKSCHEDULESHOWD_H
#define TASKSCHEDULESHOWD_H

#include <QWidget>
#include <string>
#include <iostream>
#include <QString>
#include <QPainter>
#include <QPushButton>
#include <QDebug>
#include <sys/stat.h>
#include <fcntl.h>
#include <QStyledItemDelegate>
using namespace std;

class TransferTask;
QT_BEGIN_NAMESPACE
namespace Ui {
class TaskScheduleShowd;
}
QT_END_NAMESPACE

class TaskScheduleShowd : public QWidget
{
    Q_OBJECT

public:
    explicit TaskScheduleShowd(QWidget *parent = nullptr);
    ~TaskScheduleShowd();
    //初始化
    void init(string file,int state);
    //设置按钮
    void setButtom(int state);

    string getfilename()
    {
        string s=this->filename;
        return s;
    }
private:
    Ui::TaskScheduleShowd *ui;
    //文件大小
    size_t filesize=0;
    size_t FileSize=0;
    //已传输字节
    size_t sendsize=0;
    size_t SendSize=0;
    //文件名称
    string filename="";
    //任务类型
    int taskstate;
    //按钮大小
    static int buttonW;
    static int buttonH;
    QPushButton *but;
    QPushButton *but1;

    //获取文件大小文本
    QString getSizeText(size_t sizes);
public:
    //设置文字进度条
    QString textSchedule();
    //设置传输速率
    QString textTransferRate();
protected:
    void paintEvent(QPaintEvent* event) Q_DECL_OVERRIDE;

signals:
    //发送暂停信号
    void task_stop(TransferTask* trantask,TaskScheduleShowd* scheDule);
    //发送开始信号
    void task_start(TransferTask* trantask,TaskScheduleShowd* scheDule);
    //发送删除信号
    void task_delete(TransferTask* trantask,TaskScheduleShowd* scheDule);
    //发送重新设置进度条样式信号
    void resetButtomSig(int state);
public slots:
    //接收信号，触发重绘
    void processScheduleSig(size_t file_size,size_t send_size);
    //重新设置进度条样式
    void resetButtom(int state)
    {
        setButtom(state);
    }

    void slot_task_stop()
    {
        emit task_stop(nullptr,this);
    }
    void slot_task_start()
    {
        emit task_start(nullptr,this);
    }
    void slot_task_delete()
    {
        emit task_delete(nullptr,this);
    }
};

#endif // TASKSCHEDULESHOWD_H
