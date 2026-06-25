#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "loginwidget.h"
#include "ui_loginwidget.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
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

LoginWidget::LoginWidget(QWidget *parent)
    : QWidget(parent), ui(new Ui::LoginWidget),
      m_currentAvatar("default"), m_failCount(0), m_locked(false),
      m_registerMode(false), m_pendingAction(ActionNone)
{
    ui->setupUi(this);

    // 初始状态：解锁按钮隐藏
    ui->unlockBtn->setVisible(false);

    // 性别按钮分组
    m_genderGroup = new QButtonGroup(this);
    m_genderGroup->addButton(ui->maleRadio);
    m_genderGroup->addButton(ui->femaleRadio);

    // 连接信号
    connect(ui->loginBtn, &QPushButton::clicked, this, &LoginWidget::onLoginClicked);
    connect(ui->registerBtn, &QPushButton::clicked, this, &LoginWidget::onRegisterClicked);
    connect(ui->togglePwdBtn, &QPushButton::toggled, this, &LoginWidget::onTogglePassword);
    connect(ui->avatarBtn, &QPushButton::clicked, this, &LoginWidget::onChooseAvatar);
    connect(ui->unlockBtn, &QPushButton::clicked, this, &LoginWidget::onUnlockClicked);

    // 连接NetworkClient信号
    NetworkClient *net = NetworkClient::instance();
    connect(net, &NetworkClient::connected, this, &LoginWidget::onConnected);
    connect(net, &NetworkClient::loginResult, this, &LoginWidget::onLoginResult);
    connect(net, &NetworkClient::registerResult, this, &LoginWidget::onRegisterResult);

    updateMode(false);
}

LoginWidget::~LoginWidget()
{
    delete ui;
}

void LoginWidget::setInputsEnabled(bool enabled)
{
    ui->accountEdit->setEnabled(enabled);
    ui->nicknameEdit->setEnabled(enabled && m_registerMode);
    ui->passwordEdit->setEnabled(enabled);
    ui->maleRadio->setEnabled(enabled && m_registerMode);
    ui->femaleRadio->setEnabled(enabled && m_registerMode);
    ui->avatarBtn->setEnabled(enabled && m_registerMode);
    ui->loginBtn->setEnabled(enabled);
    ui->registerBtn->setEnabled(enabled);
}

void LoginWidget::resetLockState()
{
    m_failCount = 0;
    m_locked = false;
    ui->unlockBtn->setVisible(false);
    setInputsEnabled(true);
    ui->statusLabel->setText("");
}

void LoginWidget::updateMode(bool registerMode)
{
    m_registerMode = registerMode;

    ui->titleLabel->setText(registerMode ? "注册账号" : "即时通讯系统");
    setWindowTitle(registerMode ? "注册 - 即时通讯系统" : "登录 - 即时通讯系统");

    ui->avatarLabel->setVisible(registerMode);
    ui->avatarBtn->setVisible(registerMode);
    ui->nicknameLabel->setVisible(registerMode);
    ui->nicknameEdit->setVisible(registerMode);
    ui->genderLabel->setVisible(registerMode);
    ui->maleRadio->setVisible(registerMode);
    ui->femaleRadio->setVisible(registerMode);

    ui->loginBtn->setText(registerMode ? "返回登录" : "登录");
    ui->registerBtn->setText(registerMode ? "确认注册" : "注册账号");
    ui->statusLabel->setText("");

    setInputsEnabled(!m_locked);
}

// ==================== 连接成功 → 执行待处理操作 ====================

void LoginWidget::onConnected()
{
    switch (m_pendingAction) {
    case ActionLogin:
        m_pendingAction = ActionNone;
        doSendLogin();
        break;
    case ActionRegister:
        m_pendingAction = ActionNone;
        doSendRegister();
        break;
    default:
        break;
    }
}

void LoginWidget::doSendLogin()
{
    NetworkClient::instance()->sendLogin(m_pendingAccount, m_pendingPassword);
}

void LoginWidget::doSendRegister()
{
    UserInfo user;
    user.account = m_pendingAccount;
    user.nickname = m_pendingNickname;
    user.gender = m_pendingGender;
    user.avatar = m_currentAvatar;

    NetworkClient::instance()->sendRegister(user, m_pendingPassword);
}

// ==================== 槽函数 ====================

