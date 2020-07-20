#include "myclient.h"
#include <QDataStream>
#include <QDateTime>
#include <QFile>

using namespace std;

//перегружаем операторы выввода и ввода в поток, для записи туда структуры сообщения
QDataStream& operator <<(QDataStream& out, const Message& msg)
{
    out << msg.dateTime;
    out << msg.nickname;
    out << msg.message;
    return out;
}

QDataStream& operator >>(QDataStream& in, Message& msg)
{
    in >> msg.dateTime;
    in >> msg.nickname;
    in >> msg.message;
    return in;
}

//подключение клиента к серверу
MyClient::MyClient(QObject *obj): QObject(obj),tcpSocket(nullptr), nextBlockSize(0),registration(false)
{
    //создаем новый сокет
    tcpSocket = new QTcpSocket(this);

    //подключаем сигналы и слоты для сокета
    connect(tcpSocket, SIGNAL(connected()),this,SLOT(slotConnected()));
    connect(tcpSocket, SIGNAL(disconnected()),this,SLOT(slotDisconnected()));
    connect(tcpSocket, SIGNAL(readyRead()),this,SLOT(slotReadyRead()));
    connect(tcpSocket,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(slotError(QAbstractSocket::SocketError)));
}

void MyClient::connectToServer(const QString &strHost, int port, const QString& login_,const QString& password_,bool registration_)
{    
    //если соединение уже установлено и требуется установка нового, то отключаемся
    if(tcpSocket->state()==QAbstractSocket::ConnectedState)
    {
        tcpSocket->close();
    }

    //запоминаем данные для авторизации или регистрации на сервере
    this->login = login_;
    this->password = password_;
    this->registration = registration_;

    //устанавливаем соединение с сервером
    tcpSocket->connectToHost(strHost,port);
}

//отправка сообщения на сервер
void MyClient::sendMessageToServer(const Message &msg, const QString& recipient)
{
    QByteArray arrBlock;
    QDataStream out(&arrBlock,QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_3);

    //записываем в поток размер передаваемых данных, тип сообщения и само сообщение
    out << quint64(0)<<TypeMessage::Text<<recipient<<msg;

    //сдвигаемся к началу потока, для того что бы указать размер передаваемых данных
    out.device()->seek(0);
    out << quint64(arrBlock.size()-sizeof(quint64));

    //записываем данные в сокет
    tcpSocket->write(arrBlock);
    tcpSocket->flush();
}

//отправка файла на сервер
void MyClient::sendFileToServer(const QString &pathToFile, const QString& recipient, const QString& from)
{
    QByteArray block;
    QDataStream stream(&block, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_5_3);

    //проверка существования файла
    if(!QFile::exists(pathToFile))
    {
        emit displayServiceMessage("<b>Файл " + pathToFile+ " не существует!</b>\n","red");
        return;
    }

    //открываем файл и считываем все его содержимое
    QFile file(pathToFile);
    file.open(QIODevice::ReadOnly);
    QByteArray buf = file.readAll();

    //получаем из полного пути к файлу список папок и непосредственно имя самого файла
    QStringList path = pathToFile.split("/");

    //отрезаем имя файла от его полного пути, для последующей передачи этого имени в сеть
    QString fileName=path.back();

    //записываем необходимые данные в поток
    stream << quint64(0);
    stream << TypeMessage::File;
    stream << recipient;
    stream << from;
    stream <<fileName;
    stream << buf;

    //сдвигаемся к началу потока, для того что бы указать размер передаваемых данных
    stream.device()->seek(0);
    stream << quint64(block.size()-sizeof(quint64));

    //записываем данные в сокет
    tcpSocket->write(block);
    tcpSocket->flush();

    emit displayServiceMessage("<b>Файл " + fileName+ " успешно отправлен!</b>\n","blue");
}

//отправка запроса на регистрацию или авторизацию
void MyClient::sendRequestAuthToServer()
{
    QByteArray arrBlock;
    QDataStream out(&arrBlock,QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_3);

    //в зависимости от установки флага записываем в поток данные необходимые для регистрации или аторизации
    if(registration)
    {
        out << quint64(0)<<TypeMessage::RegistrationRequest<<login<<password;
    }
    else
    {
        out << quint64(0)<<TypeMessage::AuthRequest<<login<<password;
    }

    //сдвигаемся к началу потока, для того что бы указать размер передаваемых данных
    out.device()->seek(0);
    out << quint64(arrBlock.size()-sizeof(quint64));

    //записываем данные в сокет
    tcpSocket->write(arrBlock);
    tcpSocket->flush();
}

