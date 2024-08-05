#ifndef TASKLISTVIEW_H
#define TASKLISTVIEW_H

#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>
#include <QPushButton>
#include "taskscheduleshowd.h"
class TaskListView : public QWidget
{
    Q_OBJECT
public:
    explicit TaskListView(QWidget *parent = nullptr);

    //删除某控件的函数
    void delete_task_schedule(TaskScheduleShowd* schedule_widget);
    //添加某控件的函数
    void insert_task_schedule(int task_index,TaskScheduleShowd* schedule_widget);

    void add_pushbuttom(QWidget* b);
    QWidget* getWidget();
signals:

private:
    QScrollArea* m_pChatListScrollArea;
    QVBoxLayout* m_pSCVLayout;
    QWidget* widget;

    void initComponent();
public slots:
    //删除某控件的函数
    void slotDeleteTaskSchedule(TaskScheduleShowd* schedule_widget);
    //添加某控件的函数
    void slotInsertTaskSchedule(int task_index,TaskScheduleShowd* schedule_widget);

};

#endif // TASKLISTVIEW_H
