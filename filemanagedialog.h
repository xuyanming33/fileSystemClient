#ifndef FILEMANAGEDIALOG_H
#define FILEMANAGEDIALOG_H

#include <QDialog>
#include <QInputDialog>
#include <QDebug>
#include <string>
#include <QFileDialog>
namespace Ui {
class FileManageDialog;
}

class FileManageDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FileManageDialog(QWidget *parent = nullptr);
    ~FileManageDialog();

    void addChoose();

    //方法、uri、文件/目录名、文件/目录
    int callselection=0;
    std::string method;
    std::string uri;
    std::string fileOrDirname;
    bool fileOrDir;

private slots:
    //更新文件系统
    void on_updateFileSystemPushButton_clicked();
    //添加空目录
    void on_addEmptyDirPushButton_clicked();
    //上传文件夹(该目录下)
    void slotAddFolderToCurrentDir();
    //上传文件(该目录下)
    void slotAddFileToCurrentDir();
    //上传文件夹(同级目录)
    void on_addDirPushButton_clicked();
    //上传文件(同级目录)
    void on_addFilePushButton_clicked();
    //下载文件夹/文件
    void on_upLoadPushButton_clicked();
    //删除文件夹/文件
    void on_deletePushButton_clicked();



private:
    Ui::FileManageDialog *ui;

};

#endif // FILEMANAGEDIALOG_H
