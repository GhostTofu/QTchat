#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "mainwidget.h"
#include "ui_mainwidget.h"
#include "databasemanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QInputDialog>
#include <QMessageBox>
#include <QFileDialog>
#include <QScrollBar>
#include <QPixmap>
#include <QBuffer>
#include <QIODevice>
#include <QDebug>

static QString avatarFileToDataUrl(const QString &filePath)
{
    QPixmap pix(filePath);
    if (pix.isNull()) {
        return QString();
    }

    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    pix.scaled(96, 96, Qt::KeepAspectRatio, Qt::SmoothTransformation)
        .save(&buffer, "PNG");
    return "data:image/png;base64," + QString::fromLatin1(bytes.toBase64());
}

MainWidget::MainWidget(const UserInfo &userInfo, QWidget *parent)
    : QWidget(parent), ui(new Ui::MainWidget), m_myInfo(userInfo)
{
    // 初始化本地数据库
    DatabaseManager::instance()->initDatabase(m_myInfo.account);

    ui->setupUi(this);
    connectSignals();

    setWindowTitle(QString("即时通讯 - %1 [%2]").arg(userInfo.nickname, userInfo.account));
    resize(750, 550);
}

MainWidget::~MainWidget()
{
    // 关闭所有聊天窗口
    for (ChatDialog *dlg : m_chatDialogs.values()) {
        dlg->close();
        dlg->deleteLater();
    }
    delete ui;
}

void MainWidget::connectSignals()
{
    NetworkClient *net = NetworkClient::instance();

    // 工具栏按钮
    connect(ui->changeAvatarBtn, &QPushButton::clicked, this, &MainWidget::onChangeAvatarClicked);
    connect(ui->addFriendBtn, &QPushButton::clicked, this, &MainWidget::onAddFriendClicked);

    // 好友列表双击（ui->friendList 已提升为 FriendListWidget）
    connect(ui->friendList, &FriendListWidget::friendDoubleClicked,
            this, &MainWidget::onFriendDoubleClicked);

    // 收到新消息
    connect(net, &NetworkClient::newMessage, this, &MainWidget::onNewMessage);
    // 好友状态变化
    connect(net, &NetworkClient::statusChanged, this, &MainWidget::onStatusChanged);
    // 好友头像同步
    connect(net, &NetworkClient::avatarSynced, this, &MainWidget::onAvatarSynced);
    // 被添加为好友
    connect(net, &NetworkClient::friendAdded, this, &MainWidget::onFriendAdded);
    // 添加好友结果
    connect(net, &NetworkClient::addFriendResult, this, &MainWidget::onAddFriendResult);
    // 好友列表（登录后首次）
    connect(net, &NetworkClient::friendListReceived, this, &MainWidget::onFriendListReceived);
    // 消息回执
    connect(net, &NetworkClient::messageAck, this, &MainWidget::onMessageAck);

    // 更新用户信息标签
    ui->userInfoLabel->setText(
        QString(" 👤 %1 | 🆔 %2 | %3 ")
            .arg(m_myInfo.nickname, m_myInfo.account, m_myInfo.gender));
}

// ==================== 好友双击 ====================

void MainWidget::onFriendDoubleClicked(const QString &account, const QString &nickname)
{
    openChatDialog(account, nickname);
}

ChatDialog* MainWidget::openChatDialog(const QString &account, const QString &nickname)
{
    ChatDialog *dlg = m_chatDialogs.value(account, nullptr);
    if (!dlg) {
        dlg = new ChatDialog(m_myInfo.account, account, nickname, this);
        m_chatDialogs[account] = dlg;

        // 连接发送消息信号
        connect(dlg, &ChatDialog::sendMessage, this,
                [](const QString &to, const QString &content, const QString &type) {
                    NetworkClient::instance()->sendMessage(to, content, type);
                });

        // 窗口关闭时从map中移除
        connect(dlg, &QDialog::finished, this, [this, account]() {
            m_chatDialogs.remove(account);
        });

        dlg->show();
    } else {
        dlg->show();
        dlg->raise();
        dlg->activateWindow();
    }
    return dlg;
}

ChatDialog* MainWidget::findChatDialog(const QString &account)
{
    return m_chatDialogs.value(account, nullptr);
}

// ==================== 收到新消息 ====================

