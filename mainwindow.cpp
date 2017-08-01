#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QUdpSocket>
#include "udpclient.h"
#include <QLabel>
#include <QDate>
#include "commonhelper.h"
#include <QSettings>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonDocument>
#include <QDateTime>
#include <QTimer>
#include <QDate>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    this->ip_now=12;
    qDebug("%s", __func__);
    ui->setupUi(this);
    static const QDate buildDate = QLocale( QLocale::English ).toDate( QString( __DATE__ ).replace( "  ", " 0" ), "MMM dd yyyy");

    qDebug()<<"V"<<buildDate<<buildDate.toString("yyyyMMdd");
    this->setWindowTitle(QString("LSC Tools调试工具 版本@:%1  By ®©  shaorongqiang@jiahuitech.com").arg(buildDate.toString("yyyyMMdd")));
    /* 读取配置文件 */
    doSettings(false);

    /* 设置默认通讯模式 */
    ui->tcpclient_radioButton->setChecked(true);
    /** 目前设置为UDP为默认方式 */
    ui->udp_radioButton->setChecked(true);
    /** 设置远程主机IP地址 获取本机IP */
    ui->remoteIP_lineEdit->setText(mRemoteIp);
    /* 设置远程端口号 */
    /* TODO: 将其设置为不能以0开头 */
    ui->remoteport_spinBox->setRange(1024,65533);
    ui->remoteport_spinBox->setValue(mRemotePort);
    /* 设置本地端口号 */
    ui->localport_spinBox->setRange(1024,65533);
    ui->localport_spinBox->setValue(mLocalPort);

    isConnect = false;

    // 状态栏
    statusLabel = new QLabel;
    statusLabel->setMinimumSize(260, 20); // 设置标签最小大小
    statusLabel->setFrameShape(QFrame::WinPanel); // 设置标签形状
    statusLabel->setFrameShadow(QFrame::Sunken); // 设置标签阴影
    ui->statusBar->addWidget(statusLabel);
    statusLabel->setText("UDP通信停止");
    statusLabel->setAlignment(Qt::AlignHCenter);

    // 接收数量
    receiveLabel = new QLabel;
    receiveLabel->setMinimumSize(150, 20); // 设置标签最小大小
    receiveLabel->setFrameShape(QFrame::WinPanel); // 设置标签形状
    receiveLabel->setFrameShadow(QFrame::Sunken); // 设置标签阴影
    ui->statusBar->addWidget(receiveLabel);
    receiveLabel->setAlignment(Qt::AlignHCenter);

    // 发送数量
    sendLabel = new QLabel;
    sendLabel->setMinimumSize(150, 20); // 设置标签最小大小
    sendLabel->setFrameShape(QFrame::WinPanel); // 设置标签形状
    sendLabel->setFrameShadow(QFrame::Sunken); // 设置标签阴影
    ui->statusBar->addWidget(sendLabel);
    sendLabel->setAlignment(Qt::AlignHCenter);
    updateStateBar(QString(), 0, 0);

    // 计数器清零 button
    clearCounterButton = new QPushButton();
    ui->statusBar->addWidget(clearCounterButton);
    clearCounterButton->setText(tr("计数器清零"));
    connect(clearCounterButton, SIGNAL(released()), this, SLOT(on_clearCounter_pushButton_released()));

    // 时间 TODO:要进行更新
    timeLabel = new QLabel;
    timeLabel->setMinimumSize(90, 20); // 设置标签最小大小
    timeLabel->setMaximumWidth(90);
    timeLabel->setFrameShape(QFrame::WinPanel); // 设置标签形状
    timeLabel->setFrameShadow(QFrame::Sunken); // 设置标签阴影
    ui->statusBar->addWidget(timeLabel);
    timeLabel->setText(QDate::currentDate().toString("yyyy-MM-dd"));

    // 更新接收到的数据
    connect(&client, SIGNAL(valueChanged(QString,QHostAddress)), this, SLOT(updateReceiveText(QString,QHostAddress)));
    connect(&client,
            SIGNAL(updateState(QString, QVariant, QVariant)),
            this, SLOT(updateStateBar(QString, QVariant, QVariant)));

    init();

    mReceiveNum = mSendNum = 0;
    ui->toolBox->setCurrentIndex(0);
    this->timer_send=new QTimer(this);
    connect(this->timer_send,SIGNAL(timeout()),this,SLOT(on_handSend_pushButton_released()));
}

