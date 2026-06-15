#ifndef LOGINWIDGET_H
#define LOGINWIDGET_H

#include <QWidget>
#include <QButtonGroup>
#include "networkclient.h"

namespace Ui {
class LoginWidget;
}

class LoginWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LoginWidget(QWidget *parent = nullptr);
    ~LoginWidget();

signals:
    void loginSuccess(const UserInfo &userInfo);

private slots:
    void onLoginClicked();
    void onRegisterClicked();
    void onTogglePassword();
    void onChooseAvatar();
    void onUnlockClicked();

    void onLoginResult(bool success, const QString &reason, const UserInfo &userInfo);
    void onRegisterResult(bool success, const QString &reason);

private:
    void setInputsEnabled(bool enabled);
    void resetLockState();
    void updateMode(bool registerMode);

    Ui::LoginWidget *ui;
    QButtonGroup *m_genderGroup;

    QString m_currentAvatar;
    int m_failCount;
    bool m_locked;
    bool m_registerMode;
};

#endif // LOGINWIDGET_H
