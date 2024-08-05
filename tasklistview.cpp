#include "tasklistview.h"

TaskListView::TaskListView(QWidget *parent)
    : QWidget{parent}
{
    setPalette(QPalette(Qt::white));
    initComponent();
}

void TaskListView::delete_task_schedule(TaskScheduleShowd *schedule_widget)
{
    m_pSCVLayout->removeWidget(schedule_widget);
    //schedule_widget->deleteLater();
}

void TaskListView::insert_task_schedule(int task_index, TaskScheduleShowd *schedule_widget)
{
    schedule_widget->setParent(widget);

    //m_pSCVLayout->addWidget(schedule_widget);
    m_pSCVLayout->insertWidget(task_index,schedule_widget,0,Qt::AlignLeft|Qt::AlignTop);
}

void TaskListView::add_pushbuttom(QWidget *b)
{
    b->setParent(widget);
    //m_pSCVLayout->addWidget(schedule_widget);
    m_pSCVLayout->addWidget(b);
}

QWidget *TaskListView::getWidget()
{
    return widget;
}

void TaskListView::initComponent()
{
    this->m_pChatListScrollArea = new QScrollArea(this);
    this->m_pChatListScrollArea->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred));
    this->m_pChatListScrollArea->setWidgetResizable(true);

    widget = new QWidget(this);
    widget->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred));
//    widget->setMinimumSize(400, 800);
//    widget->setMaximumSize(960,960);
    this->m_pSCVLayout = new QVBoxLayout(this);
    this->m_pSCVLayout->setSizeConstraint(QVBoxLayout::SetMinAndMaxSize);
    widget->setLayout(this->m_pSCVLayout);
    this->m_pChatListScrollArea->setWidget(widget);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(this->m_pChatListScrollArea);
    this->setLayout(mainLayout);
}

void TaskListView::slotDeleteTaskSchedule(TaskScheduleShowd *schedule_widget)
{
    delete_task_schedule(schedule_widget);
}

void TaskListView::slotInsertTaskSchedule(int task_index, TaskScheduleShowd *schedule_widget)
{
    insert_task_schedule(task_index,schedule_widget);
}