void MainWindow::connectNet()
{
    qDebug("%s", __func__);

    mRemoteIp = ui->remoteIP_lineEdit->text();
    mRemotePort = ui->remoteport_spinBox->text().toInt();
    mLocalPort = ui->localport_spinBox->text().toInt();
    updateStateBar("UDP通信 " + mRemoteIp + ":" + QString().number(mRemotePort),
                   QVariant(QVariant::Int), QVariant(QVariant::Int));

    // No.1
    isConnect = true;
    // 将状态设置为 通
    ui->state_label->setText("通");
   // QPalette pa;
  //  pa.setColor(QPalette::WindowText,Qt::blue);
    //ui->state_label->setPalette(pa);

    // 将按钮设置为　断开网络
    ui->connect_pushButton->setText("断开网络");

    // 禁用远程端口，本地端口，远程IP
    ui->remoteIP_lineEdit->setEnabled(false);
    ui->remoteport_spinBox->setEnabled(false);
    ui->localport_spinBox->setEnabled(false);
    // 使能button
    ui->handSend_pushButton->setEnabled(true);

    client.udpStart(chelper.getLocalHostIP(), mLocalPort, QHostAddress(mRemoteIp), mRemotePort);
}

void MainWindow::updateReceiveText(const QString string,QHostAddress addr)  //update scren;
{

    ui->label_pic->hide();
    QString oldString = ui->receive_textBrowser->toPlainText();
    QString sss=QString("[%2@ %1: : ]-------------------------------------\n").arg(QTime::currentTime().toString("hh:mm:ss-zzz")).arg(addr.toString());
    ui->receive_textBrowser->setText(oldString + sss+string + "\n");

    // 将光标移动到最后位置
    QTextCursor tmpCursor = ui->receive_textBrowser->textCursor();
    tmpCursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor, 4);
    ui->receive_textBrowser->setTextCursor(tmpCursor);


    QByteArray byte_array=string.toLatin1();
    QJsonParseError json_error;
    QJsonDocument docment=QJsonDocument::fromJson(byte_array,&json_error);
    if(json_error.error==QJsonParseError::NoError){  //是Json数据
        QJsonValue json_value;

        if(docment.isObject()){
            QJsonObject obj=docment.object();
            if(obj.contains("ScreenShot")){
                qDebug()<<"GEt pic";
                this->get_json_pic(string);
               // return;
            }
            }
    }
}

/**
 * @brief MainWindow::updateStateBar
 * @param state 状态
 * @param inNum 接收数值
 * @param outNum 发送数值
 */
void MainWindow::updateStateBar(QString state, QVariant inNum, QVariant outNum)
{
    if(!state.isNull())
        statusLabel->setText(state);

    if(!inNum.isNull()) {
        //累计
        if(inNum.toInt() == 0)
            mReceiveNum = 0;
        else
            mReceiveNum += inNum.toInt();
        receiveLabel->setText("接收:" + QString::number(mReceiveNum));
    }

    if(!outNum.isNull()) {
        //累计
        if(outNum.toInt() == 0)
            mSendNum = 0;
        else
            mSendNum += outNum.toInt();
        sendLabel->setText("发送:" + QString::number(mSendNum));
    }

}

/**
 * 断开UDP时调用该函数
 * @brief MainWindow::init
 */