void MainWidget::onNewMessage(const QString &fromAccount, const QString &nickname,
                               const QString &avatar, const QString &content,
                               const QString &msgType, const QString &timestamp)
{
    // 保存到本地数据库
    DatabaseManager::instance()->saveMessage(fromAccount, m_myInfo.account,
                                              content, msgType, timestamp);
    ui->friendList->updateFriendAvatar(fromAccount, avatar);

    // 更新通知栏
    QString notify = QString("[%1] %2: %3").arg(timestamp.mid(11), nickname, content.left(30));
    ui->notificationBar->insertItem(0, notify);
    while (ui->notificationBar->count() > 20) {
        delete ui->notificationBar->takeItem(ui->notificationBar->count() - 1);
    }

    // 自动弹出/更新聊天窗口
    ChatDialog *dlg = findChatDialog(fromAccount);
    if (!dlg) {
        dlg = openChatDialog(fromAccount, nickname);
    } else {
        dlg->show();
        dlg->raise();
        dlg->activateWindow();
    }

    dlg->onRecvMessage(content, timestamp);

    // 在中间区域也显示通知
    ui->welcomeView->append(QString(
        "<div style='margin:4px; padding:6px; background:#fff3e0; border-radius:4px;'>"
        "<b>📩 %1</b> <span style='color:#888;'>%2</span><br>%3</div>")
        .arg(nickname, timestamp, content.toHtmlEscaped()));
    QScrollBar *sb = ui->welcomeView->verticalScrollBar();
    sb->setValue(sb->maximum());
}

// ==================== 好友状态变化 ====================

void MainWidget::onStatusChanged(const QString &account, bool online)
{
    ui->friendList->updateFriendStatus(account, online);

    QListWidgetItem *item = new QListWidgetItem(
        QString("%1 %2 已%3").arg(online ? "🟢" : "⚫", account, online ? "上线" : "下线"));
    item->setForeground(online ? QColor("#4caf50") : QColor("#999"));
    ui->notificationBar->insertItem(0, item);
}

// ==================== 头像同步 ====================

void MainWidget::onAvatarSynced(const QString &account, const QString &avatar)
{
    ui->friendList->updateFriendAvatar(account, avatar);
}

// ==================== 好友添加 ====================

void MainWidget::onFriendAdded(const QVariantMap &friendInfo)
{
    ui->friendList->addOrUpdateFriend(friendInfo);
    QMessageBox::information(this, "新好友",
                             QString("%1 已将你添加为好友！")
                                 .arg(friendInfo["nickname"].toString()));
}

void MainWidget::onAddFriendClicked()
{
    bool ok;
    QString account = QInputDialog::getText(this, "添加好友",
                                             "请输入好友账号:", QLineEdit::Normal, "", &ok);
    if (ok && !account.trimmed().isEmpty()) {
        QStringList groups = {"家人", "朋友", "同事", "其他"};
        bool groupOk;
        QString group = QInputDialog::getItem(this, "选择分组",
                                               "请选择好友分组:", groups, 1, false, &groupOk);
        if (groupOk) {
            NetworkClient::instance()->sendAddFriend(account.trimmed(), group);
        }
    }
}

void MainWidget::onAddFriendResult(bool success, const QString &reason,
                                    const QVariantMap &friendInfo)
{
    if (success) {
        ui->friendList->addOrUpdateFriend(friendInfo);
        QMessageBox::information(this, "成功",
                                 QString("已添加好友: %1")
                                     .arg(friendInfo["nickname"].toString()));
    } else {
        QMessageBox::warning(this, "添加失败", reason);
    }
}

void MainWidget::onFriendListReceived(const QVariantList &friends)
{
    ui->friendList->loadFriends(friends);
}

// ==================== 换头像 ====================

void MainWidget::onChangeAvatarClicked()
{
    QString file = QFileDialog::getOpenFileName(this, "选择新头像",
                                                 "", "图片文件 (*.png *.jpg *.bmp)");
    if (!file.isEmpty()) {
        QString avatarData = avatarFileToDataUrl(file);
        if (avatarData.isEmpty()) {
            QMessageBox::warning(this, "头像", "头像加载失败，请重新选择。");
            return;
        }

        m_myInfo.avatar = avatarData;
        NetworkClient::instance()->sendAvatarUpdate(m_myInfo.account, avatarData);
        QMessageBox::information(this, "头像更新", "头像已更新，好友将同步显示。");
    }
}

void MainWidget::onMessageAck(const QString &toAccount, const QString &status)
{
    if (status == "offline") {
        QListWidgetItem *item = new QListWidgetItem(
            QString("⚠ 消息未送达 %1 (对方不在线)").arg(toAccount));
        item->setForeground(QColor("#ff9800"));
        ui->notificationBar->insertItem(0, item);
    }
}
