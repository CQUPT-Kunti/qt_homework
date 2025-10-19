// mainwindow.cpp
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QListWidgetItem>
#include <QMessageBox>



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , socket(new QTcpSocket(this))
{
    ui->setupUi(this);
    ui->sendButton->setEnabled(false);

    connect(ui->connectButton, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(ui->sendButton,  &QPushButton::clicked, this, &MainWindow::onSendClicked);
    connect(socket,          &QTcpSocket::connected,     this, &MainWindow::onSocketConnected);
    connect(socket,          &QTcpSocket::readyRead,     this, &MainWindow::onSocketReadyRead);
    connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
            this, &MainWindow::onSocketErrorOccurred);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::onConnectClicked() {
    socket->connectToHost(ui->hostEdit->text(), ui->portEdit->text().toUShort());
}

void MainWindow::onSocketConnected() {
    QListWidgetItem *item = new QListWidgetItem(QStringLiteral("连接成功"));
    item->setForeground(Qt::green);
    item->setTextAlignment(Qt::AlignCenter);
    ui->chatList->addItem(item);
    ui->sendButton->setEnabled(true);
}

void MainWindow::onSocketErrorOccurred(QAbstractSocket::SocketError) {
    QListWidgetItem *item = new QListWidgetItem(QStringLiteral("连接失败"));
    item->setForeground(Qt::red);
    item->setTextAlignment(Qt::AlignCenter);
    ui->chatList->addItem(item);
}

void MainWindow::onSendClicked()
{
    const QString msg = ui->messageEdit->text();
    if (msg.isEmpty()) return;

    // 自己的消息，右对齐
    QListWidgetItem* item = new QListWidgetItem(msg);
    item->setTextAlignment(Qt::AlignRight);
    ui->chatList->addItem(item);

    socket->write(msg.toUtf8());
    ui->messageEdit->clear();
}

void MainWindow::onSocketReadyRead()
{
    const QString msg = QString::fromUtf8(socket->readAll());

    // 服务器的消息，左对齐
    QListWidgetItem* item = new QListWidgetItem(msg);
    item->setTextAlignment(Qt::AlignLeft);
    ui->chatList->addItem(item);
}