void MainWindow::init()
{
    qDebug("%s", __func__);
    // No.1
    isConnect = false;
    // 将状态设置为 断
    ui->state_label->setText("断");
 //   QPalette pa;
 //   pa.setColor(QPalette::WindowText,Qt::red);
    //ui->state_label->setPalette(pa);

    // 将按钮设置为　连接网络
    ui->connect_pushButton->setText("连接网络");

    // 使能远程端口，本地端口，远程IP
    ui->remoteIP_lineEdit->setEnabled(true);
    ui->remoteport_spinBox->setEnabled(true);
    ui->localport_spinBox->setEnabled(true);
    // 禁用button
    ui->handSend_pushButton->setEnabled(false);
    //
    client.udpStop(NULL, NULL, NULL);

    updateStateBar("本地IP: " + chelper.getLocalHostIP().toString() + " 无连接",
                   QVariant(QVariant::Int), QVariant(QVariant::Int));
}

/**
 * 断开UDP时调用该函数
 * @brief MainWindow::disConnectNet
 */
void MainWindow::disConnectNet()
{
    qDebug("%s", __func__);
    // No.1
    isConnect = false;
    // 将状态设置为 断
    ui->state_label->setText("断");
  //  QPalette pa;
  //  pa.setColor(QPalette::WindowText,Qt::red);
    //ui->state_label->setPalette(pa);

    // 将按钮设置为　连接网络
    ui->connect_pushButton->setText("连接网络");

    // 使能远程端口，本地端口，远程IP
    ui->remoteIP_lineEdit->setEnabled(true);
    ui->remoteport_spinBox->setEnabled(true);
    ui->localport_spinBox->setEnabled(true);
    // 禁用button
    ui->handSend_pushButton->setEnabled(false);
    //
    client.udpStop(NULL, NULL, NULL);


    updateStateBar(tr("UDP通信停止"), QVariant(QVariant::Int), QVariant(QVariant::Int));
}

void MainWindow::doSettings(bool isWrite)
{
    QSettings settings("Yzs_think", "Application");
    const QString REMOTE_IP = "remoteip";
    const QString REMOTE_PORT = "remoteport";
    const QString LOCAL_PORT = "localport";
    QString UPDATE_URL="updateurl";
    QString UPDATE_MD5="updatemd5";
    if(isWrite) {
        settings.setValue(REMOTE_IP, mRemoteIp);
        settings.setValue(REMOTE_PORT, mRemotePort);
        settings.setValue(LOCAL_PORT, mLocalPort);
        settings.setValue(UPDATE_URL,ui->textEdit_update_url->toPlainText());
        settings.setValue(UPDATE_MD5,ui->textEdit_update_md5->toPlainText());
    } else {
        mRemoteIp = settings.value(REMOTE_IP, chelper.getLocalHostIP().toString()).toString();
        mRemotePort = settings.value(REMOTE_PORT, 9009).toInt();
        mLocalPort = settings.value(LOCAL_PORT, 9009).toInt();
        ui->textEdit_update_url->setText(settings.value(UPDATE_URL, "http://192.168.1.222/R").toString());
         ui->textEdit_update_md5->setText(settings.value(UPDATE_MD5, "md5").toString());
    }
}

MainWindow::~MainWindow()
{
    doSettings(true);
    delete ui;
}

/**
 * @brief MainWindow::on_clearCounter_pushButton_released
 * 该函数是将计数器置零
 */
void MainWindow::on_clearCounter_pushButton_released()
{
    qDebug("%s", __func__);
    updateStateBar(QString(), 0, 0);
}

void MainWindow::on_clearReceive_pushButton_released()
{
    ui->receive_textBrowser->clear();
}

void MainWindow::on_clearSend_pushButton_released()
{
    ui->send_plainTextEdit->clear();
}

/**
 * @brief MainWindow::on_connect_pushButton_released
 * 该函数是在连接/断开网络时调用
 */
void MainWindow::on_connect_pushButton_released()
{
    qDebug("%s", __func__);
    // 如果当前网络是连接状态　调用断开连接函数
    if(isConnect) {
        disConnectNet();

    } else { // 否则调用连接函数
        connectNet();

    }
}

/**
 * 当用户点击 发送 时调用该函数
 * @brief MainWindow::on_handSend_pushButton_released
 */
void MainWindow::on_handSend_pushButton_released()
{
    // 获取 rmeote ip/ port 和内容
    QString string = ui->send_plainTextEdit->toPlainText();
    if(string.length() != 0) {
        client.sendData(string, mRemoteIp, mRemotePort);
    }
}

