#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "servermainwindow.h"
#include "ui_servermainwindow.h"
#include "chatserver.h"
#include <QDateTime>
#include <QTreeWidgetItem>

ServerMainWindow::ServerMainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::ServerMainWindow),
      m_server(new ChatServer(this))
{
    ui->setupUi(this);
    connectSignals();
    updateServerState(false);

    appendLog("===== 聊天服务器管理界面 =====", "#4fc3f7");
    appendLog(QString::fromUtf8("服务器尚未启动，请点击 [启动服务器] 按钮"), "#888");
}

ServerMainWindow::~ServerMainWindow()
{
    m_server->stopServer();
    delete ui;
}

void ServerMainWindow::connectSignals()
{
    // 按钮
    connect(ui->startBtn, &QPushButton::clicked, this, &ServerMainWindow::onStartClicked);
    connect(ui->stopBtn, &QPushButton::clicked, this, &ServerMainWindow::onStopClicked);

    // ChatServer 信号
    connect(m_server, &ChatServer::serverStarted, this, [this](int port) {
        Q_UNUSED(port);
        updateServerState(true);
        appendLog("服务器启动成功，监听端口: " + QString::number(port), "#4caf50");
    });
    connect(m_server, &ChatServer::serverStopped, this, [this]() {
        updateServerState(false);
        ui->clientTree->clear();
        appendLog("服务器已停止", "#f44336");
    });
    connect(m_server, &ChatServer::logMessage, this, &ServerMainWindow::onLogMessage);
    connect(m_server, &ChatServer::clientConnected, this, &ServerMainWindow::onClientConnected);
    connect(m_server, &ChatServer::clientLoggedIn, this, &ServerMainWindow::onClientLoggedIn);
    connect(m_server, &ChatServer::clientDisconnected, this, &ServerMainWindow::onClientDisconnected);
    connect(m_server, &ChatServer::messageRelayed, this, [this](const QString &from,
           const QString &to, const QString &content, const QString &msgType) {
        QString preview = content.left(30);
        if (content.length() > 30) preview += "...";
        appendLog(QString("消息 [%1] %2 → %3: %4").arg(msgType, from, to, preview), "#81c784");
    });
    connect(m_server, &ChatServer::errorOccurred, this, &ServerMainWindow::onErrorOccurred);
}

void ServerMainWindow::updateServerState(bool running)
{
    ui->startBtn->setEnabled(!running);
    ui->stopBtn->setEnabled(running);
    ui->portSpinBox->setEnabled(!running);

    if (running) {
        ui->statusTag->setText("● 运行中");
        ui->statusTag->setStyleSheet("color:#4caf50; font-size:14px; font-weight:bold;");
    } else {
        ui->statusTag->setText("● 已停止");
        ui->statusTag->setStyleSheet("color:#f44336; font-size:14px; font-weight:bold;");
        ui->countLabel->setText("在线: 0");
    }
}

void ServerMainWindow::onStartClicked()
{
    int port = ui->portSpinBox->value();
    appendLog("正在启动服务器...", "#ff9800");
    m_server->startServer(port);
}

void ServerMainWindow::onStopClicked()
{
    appendLog("正在停止服务器...", "#ff9800");
    m_server->stopServer();
}

void ServerMainWindow::onLogMessage(const QString &message)
{
    appendLog(message, "#d4d4d4");
}

void ServerMainWindow::onClientConnected(const QString &address, int port)
{
    QString key = address + ":" + QString::number(port);
    appendLog("新客户端连接: " + key, "#29b6f6");

    QTreeWidgetItem *item = new QTreeWidgetItem(ui->clientTree);
    item->setText(0, QString("[待认证] %1:%2").arg(address).arg(port));
    item->setData(0, Qt::UserRole, key);
    item->setData(0, Qt::UserRole + 1, "");  // account, 未登录
    item->setIcon(0, style()->standardIcon(QStyle::SP_ComputerIcon));
    ui->clientTree->addTopLevelItem(item);
}

void ServerMainWindow::onClientLoggedIn(const QString &account, const QString &nickname)
{
    appendLog("用户登录成功: " + account + " (" + nickname + ")", "#66bb6a");

    // 更新 clientTree 中对应项
    for (int i = 0; i < ui->clientTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = ui->clientTree->topLevelItem(i);
        if (item->data(0, Qt::UserRole + 1).toString().isEmpty()) {
            // 找到第一个未认证的项，更新它
            item->setText(0, QString("%1 [%2]").arg(nickname, account));
            item->setData(0, Qt::UserRole + 1, account);
            item->setIcon(0, style()->standardIcon(QStyle::SP_ArrowRight));
            break;
        }
    }

    ui->countLabel->setText(QString("在线: %1").arg(m_server->clientCount()));
}

void ServerMainWindow::onClientDisconnected(const QString &account)
{
    appendLog("客户端断开: " + account, "#ef5350");

    // 从 clientTree 中移除
    for (int i = 0; i < ui->clientTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = ui->clientTree->topLevelItem(i);
        if (item->data(0, Qt::UserRole + 1).toString() == account) {
            delete ui->clientTree->takeTopLevelItem(i);
            break;
        }
    }

    ui->countLabel->setText(QString("在线: %1").arg(m_server->clientCount()));
}

void ServerMainWindow::onErrorOccurred(const QString &error)
{
    appendLog("错误: " + error, "#ef5350");
}

QTreeWidgetItem* ServerMainWindow::findClientItem(const QString &address, int port)
{
    QString key = address + ":" + QString::number(port);
    for (int i = 0; i < ui->clientTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = ui->clientTree->topLevelItem(i);
        if (item->data(0, Qt::UserRole).toString() == key) {
            return item;
        }
    }
    return nullptr;
}

void ServerMainWindow::appendLog(const QString &msg, const QString &color)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString html = QString("<span style='color:#888;'>[%1]</span> "
                           "<span style='color:%2;'>%3</span>")
                       .arg(timestamp, color, msg.toHtmlEscaped());
    ui->logView->append(html);
    // 自动滚动到底部
    QTextCursor c = ui->logView->textCursor();
    c.movePosition(QTextCursor::End);
    ui->logView->setTextCursor(c);
}
