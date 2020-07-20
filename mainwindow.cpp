#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QSettings>
#include <QSound>
#include <QDateTime>
#include <QScrollBar>
#include <QFile>
#include <QMessageBox>
#include <QStyleFactory>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //создаем всплывающее окно
    popUp = new PopUp(this);

    //инициализация процедуры сворачивания приложения в трей
    initialTrayIcon();

    //считываем конфигурацию клиента из реестра
    dialogSettings.readConfigFromRegistry(configApp);

    //обновляем конфигурацию программы
    updateConfigApp();

    //подключаем сигналы и слоты для отображения сообщений чата и служебных сообщений
    connect(&client,SIGNAL(displayMessage(Message)),this,SLOT(displayMessage(Message)));
    connect(&client,SIGNAL(displayServiceMessage(QString,QString)),this,SLOT(displayServiceMessage(QString,QString)));

    //подключаем сигнал и слот котрый при установке соединения активирует кнопку "Отправить"
    connect(&client,SIGNAL(connectionIsOK(bool)),this,SLOT(activateSendButton(bool)));

    //подключаем сигнал и слот обновления списка пользователей
    connect(&client,SIGNAL(updateUsersList(QStringList)),this,SLOT(updateUsersList(QStringList)));

    //деактивируем кнопку "Отправить"
    ui->pbSend->setEnabled(false);

    //отключаем пункт меню "Переподключиться"
    ui->actionReconnect->setEnabled(false);

    //задаем высоту поля для ввода сообщений
    myTextEdit.setMaximumHeight(70);

    //устанавливаем textEdit на позицию после label "Message:"
    ui->verticalLayout_2->insertWidget(1,&myTextEdit);

    //настраиваем область отображения сообщений
    ui->textBrowser->setFont(QFont("Arial",configApp.fontSize,QFont::Normal));

    //подключаем слот меню для установки соединения с сервером
    connect(ui->actionConnect,SIGNAL(triggered()),this,SLOT(connectToServer()));

    //вызываем диалог настроек через метод exec() что бы окно было модальным
    connect(ui->actionSettings_2,SIGNAL(triggered()),&dialogSettings,SLOT(showModalWindow()));

    //подключаем слот переподключения к серверу
    connect(ui->actionReconnect,SIGNAL(triggered()),this,SLOT(reconnection()));

    //включаем возможность отображения контекстного меню в поле чата
    ui->textBrowser->setContextMenuPolicy(Qt::CustomContextMenu);

    // Подключаем СЛОТ вызова контекстного меню
    connect(ui->textBrowser, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slotCustomMenuRequested(QPoint)));

    //обновление конфигурации приложения
    connect(&dialogSettings,SIGNAL(updateConfig()),this,SLOT(updateConfigApp()));

    //регистрация аккаунта
    connect(&dialogSettings,SIGNAL(registerAccount()),this,SLOT(registerAccount()));

    //подключаем сигнал срабатывания таймера мигания иконкой в трее к обработчику
    connect(&timer,SIGNAL(timeout()),this,SLOT(eventTimer()));

    //подключаем сигнал срабатывания таймера мигания иконкой в списке пользователей к обработчику
    connect(&timerMsg,SIGNAL(timeout()),this,SLOT(incomingMessage()));

    //при поступлении сигнала от клиента о запросе принятия файла, вызываем соответствующее диалоговое окно
    connect(&client,SIGNAL(requestReciveFile(QString,QString)),&dialogReciveFile,SLOT(requestReciveFile(QString,QString)));

    //подключаем сигнал ответа принятия файла к клиенту
    connect(&dialogReciveFile,SIGNAL(answerReciveFile(bool,QString,QString)),&client,SLOT(answerReciveFile(bool, QString, QString)));

    //соединяем сигналы и слоты для получения пути к файлу при перетаскивании его в myTextEdit
    connect(&myTextEdit,SIGNAL(sendPathToFile(QString)),this,SLOT(recivePathToFile(QString)));

    //при закрытии программы скрываем иконку в трее
    connect(this,SIGNAL(destroyed()),this,SLOT(hideTrayIcon()));

    //если установлено автоподключение то сразу подключаемся к серверу
    if(configApp.autoConnect)
        connectToServer();
}

MainWindow::~MainWindow()
{
    delete ui;
}

//поиск непрочитанных сообщений у всех пользователей
bool MainWindow::findUnreadMessages()
{
    for(auto& user : usersList )
    {
        if(user.second.unreadMessages==true)
            return true;
    }

    return false;
}

//подключение к серверу
void MainWindow::connectToServer()
{
    client.connectToServer(configApp.IP,configApp.port,configApp.login,configApp.password,false);
}