void MainWindow::on_quit_pushButton_released()
{
    QApplication::quit();
}

void MainWindow::on_pushButton_released()
{
    QString string=QString("{ \"Command\":\"GiveMeAllData\"}");
    ui->send_plainTextEdit->setPlainText(string);
    client.sendData(string, mRemoteIp, mRemotePort);

}

void MainWindow::on_comboBox_2_currentTextChanged(const QString &arg1)  //声音大小
{
    QJsonObject obj;
    obj.insert("VideoVolume",QJsonValue(arg1.toFloat()));


    QJsonDocument json_s;
    json_s.setObject(obj);
    qDebug()<<json_s.toJson();
    if(!json_s.isNull()){
        QByteArray datagram=json_s.toJson();

        ui->send_plainTextEdit->setPlainText(QString(json_s.toJson().data()));
        client.sendData(QString(json_s.toJson().data()), mRemoteIp, mRemotePort);
    }

}

void MainWindow::on_comboBox_currentTextChanged(const QString &arg1)
{
    QJsonObject obj;
    obj.insert("LcdPwm",QJsonValue(arg1.toInt()));


    QJsonDocument json_s;
    json_s.setObject(obj);
    qDebug()<<json_s.toJson();
    if(!json_s.isNull()){
        QByteArray datagram=json_s.toJson();

        ui->send_plainTextEdit->setPlainText(QString(json_s.toJson().data()));
        client.sendData(QString(json_s.toJson().data()), mRemoteIp, mRemotePort);
    }
}

void MainWindow::on_comboBox_3_currentTextChanged(const QString &arg1)
{
    int ro=0;
    if(arg1==QString("Screen0")){
        ro=0;
    }
    if(arg1==QString("Screen90")){
        ro=90;
    }
    if(arg1==QString("Screen180")){
        ro=180;
    }
    if(arg1==QString("Screen270")){
        ro=270;
    }
    QJsonObject obj;
    obj.insert("ScreenRotation",QJsonValue(ro));
    QJsonDocument json_s;
    json_s.setObject(obj);
    qDebug()<<json_s.toJson();
    if(!json_s.isNull()){
        QByteArray datagram=json_s.toJson();

        ui->send_plainTextEdit->setPlainText(QString(json_s.toJson().data()));
        client.sendData(QString(json_s.toJson().data()), mRemoteIp, mRemotePort);
    }
}

void MainWindow::on_pushButton_3_released()
{
    QString time=QDateTime::currentDateTime().toString("yyyy-MM-dd*hh:mm:ss");
    QJsonObject obj;
    obj.insert("Time",QJsonValue(time));
    QJsonDocument json_s;
    json_s.setObject(obj);
    qDebug()<<json_s.toJson();
    if(!json_s.isNull()){
        QByteArray datagram=json_s.toJson();

        ui->send_plainTextEdit->setPlainText(QString(json_s.toJson().data()));
        client.sendData(QString(json_s.toJson().data()), mRemoteIp, mRemotePort);
    }
}

void MainWindow::on_pushButton_2_released()
{
      QJsonObject obj;
    bool reboot=true;
    obj.insert("Reboot",QJsonValue(reboot));
    QJsonDocument json_s;
    json_s.setObject(obj);
    qDebug()<<json_s.toJson();
    if(!json_s.isNull()){
        QByteArray datagram=json_s.toJson();

        ui->send_plainTextEdit->setPlainText(QString(json_s.toJson().data()));
        client.sendData(QString(json_s.toJson().data()), mRemoteIp, mRemotePort);
    }
}

void MainWindow::on_pushButton_setip_released()
{
    QJsonObject root_object;
    QJsonObject obj;

    obj.insert("ip",QJsonValue(ui->lineEdit_ip->text()));
    obj.insert("netmask",QJsonValue(ui->lineEdit_netmask->text()));
    obj.insert("gw",QJsonValue(ui->lineEdit_gw->text()));
    root_object.insert("NetSet",QJsonValue(obj));
    QJsonDocument json_s;
    json_s.setObject(root_object);
    if(!json_s.isNull()){
        QByteArray datagram=json_s.toJson();

        ui->send_plainTextEdit->setPlainText(QString(json_s.toJson().data()));
        client.sendData(QString(json_s.toJson().data()), mRemoteIp, mRemotePort);
    }
}

