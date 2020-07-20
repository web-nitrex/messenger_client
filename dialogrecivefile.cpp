#include "dialogrecivefile.h"
#include "ui_dialogrecivefile.h"
#include <QIcon>
#include <QStyle>

DialogReciveFile::DialogReciveFile(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogReciveFile)
{
    ui->setupUi(this);

    //показываем диалоговое окно поверх остальных окон
    this->setWindowFlags(Qt::WindowStaysOnTopHint);
}

DialogReciveFile::~DialogReciveFile()
{
    delete ui;
}

//обработка запроса на принятие файла
void DialogReciveFile::requestReciveFile(QString fileName, QString fromUser)
{
    //устанавливаем имя файла и от какого пользователя файл пришел
    reciveFile=fileName;
    fromUser_=fromUser;

    //для красоты устанавливаем иконку в диалоговое окно
    QIcon icon = style()->standardIcon(QStyle::SP_MessageBoxInformation);
    QPixmap pixmap = icon.pixmap(QSize(60, 60));
    ui->label->setPixmap(pixmap);

    //отображаем сообщение и отображаем окно модально
    ui->labelMessage->setText("Принять файл " + fileName + " от " + fromUser +" ?");
    this->exec();
}

//обработчик нажатия кнопки "Принять"
void DialogReciveFile::on_pbRecive_clicked()
{
    //принимаем файл
    emit answerReciveFile(true,reciveFile,fromUser_);
    this->hide();
}

//обработчик нажатия кнопки "Отклонить"
void DialogReciveFile::on_pbAbort_clicked()
{
    //отказываемся от принятия файла
    emit answerReciveFile(false,reciveFile,fromUser_);
    this->hide();
}