//переподключение к серверу
void MainWindow::reconnection()
{
    //client.reconnection(configApp.IP,configApp.port,configApp.login,configApp.password,false);
    client.connectToServer(configApp.IP,configApp.port,configApp.login,configApp.password,false);
}

//обработчик нажатия на клавишу "Enter"
void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    {
        //если соединение установлено только тогда отправляем сообщение
        if(ui->pbSend->isEnabled()==true)
            on_pbSend_clicked();
    }
}

//отображение сообщения чата
void MainWindow::displayMessage(Message msg)
{
    //формируем окончательный вид сообщения
    QString resultMsg = "<b>"+msg.nickname+"</b>" + "<br>"+msg.dateTime + "<br>"+msg.message + "<p>";

    //отправляем новое сообщение в хранилище
    usersList[msg.nickname].messages.push_back(resultMsg);

    //если сейчас открыт чат пользователя от которого пришло сообщение
    if(activeRecipient==msg.nickname)
    {
        ui->textBrowser->clear();

        //выводим все сообщения для юзера
        for(const auto& msg : usersList[activeRecipient].messages)
        {
            ui->textBrowser->append(msg);
        }

        //прокручиваем скроллбар вниз
        QScrollBar* sb = ui->textBrowser->verticalScrollBar();
        sb->setValue(sb->maximum());
    }
    else
    {
        //делаем оранжевой иконку того юзера для кого пришло сообщение и начинаем ей мигать
        usersList[msg.nickname].itemIsMessage->setIcon(QPixmap(":resource/image/iconMessage.png"));
        usersList[msg.nickname].unreadMessages=true;
        timerMsg.start(1000);
    }

    //если в настройках установлена опция отображения всплывающего окна, то отображаем его
    if(configApp.showPopUp)
    {
        //если окно свернуто в трей то показываем общее количество непрочитанных сообщений
        if(!this->isVisible())
        {
            ++count_messages;
            timer.start(1000);
            popUp->setPopupText("Новых сообщений: "+QString::number(count_messages));
            popUp->show(1);
        }
        else
        {
            //если окно открыто то просто показываем от кого пришло сообщение
            popUp->setPopupText("Новое сообщение от "+msg.nickname);
            popUp->show(1);
        }
    }

    //если в настройках включен звук то проигрываем его
    playSound();
}

//отображение служебных сообщений
void MainWindow::displayServiceMessage(QString msg, QString color)
{
    QString text = "<font color=\"" + color +"\">"+msg+"</font>";
    ui->textBrowser->append(text);
    ui->textBrowser->append("");

    //показываем служебное сообщение во всплывающем окне
    if(configApp.showPopUp)
    {
        popUp->setPopupText(text);
        popUp->show(1);
    }

    //если это сообщение об операции с файлом то воспроизводим звук
    if(color=="blue")
    {
        playSound();
    }
}

//активация/деактивация кнопки "Отправить" и пунктов меню "Соединение" и переподключение
void MainWindow::activateSendButton(bool value)
{
    ui->pbSend->setEnabled(value);
    ui->actionReconnect->setEnabled(value);
    ui->actionConnect->setEnabled(!value);
}

//обработчик нажатия кнопки "Отправить"
void MainWindow::on_pbSend_clicked()
{
    //если нет получателя то не отправляем сообщение
    if(activeRecipient=="")
        return;

    //отправляем файл
    if(ui->cbSendFile->isChecked())
    {
        //узнаем размер файла
        QFile file(ui->labelPathToFile->text());
        quint64 size = file.size();

        //передаем файл размером до 200 мб
        if(size>200000000)
        {
            QMessageBox::warning(this,"Внимание!","Размер передаваемого файла должен быть не более 200 Мбайт!");
            return;
        }

        //передаем полный путь к файлу клиенту для его непосредственной отправки
        client.sendFileToServer(ui->labelPathToFile->text(),activeRecipient,configApp.login);

        //снимаем флаг прикрепления файла и очищаем отображение полного пути к файлу
        ui->cbSendFile->setChecked(false);
        ui->labelPathToFile->clear();
    }
    else
    {
        //формируем сообщение для отправки
        Message msg;
        msg.nickname = configApp.login;
        msg.message = myTextEdit.toPlainText();
        QString resultMsg = "<b>"+configApp.login+"</b>"
                            + "<br>"+QDateTime::currentDateTime().toString("dd.MM.yyyy hh.mm.ss")
                            + "<br>"+myTextEdit.toPlainText()
                            + "<p>";

        //отправляем сообщение в хранилище
        usersList[activeRecipient].messages.push_back(resultMsg);

        ui->textBrowser->clear();

        //выводим все сообщения для юзера
        for(const auto& msg : usersList[activeRecipient].messages)
        {
            ui->textBrowser->append(msg);
        }

        //прокручиваем скроллбар вниз
        QScrollBar* sb = ui->textBrowser->verticalScrollBar();
        sb->setValue(sb->maximum());

        //отправляем сообщение в сеть
        client.sendMessageToServer(msg,activeRecipient);

        //очищаем поле ввода сообщения
        myTextEdit.clear();
    }

}

