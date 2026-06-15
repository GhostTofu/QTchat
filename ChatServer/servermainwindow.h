#ifndef SERVERMAINWINDOW_H
#define SERVERMAINWINDOW_H

#include <QMainWindow>
#include <QTreeWidgetItem>

namespace Ui {
class ServerMainWindow;
}

class ChatServer;

class ServerMainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit ServerMainWindow(QWidget *parent = nullptr);
    ~ServerMainWindow();

private slots:
    void onStartClicked();
    void onStopClicked();
    void onLogMessage(const QString &message);
    void onClientConnected(const QString &address, int port);
    void onClientLoggedIn(const QString &account, const QString &nickname);
    void onClientDisconnected(const QString &account);
    void onErrorOccurred(const QString &error);

private:
    void connectSignals();
    void updateServerState(bool running);
    QTreeWidgetItem* findClientItem(const QString &address, int port);
    void appendLog(const QString &msg, const QString &color = "#d4d4d4");

    Ui::ServerMainWindow *ui;
    ChatServer *m_server;
};

#endif // SERVERMAINWINDOW_H
