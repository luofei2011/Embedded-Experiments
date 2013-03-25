#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtCore>
#include "posix_qextserialport.h"
#include <QMessageBox>
#include <QFile>
#include <QInputDialog>
#include <QFileDialog>
#include <QTextStream>

namespace Ui {
    class MainWindow;
}

//延时，TIME_OUT是串口读写的延时
#define TIME_OUT 10

//读取定时器计时间隔,200ms，读取定时器是我们读取串口缓存的延时
#define TIMER_INTERVAL 200

//连续发送定时器计时间隔,200ms
#define OBO_TIMER_INTERVAL 200


class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::MainWindow *ui;
    int serial_status;
    Posix_QextSerialPort *myCom;
    QTimer *timer;
    QTimer *obotimer;
    QTimer *mySaveTimer;
    QTimer *myTimer;

    //初始化接受数据大小
    int SendCount;
    int ReceiveCount;

    unsigned int timerdly;
    unsigned int obotimerdly;

private slots:
    void on_pushButton_clicked();
    void on_pushButton_2_clicked();
    void on_pushButton_4_clicked();
    void on_pushButton_5_clicked();
    QByteArray GetHexValue(QString str);
    char ConvertHexChar(char ch);
    void SaveData();
    void writeFile();
    void GetDateTime();
    void ReadCom();
    void WriteCom();
};

#endif // MAINWINDOW_H
