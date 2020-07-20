#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QKeyEvent>
#include "myclient.h"
#include "textedit.h"
#include <QStringList>
#include "dialogsettings.h"
#include "dialogrecivefile.h"
#include <QSystemTrayIcon>
#include "popup.h"
#include <QTimer>
#include <QListWidgetItem>
#include <map>
#include <QVector>


//структура для хранения текущих настроек пользователя из списка пользователей
struct UserCongiguration{
    QListWidgetItem* itemIsMessage=nullptr; //указатель для иконки пользователя
    bool unreadMessages=false;              //флаг о том что есть непрочитанные сообщения
    QVector<QString> messages;              //вектор для хранения сообщений от пользователя
};

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    MyClient client;                                //клиент, через который идет все сетевое взаимодействие
    TextEdit myTextEdit;                            //поле для ввода сообщения чата
    DialogSettings dialogSettings;                  //диалоговое окно настроек приложения
    DialogReciveFile dialogReciveFile;              //диалоговое окно подтверждения принятия файла
    Configuration configApp;                        //текущие настройки приложения
    PopUp *popUp;                                   //высплывающее окно уведомления
    QSystemTrayIcon *trayIcon;                      //иконка приложения в трее
    QTimer timer;                                   //таймер для мигания иконкой в трее, при приеме сообщения
    QTimer timerMsg;                                //таймер для мигания иконкой в списке пользователей
    QString activeRecipient;                        //имя активного получателя (получатель чат которого открыт на данный момент)
    std::map<QString,UserCongiguration> usersList;  //логин - конфигурация для каждого пользователя
    unsigned count_messages=0;                      //счетчик новых сообщений
    bool findUnreadMessages();
    void initialTrayIcon();
    void playSound();
private slots:

    void connectToServer();
    void reconnection();
    void displayMessage(Message msg);
    void displayServiceMessage(QString msg,QString color);
    void activateSendButton(bool value);
    void on_pbSend_clicked();
    void slotCustomMenuRequested(QPoint pos);
    void updateConfigApp();
    void on_cbSendFile_clicked(bool checked);
    void iconActivated(QSystemTrayIcon::ActivationReason reason);   //слот, который будет принимать сигнал от события нажатия на иконку в трее
    void eventTimer();
    void updateUsersList(QStringList usersOnline);
    void registerAccount();
    void incomingMessage();
    void recivePathToFile(QString pathToFile);
    void on_listUserOnline_itemDoubleClicked(QListWidgetItem *item);
    void hideTrayIcon();

protected:
    virtual void keyPressEvent(QKeyEvent* event);   //обработчик события нажатия на Enter
    void hideEvent(QHideEvent *event);
    void showEvent(QShowEvent *event);
};
#endif // MAINWINDOW_H
