// mainwindow.h
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QVector>
#include <QMap>
#include "protocol.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onConnectClicked();
    void onSendClicked();
    void onSocketConnected();
    void onSocketReadyRead();
    void onSocketErrorOccurred(QAbstractSocket::SocketError);

private:
    Ui::MainWindow *ui;
    QTcpSocket *socket;
    quint32 userId;
    quint64 msgIdCounter;

    void sendMessage(int type, const QByteArray &data);
    QByteArray grabScreen();

    // 新增：接收缓冲与消息分片缓存结构
    QByteArray recvBuffer;
    struct ReceivedMessage {
        quint32 type;
        quint32 totalSeq;
        QVector<QByteArray> parts;
    };
    QMap<quint64, ReceivedMessage> receivedMessages;
};

#endif // MAINWINDOW_H
