#ifndef MYCLIENT_H
#define MYCLIENT_H

#include <QObject>
#include <QTcpSocket>

//типы передаваемых по сети сообщений
struct  TypeMessage
{
    static const quint16 Text =1;
    static const quint16 File=2;
    static const quint16 AuthRequest=3;
    static const quint16 AuthAnswer=4;
    static const quint16 RegistrationRequest=5;
    static const quint16 UsersList=6;
};

//структура для упаковки сообщения чата
struct Message{
    QString dateTime;
    QString nickname;
    QString message;
};

class MyClient : public QObject
{
    Q_OBJECT
public:

    MyClient(QObject* obj=nullptr);
    void connectToServer(const QString& strHost, int port, const QString& login_,const QString& password_,bool registration_);
    void sendMessageToServer(const Message& str, const QString& recipient);
    void sendFileToServer(const QString& pathToFile,const QString& recipient, const QString& from);
    void sendRequestAuthToServer();
    void setDownloadDirectory(const QString& dir);

private:
    QTcpSocket* tcpSocket;
    quint64 nextBlockSize;     //размер блока принимаемых данных
    QString downloadDirectory; //папка для сохранения файла
    QString login;             //логин текущего подключения
    QString password;          //пароль текущего подключения
    QByteArray arrFile;        //хранилище содержимого принимаемого файла
    bool registration;         //флаг запроса на регистрацию

private slots:
    void slotReadyRead();
    void slotError(QAbstractSocket::SocketError);
    void slotConnected();
    void slotDisconnected();
    void answerReciveFile(bool answer, QString fileName, QString from);

signals:
    void displayMessage(Message msg);                           //отображение сообщения в главном окне
    void displayServiceMessage(QString msg,QString color);      //отображение сервисного сообщения
    void connectionIsOK(bool);                                  //сигнал о наличии установленного подключения
    void updateUsersList(QStringList);                          //обновление списка пользователей
    void requestReciveFile(QString fileName,QString fromUser);  //запрос на принятие файла

};

#endif // MYCLIENT_H