void MainWindow::on_pushButton_4_released() //0.x
{
    ui->lineEdit_ip->setText("192.168.0.33");
    ui->lineEdit_netmask->setText("255.255.255.0");
    ui->lineEdit_gw->setText("192.168.0.1");
}

void MainWindow::on_pushButton_5_released() //1.x
{
    ui->lineEdit_ip->setText("192.168.1.33");
    ui->lineEdit_netmask->setText("255.255.255.0");
    ui->lineEdit_gw->setText("192.168.1.1");
}

void MainWindow::on_pushButton_6_released()
{
    QJsonObject obj;
  QString s="ScreenShot";
  obj.insert("Command",QJsonValue(s));
  QJsonDocument json_s;
  json_s.setObject(obj);
  qDebug()<<json_s.toJson();
  if(!json_s.isNull()){
      QByteArray datagram=json_s.toJson();

      ui->send_plainTextEdit->setPlainText(QString(json_s.toJson().data()));
      client.sendData(QString(json_s.toJson().data()), mRemoteIp, mRemotePort);
  }
}

void MainWindow::get_json_pic(QString s)
{

    QByteArray byte_array=s.toLatin1();
    QJsonParseError json_error;
    QJsonDocument docment=QJsonDocument::fromJson(byte_array,&json_error);
    if(json_error.error!=QJsonParseError::NoError){
      //  qDebug()<<"JSON ERROR";
        //this->back_command_msg(addr,QString("not a json value"));
        return;
    }
    qDebug()<<"Get json";
    QJsonValue json_value;
    QString s1;


    if(docment.isObject()){
        QJsonObject obj=docment.object();
        if(obj.contains("ScreenShot")){
            json_value=obj.take("ScreenShot");
            if(json_value.isObject()){//
                QJsonValue value2;
                QJsonObject obj2=json_value.toObject();
                if(obj2.contains("PicData")){  qDebug()<<"ssss";
                    value2=obj2.take("PicData");

                    if(value2.isString()){
                        s1=value2.toString();
                       qDebug()<<"S1"<<s1;
                    }
                }
            }
        }
    }
    QByteArray aa=s1.toLatin1();
    QByteArray bb=aa.fromBase64(aa);
    QImage image;
    image.loadFromData(bb);
    image.save("1.jpg");
    QPixmap pm;
    pm = pm.fromImage(image).scaled(480,320);

    ui->label_pic->setPixmap(pm);
    ui->label_pic->show();

    ui->label_pic->setStyleSheet("background-color: rgba(255, 255, 255, 0);");
    QTimer::singleShot(5000,ui->label_pic,SLOT(hide()));

qDebug()<<s1;
 //   QString oldString = ui->receive_textBrowser->toPlainText();
 //   QString sss=QString("[@ %1:]-------------------------------------\n").arg(QTime::currentTime().toString("hh:mm:ss-zzz"));
 //   ui->receive_textBrowser->setText(oldString + sss+s + "\n");

    // 将光标移动到最后位置
//    QTextCursor tmpCursor = ui->receive_textBrowser->textCursor();
 //   tmpCursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor, 4);
 //   ui->receive_textBrowser->setTextCursor(tmpCursor);
}

void MainWindow::on_pushButton_7_released()
{
    QJsonObject obj;
 // QString s="ScreenShot";
  obj.insert("Live",QJsonValue(ui->textEdit_live->toPlainText()));
  QJsonDocument json_s;
  json_s.setObject(obj);
  qDebug()<<json_s.toJson();
  if(!json_s.isNull()){
      QByteArray datagram=json_s.toJson();

      ui->send_plainTextEdit->setPlainText(QString(json_s.toJson().data()));
      client.sendData(QString(json_s.toJson().data()), mRemoteIp, mRemotePort);
  }
}

