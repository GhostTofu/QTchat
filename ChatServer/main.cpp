#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include <QApplication>
#include "servermainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("聊天服务器管理");

    // 设置全局样式
    app.setStyleSheet(
        "QWidget { font-family: 'Microsoft YaHei', 'SimHei', sans-serif; }"
        "QPushButton { padding: 6px 16px; border: 1px solid #ccc; "
        "border-radius: 4px; background: #f5f5f5; }"
        "QPushButton:hover { background: #e0e0e0; }"
    );

    ServerMainWindow window;
    window.show();

    return app.exec();
}
