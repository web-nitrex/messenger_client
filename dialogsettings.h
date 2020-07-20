#ifndef DIALOGSETTINGS_H
#define DIALOGSETTINGS_H

#include <QDialog>

//структура для хранения конфигурации приложения
struct Configuration{
    QString login="";               //имя пользователя
    QString password="";            //пароль
    QString IP="127.0.0.1";         //IP-адрес сервера
    int port=2323;                  //порт на котором запущен сервер
    bool autoConnect=true;;         //автоматическое соединение с сервером при запуске приложения
    int fontSize=10;                //размер шрифта чата
    bool showPopUp=true;            //показывать всплывающие уведомления
    QString downloadDirectory="C:";   //папка для загрузки файла
    QString sound="Выкл.";          //звук уведомления

};

namespace Ui {
class DialogSettings;
}

class DialogSettings : public QDialog
{
    Q_OBJECT

public:
    explicit DialogSettings(QWidget *parent = nullptr);
    void readConfigFromRegistry(Configuration &configApp);
    ~DialogSettings();

public slots:
    void showModalWindow();

private slots:
    void on_pbOK_clicked();
    void on_pbSelectFolder_clicked();
    void on_cbShowPassword_clicked(bool checked);
    void on_pbRegisterAccount_clicked();

    void on_lineEditLogin_textEdited(const QString &arg1);

    void on_lineEditPassword_textEdited(const QString &arg1);

private:
    Ui::DialogSettings *ui;
    bool authDataHasBeenChanged=false; //флаг изменения логина или пароля
    void saveSettings();
    void writeConfigToRegistry(const Configuration& settings);
    void showSettings(const Configuration& settings);
    void setRegExpression();

signals:
    void updateConfig();    //обновление конфигурации приложения
    void registerAccount(); //регистрация нового аккаунта

};

#endif // DIALOGSETTINGS_H