void MainWindow::on_pushButton_8_released()
{
    QJsonObject obj;
  QString s="Clear";
  obj.insert("PlayCommand",QJsonValue(s));
  QJsonDocument json_s;
  json_s.setObject(obj);
  qDebug()<<json_s.toJson();
  if(!json_s.isNull()){
      QByteArray datagram=json_s.toJson();

      ui->send_plainTextEdit->setPlainText(QString(json_s.toJson().data()));
      client.sendData(QString(json_s.toJson().data()), mRemoteIp, mRemotePort);
  }
}

void MainWindow::on_pushButton_9_released()
{
    QJsonObject obj;
  QString s="Media";
  obj.insert("PlayCommand",QJsonValue(s));
  QJsonDocument json_s;
  json_s.setObject(obj);
  qDebug()<<json_s.toJson();
  if(!json_s.isNull()){
      QByteArray datagram=json_s.toJson();

      ui->send_plainTextEdit->setPlainText(QString(json_s.toJson().data()));
      client.sendData(QString(json_s.toJson().data()), mRemoteIp, mRemotePort);
  }
}

void MainWindow::on_pushButton_10_released() //update soft
{
    QJsonObject root_object;
    QJsonObject obj;

    obj.insert("address",QJsonValue(ui->textEdit_update_url->toPlainText()));
    obj.insert("md5",QJsonValue(ui->textEdit_update_md5->toPlainText()));

    root_object.insert("AppUpdate",QJsonValue(obj));
    QJsonDocument json_s;
    json_s.setObject(root_object);
    if(!json_s.isNull()){
        QByteArray datagram=json_s.toJson();

        ui->send_plainTextEdit->setPlainText(QString(json_s.toJson().data()));
        client.sendData(QString(json_s.toJson().data()), mRemoteIp, mRemotePort);
    }
}

void MainWindow::on_pushButton_11_released()  //电梯协议
{
    QString lift="CAN";
    QString brand="ThyssenKrupp";
    QString model="MC2";
    switch (ui->comboBox_lift->currentIndex()) {
    case 0:
        lift="CAN";
        brand="ThyssenKrupp";
        model="MC2";
        break;
    case 1:
        lift="MC2-H";
        brand="ThyssenKrupp";
        model="MC2-H";
        break;
    case 2:
        lift="SPI";
        brand="KONE";
        model="Unknow";
        break;
    case 3:
        lift="TOSHIBA";
        brand="TOSHIBA";
        model="Unknow";
        break;
    default:
        break;
    }
    QJsonObject root_object;
    QJsonObject obj;

    obj.insert("protocol",QJsonValue(lift));
    obj.insert("brand",QJsonValue(brand));
    obj.insert("model",QJsonValue(model));
     obj.insert("hibcop",QJsonValue(ui->comboBox_toshiba_hib->currentText()));
     obj.insert("can08address",QJsonValue(ui->spinBox_can08->value()));
    root_object.insert("ElevatorProtocol",QJsonValue(obj));
    QJsonDocument json_s;
    json_s.setObject(root_object);
    if(!json_s.isNull()){
        QByteArray datagram=json_s.toJson();

        ui->send_plainTextEdit->setPlainText(QString(json_s.toJson().data()));
        client.sendData(QString(json_s.toJson().data()), mRemoteIp, mRemotePort);
    }
}

void MainWindow::on_pushButton_12_released()
{
     ui->send_plainTextEdit->setPlainText(QString("NetSet$192.168.1.%1#255.255.255.0#192.168.1.1#end").arg(QString::number(ip_now)));
     this->ip_now++;
}

void MainWindow::on_pushButton_13_released()
{
    QJsonObject obj;
  QString s="XXXX";
  obj.insert("Rollmessage",QJsonValue(s));
  QJsonDocument json_s;
  json_s.setObject(obj);
  qDebug()<<json_s.toJson();
  if(!json_s.isNull()){
      QByteArray datagram=json_s.toJson();

      ui->send_plainTextEdit->setPlainText(QString(json_s.toJson().data()));

  }
}

