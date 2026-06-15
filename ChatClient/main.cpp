#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include <QApplication>
#include <QMessageBox>
#include <QDebug>
#include "loginwidget.h"
#include "mainwidget.h"
#include "networkclient.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("即时通讯系统");

    // 设置全局样式
    app.setStyleSheet(
        "QWidget { font-family: 'Microsoft YaHei', 'SimHei', sans-serif; }"
        "QPushButton { padding: 6px 16px; border: 1px solid #ccc; "
        "border-radius: 4px; background: #f5f5f5; }"
        "QPushButton:hover { background: #e0e0e0; }"
        "QLineEdit { padding: 6px; border: 1px solid #ccc; border-radius: 4px; }"
        "QLineEdit:focus { border-color: #2196f3; }"
    );

    // 默认服务器地址（可通过命令行参数修改）
    QString serverHost = "127.0.0.1";
    int serverPort = 8888;
    if (argc >= 2) serverHost = argv[1];
    if (argc >= 3) serverPort = QString(argv[2]).toInt();

    // 连接服务器
    NetworkClient *net = NetworkClient::instance();
    net->connectToServer(serverHost, serverPort);

    // 显示登录界面
    LoginWidget *loginWidget = new LoginWidget();
    loginWidget->show();

    // 登录成功后切换到主界面
    // LoginWidget 内部处理 failCount 逻辑，最终通过 loginSuccess 信号通知
    QObject::connect(loginWidget, &LoginWidget::loginSuccess, loginWidget,
        [loginWidget](const UserInfo &userInfo) {
            MainWidget *mainWidget = new MainWidget(userInfo);
            mainWidget->show();
            loginWidget->close();
        });

    return app.exec();
}