//обработчик вызова контекстного меню в поле чата
void MainWindow::slotCustomMenuRequested(QPoint pos)
{
    //создаем объект контекстного меню
    QMenu * menu = new QMenu(this);

    //создаём действия для контекстного меню
    QAction * clearChat = new QAction(trUtf8("Очистить чат"), this);

    //подключаем слоты обработчики для действий контекстного меню
    connect(clearChat, SIGNAL(triggered()), ui->textBrowser, SLOT(clear()));     // Обработчик вызова диалога редактирования

    //устанавливаем действия в меню
    menu->addAction(clearChat);

    //вызываем контекстное меню
    menu->popup(ui->textBrowser->viewport()->mapToGlobal(pos));
}

//обновление конфигурации приложения
void MainWindow::updateConfigApp()
{
    //считываем конфигурацию клиента из реестра
    dialogSettings.readConfigFromRegistry(configApp);

    //устанавливаем размер шрифта для списка пользоватедей и для сообщений чата
    ui->textBrowser->setFont(QFont("Arial",configApp.fontSize,QFont::Normal));
    ui->listUserOnline->setFont(QFont("Arial",configApp.fontSize,QFont::Normal));
    ui->textBrowser->update();

    //в соответствии с логином устанавливаем заголовок приложения
    this->setWindowTitle(configApp.login);

    //в клиенте устанавливаем папку для загрузки файлов
    client.setDownloadDirectory(configApp.downloadDirectory);
}

//обработчик изменения флага "Прикрепить файл"
void MainWindow::on_cbSendFile_clicked(bool checked)
{
    if(checked && (ui->labelPathToFile->text()==""))
    {
        //открываем диалоговое окно выбора файла
        QString pathFile = QFileDialog::getOpenFileName(this,tr("Выбор файла"), "/home");
        if(pathFile!="")
        {
            ui->labelPathToFile->setText(pathFile);
        }
        else
        {
            //если файл не выбран то снимаем флаг прикрепления файла и очищаем поле полного пути к файлу
            ui->cbSendFile->setChecked(false);
            ui->labelPathToFile->clear();
        }
    }
    else
    {
        //если флаг снимается то очищаем поле полного пути к файлу
        ui->labelPathToFile->clear();
    }

}

//инициализация иконки в трее
void MainWindow::initialTrayIcon()
{
    //Инициализируем иконку трея, устанавливаем иконку, а так же задаём всплывающую подсказку
    trayIcon = new QSystemTrayIcon(this);

    //устанавливаем изображение иконки и всплывающую подсказку
    QIcon trayImage(":resource/image/icon.png");
    trayIcon->setIcon(trayImage);
    trayIcon->setToolTip("Messenger");

    //После чего создаем контекстное меню из двух пунктов
    QMenu * menu = new QMenu(this);
    QAction * viewWindow = new QAction(trUtf8("Развернуть окно"), this);
    QAction * quitAction = new QAction(trUtf8("Выход"), this);

    // подключаем сигналы нажатий на пункты меню к соответсвующим слотам.
    // первый пункт меню разворачивает приложение из трея, а второй пункт меню завершает приложение
    connect(viewWindow, SIGNAL(triggered()), this, SLOT(show()));
    connect(quitAction, SIGNAL(triggered()), this, SLOT(close()));

    menu->addAction(viewWindow);
    menu->addAction(quitAction);

    //устанавливаем контекстное меню на иконку и показываем иконку приложения в трее
    trayIcon->setContextMenu(menu);
    trayIcon->show();

    //так же подключаем сигнал нажатия на иконку к обработчику данного нажатия
    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),this, SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));
}

//воспроизведение звука при принятии сообщения или файла
void MainWindow::playSound()
{
    if(configApp.sound!="Выкл.")
    {
        QSound::play(":resource/sound/"+configApp.sound+".wav");
    }
}

//обработчик сворачивания приложения в трей
void MainWindow::hideEvent(QHideEvent *event)
{
    if(this->isVisible())
    {
       event->ignore();
       this->hide();
    }
}

//обработчик отображения (разворачивания) окна
void MainWindow::showEvent(QShowEvent *event)
{
    //как только окно развернулось обнуляем счетчик новых сообщений и останавливаем мигание иконкой
    count_messages=0;
    timer.stop();

    //устанавливаем стандартную иконку
    QIcon trayImage(":resource/image/icon.png");
    trayIcon->setIcon(trayImage);
}