void MainWindow::on_checkBox_udp_broadcast_toggled(bool checked)
{
    if(checked){

        this->mRemoteIp=QString("255.255.255.255");
    }
    else{
        this->mRemoteIp=ui->remoteIP_lineEdit->text();

    }
}

void MainWindow::on_pushButton_ftp_choose_released()
{
    QString file_name="0";

    QFileDialog *fileDialog = new QFileDialog(this);
           fileDialog->setWindowTitle(tr("选择ZIP升级包"));
           fileDialog->setDirectory(".");
           fileDialog->setNameFilter(tr("ZIP (*.zip)"));
           if(fileDialog->exec() == QDialog::Accepted) {
                   file_name = fileDialog->selectedFiles()[0];
                //   QMessageBox::information(NULL, tr("Path"), tr("You selected ") + file_name );

           } else {
                  // QMessageBox::information(NULL, tr("Path"), tr("You didn't select any files."));
           }

        ui->textEdit_ftp->setText(file_name);
           QString sMd5="0";
         // qDebug()<<"File here: "<<newFileInfo.absoluteFilePath();
             QFile file(file_name);
             if(file.open(QIODevice::ReadOnly)){
                 QByteArray bArray = QCryptographicHash::hash(file.readAll(),QCryptographicHash::Md5);
                 sMd5 = QString(bArray.toHex()).toUpper();
            }
            ui->textEdit_md5->setText(sMd5);
           file.close();
}

void MainWindow::on_pushButton_upload_released()
{

    QFile file(ui->textEdit_ftp->toPlainText());
    qDebug()<<file.fileName();
    QFileInfo fileinfo(file.fileName());
    QString s_name=fileinfo.fileName();

    ftp.setHostPort(this->mRemoteIp,21);
    ui->textEdit_ftp_info->setText(QString("Connect to host:%1").arg(this->mRemoteIp));
    ftp.setUserInfo("root","jh=-tech");
    ui->progressBar_ftp->setValue(0);
    ftp.put(ui->textEdit_ftp->toPlainText(),"/home/root/tmp/"+s_name);
    connect(&ftp, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(error(QNetworkReply::NetworkError)));
   connect(&ftp, SIGNAL(uploadProgress(qint64, qint64)), this, SLOT(uploadProgress(qint64, qint64)));
    //ftp.deleteLater();
}

void MainWindow::error(QNetworkReply::NetworkError error)
{
    qDebug()<<error;
    switch (error) {
       case QNetworkReply::HostNotFoundError :
           qDebug() << QString::fromLocal8Bit("主机没有找到");
           break;
           // 其他错误处理
       default:
           break;
       }
    ui->textEdit_ftp_info->setText(ui->textEdit_ftp_info->toPlainText()+"错误,错误号:"+QString::number(error));
    QTextCursor tmpCursor = ui->textEdit_ftp_info->textCursor();
    tmpCursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor, 4);
    ui->textEdit_ftp_info->setTextCursor(tmpCursor);
}
void MainWindow::uploadProgress(qint64 bytesSent, qint64 bytesTotal)
{

    int value=bytesSent*100/bytesTotal;
  qDebug()<<bytesSent<<bytesTotal<<value;
    ui->progressBar_ftp->setValue(value);
    if(value==100){
        ui->textEdit_ftp_info->setText(ui->textEdit_ftp_info->toPlainText()+"上传完毕,可以发送升级命令并重启:");
        QTextCursor tmpCursor = ui->textEdit_ftp_info->textCursor();
        tmpCursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor, 4);
        ui->textEdit_ftp_info->setTextCursor(tmpCursor);

    }
}

