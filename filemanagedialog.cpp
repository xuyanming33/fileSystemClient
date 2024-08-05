#include "filemanagedialog.h"
#include "ui_filemanagedialog.h"
#include <QPushButton>

FileManageDialog::FileManageDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FileManageDialog)
{
    ui->setupUi(this);
    //addChoose();
}

FileManageDialog::~FileManageDialog()
{
    delete ui;
}

void FileManageDialog::addChoose()
{
    QPushButton* btn=new QPushButton(this);
    btn->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Preferred);
    btn->setText("上传文件夹(该目录下)");
    connect(btn,&QPushButton::clicked,this,&FileManageDialog::slotAddFolderToCurrentDir);
    ui->verticalLayout->insertWidget(1,btn,1,Qt::AlignTop);

    QPushButton* btn1=new QPushButton(this);
    btn1->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Preferred);
    btn1->setText("上传文件(该目录下)");
    connect(btn1,&QPushButton::clicked,this,&FileManageDialog::slotAddFileToCurrentDir);
    ui->verticalLayout->insertWidget(2,btn1,1,Qt::AlignTop);
}

//更新文件系统
void FileManageDialog::on_updateFileSystemPushButton_clicked()
{
    callselection=1;
    this->close();
}

//添加空目录
void FileManageDialog::on_addEmptyDirPushButton_clicked()
{
    QString dlgTitle="新建目录名";
    QString txtLabel="请输入新建目录名";
    QString defaultInput="新建目录";
    bool ok=false;
    QString text=QInputDialog::getText(this,dlgTitle,txtLabel,QLineEdit::Normal,defaultInput,&ok);
    callselection=2;
    method="POST";
    fileOrDirname=text.toStdString();
    fileOrDir=false;
    this->close();
}

//上传文件夹(该目录下)
void FileManageDialog::slotAddFolderToCurrentDir()
{
    QString curpath=QCoreApplication::applicationDirPath();
    QString dlgTitle="选择文件夹";
    QString selectedDir=QFileDialog::getExistingDirectory(this,dlgTitle,curpath+"/FileSystem",QFileDialog::ShowDirsOnly);
    callselection=3;
    method="POST";
    fileOrDirname=selectedDir.toStdString();
    fileOrDir=false;
    this->close();
}

//上传文件(该目录下)
void FileManageDialog::slotAddFileToCurrentDir()
{
    QString curpath=QDir::currentPath();
    QString dlgTitle="选择文件";
    QString filter="";
    QString file=QFileDialog::getOpenFileName(this,dlgTitle,curpath+"/FileSystem",filter);
    callselection=4;
    method="POST";
    fileOrDirname=file.toStdString();
    fileOrDir=true;
    this->close();
}

//上传文件夹(同级目录)
void FileManageDialog::on_addDirPushButton_clicked()
{
    QString curpath=QCoreApplication::applicationDirPath();
    QString dlgTitle="选择文件夹";
    QString selectedDir=QFileDialog::getExistingDirectory(this,dlgTitle,curpath+"/FileSystem",QFileDialog::ShowDirsOnly);
    callselection=5;
    method="POST";
    fileOrDirname=selectedDir.toStdString();
    fileOrDir=false;
    this->close();
}

//上传文件(同级目录)
void FileManageDialog::on_addFilePushButton_clicked()
{
    QString curpath=QDir::currentPath();
    QString dlgTitle="选择文件";
    QString filter="";
    QString file=QFileDialog::getOpenFileName(this,dlgTitle,curpath+"/FileSystem",filter);
    callselection=6;
    method="POST";
    fileOrDirname=file.toStdString();
    fileOrDir=true;
    this->close();
}

//下载文件夹/文件
void FileManageDialog::on_upLoadPushButton_clicked()
{
    QString curPath=QCoreApplication::applicationDirPath();
    QString dlgTitle="选择一个目录";
    QString selectedDir=QFileDialog::getExistingDirectory(this,dlgTitle,curPath+"/FileSystem",QFileDialog::ShowDirsOnly);
    callselection=7;
    method="GET";
    fileOrDirname=selectedDir.toStdString();
    fileOrDir=false;
    this->close();
}

//删除文件夹/文件
void FileManageDialog::on_deletePushButton_clicked()
{
    callselection=8;
    method="DELETE";
    this->close();
}






