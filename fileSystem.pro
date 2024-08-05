QT       += core gui
QT       += widgets
QT       += network


LIBS += -levent
LIBS += -levent_core
LIBS += -levent_extra
LIBS += -levent_pthreads

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES +=  \
    filemanagedialog.cpp \
    logindialog.cpp \
    main.cpp \
    mainwindow.cpp \
    ordinarytransfertask.cpp \
    supertransfertask.cpp \
    tasklistview.cpp \
    taskqueuemanage.cpp \
    taskscheduleshowd.cpp \
    transfertask.cpp \
    workthreadmanage.cpp


HEADERS += \
    filemanagedialog.h \
    logindialog.h \
    mainwindow.h \
    ordinarytransfertask.h \
    supertransfertask.h \
    tasklistview.h \
    taskqueuemanage.h \
    taskscheduleshowd.h \
    transfertask.h \
    workthreadmanage.h


FORMS += \
    filemanagedialog.ui \
    logindialog.ui \
    mainwindow.ui \
    taskscheduleshowd.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    res.qrc