void MainWindow::on_pushButton_14_released() //发送解压命令
{
     ui->progressBar_ftp->setValue(0);
    QJsonObject root_object;
    QJsonObject obj;
    QFile file(ui->textEdit_ftp->toPlainText());
    qDebug()<<file.fileName();
    QFileInfo fileinfo(file.fileName());
    QString s_name=fileinfo.fileName();
    obj.insert("zipname",QJsonValue(s_name));
    obj.insert("md5",QJsonValue(ui->textEdit_md5->toPlainText()));
    obj.insert("action",QJsonValue("unzip"));
    obj.insert("isreboot",QJsonValue("false"));

    root_object.insert("AppUnzip",QJsonValue(obj));
    QJsonDocument json_s;
    json_s.setObject(root_object);
    if(!json_s.isNull()){

        ui->send_plainTextEdit->setPlainText(QString(json_s.toJson().data()));
        client.sendData(QString(json_s.toJson().data()), mRemoteIp, mRemotePort);
    }
}

void MainWindow::on_pushButton_15_released()
{
    ui->progressBar_ftp->setValue(0);
    on_pushButton_2_released();
}

void MainWindow::on_pushButton_16_released() //close watch dog open debug mode
{
    QJsonObject obj;
  QString s="CloseWatchDog";
  obj.insert("Command",QJsonValue(s));
  QJsonDocument json_s;
  json_s.setObject(obj);
  qDebug()<<json_s.toJson();
  if(!json_s.isNull()){
      QByteArray datagram=json_s.toJson();

      ui->send_plainTextEdit->setPlainText(QString(json_s.toJson().data()));
      client.sendData(QString(json_s.toJson().data()), mRemoteIp, mRemotePort);
  }
}

void MainWindow::on_pushButton_17_released()//opendog
{
    QJsonObject obj;
  QString s="OpenWatchDog";
  obj.insert("Command",QJsonValue(s));
  QJsonDocument json_s;
  json_s.setObject(obj);
  qDebug()<<json_s.toJson();
  if(!json_s.isNull()){
      QByteArray datagram=json_s.toJson();

      ui->send_plainTextEdit->setPlainText(QString(json_s.toJson().data()));
      client.sendData(QString(json_s.toJson().data()), mRemoteIp, mRemotePort);
  }
}

void MainWindow::on_pushButton_18_released()
{
    QJsonObject obj;
  QString s="GiveMeElevatorStatus";
  obj.insert("JsonType",QJsonValue(s));
  QJsonDocument json_s;
  json_s.setObject(obj);
  qDebug()<<json_s.toJson();
  if(!json_s.isNull()){
      QByteArray datagram=json_s.toJson();

      ui->send_plainTextEdit->setPlainText(QString(json_s.toJson().data()));
      client.sendData(QString(json_s.toJson().data()), mRemoteIp, mRemotePort);
  }
}

void MainWindow::on_pushButton_19_released()
{
    QJsonObject root_object;
    QJsonObject obj;

    obj.insert("ssid",QJsonValue(ui->lineEdit_ssid->text()));
    obj.insert("passwd",QJsonValue(ui->lineEdit_ssid_passwd->text()));

    root_object.insert("AP",QJsonValue(obj));
    QJsonDocument json_s;
    json_s.setObject(root_object);
    if(!json_s.isNull()){

        ui->send_plainTextEdit->setPlainText(QString(json_s.toJson().data()));
        client.sendData(QString(json_s.toJson().data()), mRemoteIp, mRemotePort);
    }
}

void MainWindow::on_autoSend_checkBox_clicked(bool checked)
{
    if(checked==true){
        this->timer_send->start(ui->autoSendTime_lineEdit->text().toInt());
    }
    else{
        this->timer_send->stop();
    }
}

void MainWindow::on_comboBox_fire_volume_currentIndexChanged(const QString &arg1)
{

}

void MainWindow::on_comboBox_fire_volume_currentTextChanged(const QString &arg1)
{
    QJsonObject obj;
    obj.insert("FireVolume",QJsonValue(arg1.toFloat()));


    QJsonDocument json_s;
    json_s.setObject(obj);
    qDebug()<<json_s.toJson();
    if(!json_s.isNull()){
        QByteArray datagram=json_s.toJson();

        ui->send_plainTextEdit->setPlainText(QString(json_s.toJson().data()));
        client.sendData(QString(json_s.toJson().data()), mRemoteIp, mRemotePort);
    }
}