void LoginWidget::onLoginClicked()
{
    if (m_registerMode) {
        updateMode(false);
        return;
    }

    QString account = ui->accountEdit->text().trimmed();
    QString password = ui->passwordEdit->text();

    if (account.isEmpty() || password.isEmpty()) {
        ui->statusLabel->setText("账号和密码不能为空");
        return;
    }

    // 保存待发送的凭证
    m_pendingAccount = account;
    m_pendingPassword = password;

    if (!NetworkClient::instance()->isConnected()) {
        m_pendingAction = ActionLogin;
        NetworkClient::instance()->connectToServer("127.0.0.1", 8888);
        ui->statusLabel->setText("正在连接服务器...");
    } else {
        doSendLogin();
    }
}

void LoginWidget::onRegisterClicked()
{
    if (!m_registerMode) {
        updateMode(true);
        return;
    }

    QString account = ui->accountEdit->text().trimmed();
    QString nickname = ui->nicknameEdit->text().trimmed();
    QString password = ui->passwordEdit->text();

    if (account.isEmpty() || nickname.isEmpty() || password.isEmpty()) {
        ui->statusLabel->setText("账号、昵称和密码不能为空");
        return;
    }

    // 保存待发送的凭证
    m_pendingAccount = account;
    m_pendingNickname = nickname;
    m_pendingPassword = password;
    m_pendingGender = ui->maleRadio->isChecked() ? "男" : "女";

    if (!NetworkClient::instance()->isConnected()) {
        m_pendingAction = ActionRegister;
        NetworkClient::instance()->connectToServer("127.0.0.1", 8888);
        ui->statusLabel->setText("正在连接服务器...");
    } else {
        doSendRegister();
    }
}

void LoginWidget::onTogglePassword()
{
    if (ui->togglePwdBtn->isChecked()) {
        ui->passwordEdit->setEchoMode(QLineEdit::Normal);
        ui->togglePwdBtn->setText("隐藏");
    } else {
        ui->passwordEdit->setEchoMode(QLineEdit::Password);
        ui->togglePwdBtn->setText("显示");
    }
}

void LoginWidget::onChooseAvatar()
{
    QString file = QFileDialog::getOpenFileName(this, "选择头像",
                                                 "", "图片文件 (*.png *.jpg *.bmp)");
    if (!file.isEmpty()) {
        QString avatarData = avatarFileToDataUrl(file);
        if (avatarData.isEmpty()) {
            QMessageBox::warning(this, "头像", "头像加载失败，请重新选择。");
            return;
        }

        m_currentAvatar = avatarData;
        QPixmap pix(file);
        ui->avatarLabel->setPixmap(pix.scaled(80, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        ui->avatarLabel->setStyleSheet("QLabel { border: 2px solid #4caf50; border-radius: 40px; }");
    }
}

void LoginWidget::onUnlockClicked()
{
    bool ok;
    QString answer = QInputDialog::getText(this, "解锁验证",
                                            "验证问题: 1 + 1 = ?",
                                            QLineEdit::Normal, "", &ok);
    if (ok && answer.trimmed() == "2") {
        resetLockState();
        QMessageBox::information(this, "解锁成功",
                                 "验证通过！输入框已恢复。\n"
                                 "提示：请使用正确的账号和密码登录。");
    } else if (ok) {
        QMessageBox::warning(this, "验证失败",
                             "答案错误，请重试。");
    }
}

// ==================== 服务端响应 ====================

void LoginWidget::onLoginResult(bool success, const QString &reason, const UserInfo &userInfo)
{
    if (success) {
        ui->statusLabel->setStyleSheet("color: green; font-size: 12px;");
        ui->statusLabel->setText("登录成功！欢迎 " + userInfo.nickname);
        resetLockState();
        emit loginSuccess(userInfo);
    } else {
        m_failCount++;
        ui->statusLabel->setStyleSheet("color: red; font-size: 12px;");
        ui->statusLabel->setText(QString("登录失败 (%1/3): %2").arg(m_failCount).arg(reason));

        if (m_failCount >= 3 && !m_locked) {
            m_locked = true;
            setInputsEnabled(false);
            ui->unlockBtn->setVisible(true);
            QMessageBox::warning(this, "账户锁定",
                                 "连续输错密码3次，输入框已锁定！\n"
                                 "请点击右侧\"解锁\"按钮进行验证。");
        }
    }
}

void LoginWidget::onRegisterResult(bool success, const QString &reason)
{
    if (success) {
        updateMode(false);
        ui->statusLabel->setStyleSheet("color: green; font-size: 12px;");
        ui->statusLabel->setText("注册成功！请点击登录。");
        QMessageBox::information(this, "注册成功",
                                 "账号注册成功！\n现在可以使用该账号登录了。");
    } else {
        ui->statusLabel->setStyleSheet("color: red; font-size: 12px;");
        ui->statusLabel->setText("注册失败: " + reason);
    }
}
