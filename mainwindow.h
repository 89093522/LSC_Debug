#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QUdpSocket>
#include <QLabel>
#include <QPushButton>
#include "udpclient.h"
#include "commonhelper.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QCryptographicHash>
#include <FtpManager.h>
#include <QTimer>
namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void get_json_pic(QString s);

private slots:
    void on_clearReceive_pushButton_released();
    void on_clearSend_pushButton_released();
    void on_connect_pushButton_released();
    void on_handSend_pushButton_released();
    void updateReceiveText(const QString string,QHostAddress addr);
    void on_clearCounter_pushButton_released();
    void on_quit_pushButton_released();
    void updateStateBar(QString state, QVariant inNum, QVariant outNum);

    void on_pushButton_released();

    void on_comboBox_2_currentTextChanged(const QString &arg1);

    void on_comboBox_currentTextChanged(const QString &arg1);

    void on_comboBox_3_currentTextChanged(const QString &arg1);

    void on_pushButton_3_released();

    void on_pushButton_2_released();

    void on_pushButton_setip_released();

    void on_pushButton_4_released();

    void on_pushButton_5_released();

    void on_pushButton_6_released();

    void on_pushButton_7_released();

    void on_pushButton_8_released();

    void on_pushButton_9_released();

    void on_pushButton_10_released();

    void on_pushButton_11_released();

    void on_pushButton_12_released();

    void on_pushButton_13_released();

    void on_checkBox_udp_broadcast_toggled(bool checked);

    void on_pushButton_ftp_choose_released();

    void on_pushButton_upload_released();

    void uploadProgress(qint64 bytesSent, qint64 bytesTotal);
    void error(QNetworkReply::NetworkError error);

    void on_pushButton_14_released();

    void on_pushButton_15_released();

    void on_pushButton_16_released();

    void on_pushButton_17_released();

    void on_pushButton_18_released();

    void on_pushButton_19_released();

    void on_autoSend_checkBox_clicked(bool checked);

    void on_comboBox_fire_volume_currentIndexChanged(const QString &arg1);

    void on_comboBox_fire_volume_currentTextChanged(const QString &arg1);

    void on_spinBox_rollmsg_speed_valueChanged(int arg1);

private:
    Ui::MainWindow *ui;
    QUdpSocket *udpSocket;
    void connectNet();
    void disConnectNet();
    bool isConnect;
    UDPClient client;
    /** 工具类 */
    CommonHelper chelper;
    /** 状态标签 */
    QLabel *statusLabel;
    /** 总接收数量显示标签 */
    QLabel *receiveLabel;
    /** 总发送数量显示标签 */
    QLabel *sendLabel;
    /** 状态栏 计数清零 按钮 */
    QPushButton *clearCounterButton;
    /** 状态栏 时间标签 */
    QLabel *timeLabel;
    /** 接收总数 */
    quint64 mReceiveNum;
    /** 发送总数 */
    quint64 mSendNum;
    /** 远程IP */
    QString mRemoteIp;
    /** 远程端口 */
    int mRemotePort;
    /** 本地端口 */
    int mLocalPort;

    void doSettings(bool isWrite);
    void init();
    int ip_now;
    QTimer *timer_send;

    FtpManager ftp;
};

#endif // MAINWINDOW_H