//обработчик события разворачивания приложения из трея
void MainWindow::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
   switch (reason)
   {
       case QSystemTrayIcon::Trigger:
           if(!this->isVisible())
           {
              //если приложение свернуто то разворачиваем его
              this->setWindowState(this->windowState() & ~Qt::WindowMinimized | Qt::WindowActive);
              this->show();
           }
           else
           {
               //иначе так же оставляем скрытым
               this->hide();
           }
       break;
   default:
       break;

   }
}

//обработчик события таймера для мигания икнокой в трее
void MainWindow::eventTimer()
{
        static bool flag=false;
        flag=!flag;

        if(flag)
        {
            QIcon trayImage(":resource/image/message.png");
            trayIcon->setIcon(trayImage);
        }
        else {
            QIcon trayImage(":resource/image/icon.png");
            trayIcon->setIcon(trayImage);
        }
}

//обновление списка пользователей
void MainWindow::updateUsersList(QStringList usersOnline)
{
   //очищаем информацию по старым пользователям
   ui->listUserOnline->clear();
   usersList.clear();
   activeRecipient="";
   ui->label_4->setText("Сообщение:");

   //устанавливаем размер иконок в списке пользователей
   ui->listUserOnline->setIconSize(QSize(15,15));

   for(auto user : usersOnline)
   {
        //не выводим самого себя в списке онлайн
        if(user==configApp.login)
            continue;

        //создаем пользователя в списке
        QListWidgetItem* item = new QListWidgetItem(user,ui->listUserOnline);
        item->setIcon(QPixmap(":resource/image/iconOnline.png"));

        //запоминаем икноку, которая соответствует данному пользователю, для того что бы в последствии ей можно было мигать
        usersList[user].itemIsMessage=item;
   }

}

//регистрация нового аккаунта
void MainWindow::registerAccount()
{
    //при регистрации нового аккаунта просто переподключаемся к серверу с логином и паролем и выставляем флаг что это регистрация
    //client.reconnection(configApp.IP,configApp.port,configApp.login,configApp.password,true);
    client.connectToServer(configApp.IP,configApp.port,configApp.login,configApp.password,true);
}

//мигание иконкой в списке пользователей, при поступлении нового сообщения
void MainWindow::incomingMessage()
{
    static bool flag=false;
    flag=!flag;
    if(flag)
    {
        for(auto& user : usersList )
        {
            if(user.second.unreadMessages==true)
                user.second.itemIsMessage->setIcon(QPixmap(":resource/image/iconOnline.png"));
        }
    }
    else
    {
        for(auto& user : usersList )
        {
            if(user.second.unreadMessages==true)
                user.second.itemIsMessage->setIcon(QPixmap(":resource/image/iconMessage.png"));
        }
    }
}

//получение полного пути к файлу при перетаскивании его в поле ввода сообщения
void MainWindow::recivePathToFile(QString pathToFile)
{
    //устанавливаем флаг что прикрепляем файл
    ui->cbSendFile->setChecked(true);

    //удаляем из абсолютного пути к файлу ненужную информацию
    pathToFile=pathToFile.remove(0,8);

    //отображаем полный путь к файлу
    ui->labelPathToFile->setText(pathToFile);
}

//обработчик двойного клика по пользователю в списке пользователей
void MainWindow::on_listUserOnline_itemDoubleClicked(QListWidgetItem *item)
{
    //устанавливаем активного получателя и очищаем окно вывода сообщений
    activeRecipient = item->text();
    ui->textBrowser->clear();

    //делаем зеленой иконку того юзера для кого пришло сообщение и устанавливаем флаг что у него нет непрочитанных сообщений
    usersList[activeRecipient].itemIsMessage->setIcon(QPixmap(":resource/image/iconOnline.png"));
    usersList[activeRecipient].unreadMessages = false;

    //есть ли еще непрочитанные сообщения у пользователей? если у всех все прочитано то отключаем таймер мигания значком в списке пользователей
    if(!findUnreadMessages() && timerMsg.isActive())
        timerMsg.stop();

    //выводим все сообщения для юзера
    for(const auto& msg : usersList[activeRecipient].messages)
    {
        ui->textBrowser->append(msg);
    }

    //прокручиваем скроллбар вниз
    QScrollBar* sb = ui->textBrowser->verticalScrollBar();
    sb->setValue(sb->maximum());

    ui->label_4->setText("Сообщение для "+activeRecipient+":");
}

//скрываем иконку в трее
void MainWindow::hideTrayIcon()
{
    trayIcon->hide();
}
