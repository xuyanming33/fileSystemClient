#include "mainwindow.h"
#include "ordinarytransfertask.h"
#include <QApplication>
#include "taskscheduleshowd.h"
#include <QMetaType>
#include <QTextCodec>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    if(QT_VERSION>=QT_VERSION_CHECK(5,6,0))
        QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    MainWindow w;


//    QFile file(":/qss/Ubuntu.qss");
//    file.open(QFile::ReadOnly);
//    QTextStream filetext(&file);
//    QString stylesheet=filetext.readAll();
//    w.setStyleSheet(stylesheet);

    w.show();

    return a.exec();
}
