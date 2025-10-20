// mainwindow.cpp
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QScreen>
#include <QDateTime>
#include <QGuiApplication>
#include <QBuffer>
#include <QPixmap>
#include <QListWidgetItem>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , socket(new QTcpSocket(this))
    , userId(12345)
    , msgIdCounter(1)
{
    ui->setupUi(this);
    ui->sendButton->setEnabled(false);

    connect(ui->connectButton, &QPushButton::clicked,
            this, &MainWindow::onConnectClicked);
    connect(ui->sendButton, &QPushButton::clicked,
            this, &MainWindow::onSendClicked);
    connect(socket, &QTcpSocket::connected,
            this, &MainWindow::onSocketConnected);
    connect(socket, &QTcpSocket::readyRead,
            this, &MainWindow::onSocketReadyRead);
    connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
            this, &MainWindow::onSocketErrorOccurred);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::onConnectClicked() {
    socket->connectToHost(ui->hostEdit->text(),
                          ui->portEdit->text().toUShort());
}

void MainWindow::onSocketConnected() {
    ui->sendButton->setEnabled(true);
    auto *item = new QListWidgetItem(QStringLiteral("连接成功"));
    item->setForeground(Qt::green);
    ui->chatList->addItem(item);
}

void MainWindow::onSendClicked() {
    int type = ui->sendTypeComboBox->currentIndex();
    QByteArray payload;
    QString display;

    if (type == TEXT) {
        QString msg = ui->messageEdit->text();
        if (msg.isEmpty()) return;
        payload = msg.toUtf8();
        display = msg;
    } else if (type == IMAGE) {
        QString fileName = QFileDialog::getOpenFileName(
            this, tr("选择图片"), QString(), "Images (*.png *.jpg *.bmp)");
        if (fileName.isEmpty()) return;
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly)) return;
        payload = file.readAll();
        display = fileName;
    } else if (type == MSGFILE) {
        QString fileName = QFileDialog::getOpenFileName(
            this, tr("选择文件"), QString(), "All files (*)");
        if (fileName.isEmpty()) return;
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly)) return;
        payload = file.readAll();
        display = fileName;
    } else if (type == SCREEN) {
        payload = grabScreen();
        display = QStringLiteral("屏幕截图已发送");
    }

    sendMessage(type, payload);

    auto *item = new QListWidgetItem(display);
    item->setTextAlignment(Qt::AlignRight);
    ui->chatList->addItem(item);
    if (type == TEXT) ui->messageEdit->clear();
}

void MainWindow::sendMessage(int type, const QByteArray &data) {
    if (data.isEmpty()) return;
    quint64 msgId = QDateTime::currentMSecsSinceEpoch() + msgIdCounter++;
    auto packets = makeFragments(userId, msgId, type, data);
    for (const auto &packet : packets) {
        socket->write(packet);
    }
}

QByteArray MainWindow::grabScreen() {
    QScreen *screen = QGuiApplication::primaryScreen();
    QPixmap pixmap = screen->grabWindow(0);
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    pixmap.save(&buffer, "PNG");
    return bytes;
}

void MainWindow::onSocketReadyRead() {
    // 累积 TCP 数据
    recvBuffer.append(socket->readAll());

    // 持续尝试解析完整分片
    while (recvBuffer.size() >= int(sizeof(ProtocolHeader))) {
        ProtocolHeader header;
        memcpy(&header, recvBuffer.constData(), sizeof(header));
        int packetSize = sizeof(header) + header.payloadLen;
        if (recvBuffer.size() < packetSize) break;

        QByteArray payload = recvBuffer.mid(sizeof(header), header.payloadLen);
        recvBuffer.remove(0, packetSize);

        auto &msg = receivedMessages[header.msgId];
        msg.type = header.type;
        if (msg.parts.isEmpty()) {
            msg.totalSeq = header.totalSeq;
            msg.parts.resize(header.totalSeq);
        }
        msg.parts[header.seq] = payload;

        // 检查是否接收完所有分片
        bool complete = true;
        for (const auto &part : msg.parts) {
            if (part.isEmpty()) { complete = false; break; }
        }
        if (complete) {
            QByteArray fullData;
            for (const auto &part : msg.parts) fullData.append(part);

            // 根据消息类型显示
            if (msg.type == TEXT) {
                QString text = QString::fromUtf8(fullData);
                auto *item = new QListWidgetItem(text);
                item->setTextAlignment(Qt::AlignLeft);
                ui->chatList->addItem(item);
            } else if (msg.type == IMAGE) {
                QPixmap pix;
                pix.loadFromData(fullData);
                auto *item = new QListWidgetItem;
                item->setIcon(QIcon(pix));
                ui->chatList->addItem(item);
            }

            receivedMessages.remove(header.msgId);
        }
    }
}

void MainWindow::onSocketErrorOccurred(QAbstractSocket::SocketError) {
    auto *item = new QListWidgetItem(QStringLiteral("连接异常"));
    item->setForeground(Qt::red);
    ui->chatList->addItem(item);
}