//установки папки для загрузки файла
void MyClient::setDownloadDirectory(const QString &dir)
{
    downloadDirectory=dir;
}

//чтение данных из сокета
void MyClient::slotReadyRead()
{
    QDataStream in(tcpSocket);

    in.setVersion(QDataStream::Qt_5_3);

    for(;;)
    {
        //проверяем что это первый прием данных и до этого ничего не принимали
        if(!nextBlockSize)
        {
            //если количество входящих байт, которые ожидают считывания меньше чем размер sizeof переменной
            //в которой мы указываем размер блока данных то выходим из цикла
            if(tcpSocket->bytesAvailable() < sizeof (quint64))
            {
                break;
            }

            //если все в порядке, то считываем общее количество принимаемых данных
            in >> nextBlockSize;
        }

        //если нет больше данных то прекращаем чтение
        if(tcpSocket->bytesAvailable() < nextBlockSize)
        {
            break;
        }

        //тип принятого сообщения
        quint16 typeMsg;

        //считываем тип принятого сообщения
        in >> typeMsg;

        if(typeMsg == TypeMessage::Text)
        {
            Message msg;

            //записываем принятые данные в строку
            in >> msg;

            //добавляем к принятой строке текущие дату и время
            msg.dateTime = QDateTime::currentDateTime().toString("dd.MM.yyyy hh.mm.ss");

            //выводим принятое содержимое
            emit displayMessage(msg);

        }
        else if (typeMsg == TypeMessage::File)
        {
            QString fileName,from,str;
            arrFile.clear();

            //считываем от кого принят файл, имя файла и само содержимое файла
            in >> from;
            in >> fileName;
            in >> arrFile;

            //отправляем запрос на сохранение файла
            emit requestReciveFile(fileName,from);

        }
        else if (typeMsg == TypeMessage::AuthAnswer)
        {
            QString msgAuth;

            //считываем ответ авторизации
            in >>msgAuth;
            emit displayServiceMessage(msgAuth+"\n","orange");
        }
        else if (typeMsg == TypeMessage::UsersList)
        {
            QStringList usersOnline;

            //считываем список пользователей подключенных к серверу
            in >> usersOnline;
            emit updateUsersList(usersOnline);
        }

        //показываем что данных больше нет, что бы выйти из слота чтения данных
        nextBlockSize=0;

    }
}


//обработка ошибки в сокете
void MyClient::slotError(QAbstractSocket::SocketError err)
{
    QString strError = "Error: " + (err == QAbstractSocket::HostNotFoundError ? "The host was not found." :
                                    err == QAbstractSocket::RemoteHostClosedError ? "The remote host is closed." :
                                    err == QAbstractSocket::ConnectionRefusedError ? "The connection was refused." :
                                    QString(tcpSocket->errorString()));
    displayServiceMessage(strError,"red");
}

//обработчик успешного подключения к серверу
void MyClient::slotConnected()
{
    emit displayServiceMessage("The connection to the server was successfully established!\n","green");
    //отсылаем запрос авторизации на сервер
    sendRequestAuthToServer();
    //указываем что соединение установлено
    emit connectionIsOK(true);
}

//обработка слота отключения от сервера
void MyClient::slotDisconnected()
{
    //удаляем сокет
    //tcpSocket->deleteLater();

    emit displayServiceMessage("<font color=\"red\">The connection is closed!</font>\n","red");

    //указываем что соединение разорвано
    emit connectionIsOK(false);
}

//обработчик ответа пользователя на сохранение файла
void MyClient::answerReciveFile(bool answer, QString fileName, QString from)
{
    if(answer)
    {
        //формируем полный путь к файлу
        fileName = downloadDirectory+"/"+fileName;

        //создаем файл и записываем в него данные
        QFile file(fileName);
        file.open(QIODevice::WriteOnly);
        file.write(arrFile);
        file.close();
        arrFile.clear();

        //отправляем сообщение об успешном принятии файла
        QString str = QDateTime::currentDateTime().toString("dd.MM.yyyy hh.mm.ss") + "<br>"+from+"<br>"+"Принят файл: " + fileName+ "\n";
        emit displayServiceMessage(str,"blue");
    }
    else
    {
        //пользователь отказался от принятия файла. Очищаем его содержимое.
        arrFile.clear();
    }
}
