#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    serial_status = 0;
     ui->statusBar->showMessage(tr("串口打开状态"));

     //初始化读取定时器计时间隔
     timerdly = TIMER_INTERVAL;
     //初始化接受数据大小
     SendCount = 0;
     ReceiveCount = 0;

     //初始化连续发送计时器计时间隔
     obotimerdly = OBO_TIMER_INTERVAL;

     //设置读取计时器
     timer = new QTimer(this);
     //设置读取时间间隔
     timer->setInterval(200);
     connect(timer, SIGNAL(timeout()), this, SLOT(ReadCom()));

     //发送计时器
     obotimer = new QTimer(this);
     //设置发送间隔
     //obotimer->setInterval(5000);
     //connect(obotimer, SIGNAL(timeout()), this, SLOT(WriteCom()));

     //显示日期时间
     myTimer=new QTimer(this);
     myTimer->start(1000);
     connect(myTimer,SIGNAL(timeout()),this,SLOT(GetDateTime()));
}

void MainWindow::GetDateTime()
{
    QTime timeNow=QTime::currentTime();
    ui->labTime->setText(tr("Date:%1").arg(timeNow.toString()));
    ui->labDate->setText(tr("Time:%1").arg(QDate::currentDate().toString(tr("yyyy-MM-dd- dddd"))));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void MainWindow::on_pushButton_clicked()
{
    QString path;
    QString portName="/dev/"+ui->comboBox->currentText(); //得到串口名字

    if(serial_status == 0){
        path = "2.jpg";
        ui->pushButton->setText(tr("关闭串口"));
        ui->label->setPixmap(path);
        ui->comboBox->setEnabled(false);
        ui->comboBox_2->setEnabled(false);
        ui->comboBox_3->setEnabled(false);
        ui->comboBox_4->setEnabled(false);
        ui->comboBox_5->setEnabled(false);
        /*ui->ckHexReceive->setEnabled(false);
        ui->ckHexSend->setEnabled(false);
        ui->txtSend->setEnabled(false);
        ui->pushButton_2->setEnabled(false);
        ui->pushButton_3->setEnabled(false);*/

        myCom = new Posix_QextSerialPort(portName, QextSerialBase::Polling);

        if(myCom->open(QIODevice::ReadWrite)){
            QMessageBox::information(this, tr("打开成功"), tr("已成功打开串口 ") + portName, QMessageBox::Ok);
        }else{
            QMessageBox::critical(this, tr("打开失败"), tr("未能打开串口 ") + portName + tr("\n该串口设备不存在或已被占用"), QMessageBox::Ok);
            return;
        }

        //设置波特率
        myCom->setBaudRate((BaudRateType)ui->comboBox_2->currentIndex());

        //设置校验
        myCom->setParity((ParityType)ui->comboBox_3->currentIndex());

        //设置数据位
        myCom->setDataBits((DataBitsType)ui->comboBox_4->currentIndex());

        //设置停止位
        myCom->setStopBits((StopBitsType)ui->comboBox_5->currentIndex());

        //开启读取定时器
        timer->start(timerdly);
        obotimer->start(timerdly);
        myTimer->start(timerdly);
        //设置数据流控制
        myCom->setFlowControl(FLOW_OFF);
        //设置延时
        myCom->setTimeout(TIME_OUT);

        serial_status = 1;

        ui->statusBar->showMessage(tr("串口打开成功"));
    }
    else{

        myCom->close();
        delete myCom;
        //关闭所有计时器
        timer->stop();
        obotimer->stop();
        myTimer->stop();

        path = "1.jpg";
        ui->pushButton->setText(tr("打开串口"));
        ui->label->setPixmap(path);
        ui->comboBox->setEnabled(true);
        ui->comboBox_2->setEnabled(true);
        ui->comboBox_3->setEnabled(true);
        ui->comboBox_4->setEnabled(true);
        ui->comboBox_5->setEnabled(true);
        ui->ckHexReceive->setEnabled(true);
        ui->ckHexSend->setEnabled(true);
        ui->txtSend->setEnabled(true);
        ui->pushButton_2->setEnabled(true);
        serial_status = 0;
        ui->statusBar->showMessage(tr("串口已经关闭"));
    }
}

void MainWindow::on_pushButton_2_clicked(){
    this->WriteCom();
}

void MainWindow::on_pushButton_4_clicked(){
    this->SaveData();
}

void MainWindow::on_pushButton_5_clicked(){
    this->writeFile();
}

void MainWindow::ReadCom()
{
   // MyHelper::DelayTime(100);
    QString strHex;//16进制数据
    QString strNor;//正常数据
    QByteArray temp = myCom->readAll();
    if(!temp.isEmpty()){
        QDataStream out(&temp,QIODevice::ReadWrite);
        while (!out.atEnd())
        {
                    qint8 outChar=0;
                    out>>outChar;
                    QString str=QString("%1").arg(outChar&0xFF,2,16,QLatin1Char('0'));

                    if (str.length()>1)
                    {
                        strHex+=str+" ";
                    }
                    else
                    {
                        strHex+="0"+str+" ";
                    }
        }

        strNor = temp;//正常数据

        if(ui->ckHexReceive->isChecked()){
            ui->textBrowser_6->setTextColor(Qt::red);
            ui->textBrowser_6->append(tr("接收"));
            ui->textBrowser_6->setTextColor(Qt::yellow);
            ui->textBrowser_6->append(strHex.toUpper());
        }
        else{
            ui->textBrowser_6->setTextColor(Qt::red);
            ui->textBrowser_6->append(tr("接收"));
            ui->textBrowser_6->setTextColor(Qt::yellow);
            ui->textBrowser_6->append(strNor);
        }

        ReceiveCount += temp.size();
        ui->labReceive->setText(tr("接收:%1 字节").arg(ReceiveCount));
    }
}

//获得十六进制数据
QByteArray MainWindow::GetHexValue(QString str)
{
    QByteArray senddata;
    int hexdata,lowhexdata;
    int hexdatalen = 0;
    int len = str.length();
    senddata.resize(len/2);
    char lstr,hstr;
    for(int i=0; i<len;)
    {
        hstr=str[i].toAscii();
        if(hstr == ' ')
        {
            i++;
            continue;
        }
        i++;
        if(i >= len)
            break;
        lstr = str[i].toAscii();
        hexdata = ConvertHexChar(hstr);
        lowhexdata = ConvertHexChar(lstr);
        if((hexdata == 16) || (lowhexdata == 16))
            break;
        else
            hexdata = hexdata*16+lowhexdata;
        i++;
        senddata[hexdatalen] = (char)hexdata;
        hexdatalen++;
    }
    senddata.resize(hexdatalen);
    return senddata;

}

char MainWindow::ConvertHexChar(char ch)
{
    if((ch >= '0') && (ch <= '9'))
        return ch-0x30;
    else if((ch >= 'A') && (ch <= 'F'))
        return ch-'A'+10;
    else if((ch >= 'a') && (ch <= 'f'))
        return ch-'a'+10;
    else return (-1);
}

void MainWindow::WriteCom()
{
    QString str = ui->txtSend->text();
    QByteArray outData = str.toAscii();

    int size = outData.size();

    if(!myCom->isOpen()){
        return;
    }

    if(outData.isEmpty()){
        ui->txtSend->setFocus();
        return;
    }

    if(ui->ckHexSend->isChecked()){
        outData = this->GetHexValue(str);
        size = outData.size();
        myCom->write(outData);
    }
    else{
        size = outData.size();
        myCom->write(outData);
    }

   SendCount = SendCount + size;
   ui->labSend->setText(tr("发送:%1 字节").arg(SendCount));

   ui->textBrowser_6->setTextColor(Qt::blue);
   ui->textBrowser_6->append(tr("发送"));
   ui->textBrowser_6->setTextColor(Qt::yellow);
   ui->textBrowser_6->append(outData);
   ui->txtSend->setText(outData);

}


void MainWindow::SaveData()
{
    QDateTime now=QDateTime::currentDateTime();
    QString name=now.toString("yyyyMMddHHmmss");
    QString fileName=QCoreApplication::applicationDirPath()+"/"+name+".txt";
    //如果没有内容则不保存
    if (ui->textBrowser_6->document()->toPlainText().length()==0)
    {
        return;
    }
    QFile file(fileName);
    file.open(QFile::WriteOnly | QIODevice::Text);
    QTextStream out(&file);
    out<<ui->textBrowser_6->document()->toPlainText();
    file.close();
}

void MainWindow::writeFile(){
    //myCom->right();
    QByteArray temp;
    QString file_path;
    file_path = QFileDialog::getOpenFileName(this);
    QFile file(file_path);
    file.open(QIODevice::ReadOnly);
    QTextStream t(&file);
    temp = (t.readAll()).toAscii();
    myCom->write(temp);
    file.close();
}
