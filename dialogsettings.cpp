#include "dialogsettings.h"
#include "ui_dialogsettings.h"
#include <QFileDialog>
#include <QSettings>
#include <QRegExpValidator>
#include <QCryptographicHash>
#include <QMessageBox>

DialogSettings::DialogSettings(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogSettings)
{
    ui->setupUi(this);

    //устанавливаем регулярные выражения для полей формы
    setRegExpression();

    //скрываем пароль по умолчанию звездочками
    ui->lineEditPassword->setEchoMode(QLineEdit::Password);

    //устанавливаем подсказки для полей ввода логина и пароля
    ui->lineEditPassword->setToolTip("Строчные и прописные латинские буквы, цифры.");
    ui->lineEditLogin->setToolTip("2-20 символов, которыми могут быть и буквы и цифры, первый символ обязательно буква.");
    ui->lineEditPassword->update();

    //переменная для хранения текущих настроек приложения
    Configuration currentSettings;

    //читаем конфигурацию приложения из реестра
    readConfigFromRegistry(currentSettings);

    //отображаем текущую конфигурацию в диалоговом окне
    showSettings(currentSettings);
}

DialogSettings::~DialogSettings()
{
    delete ui;
}

//метод отображения диалогового окна настроек как модального
void DialogSettings::showModalWindow()
{
    //устанавливаем флаг изменения настроек авторизации
    authDataHasBeenChanged=false;

    //переменная для хранения текущих настроек приложения
    Configuration currentSettings;

    //читаем конфигурацию приложения из реестра
    readConfigFromRegistry(currentSettings);

    //отображаем текущую конфигурацию в диалоговом окне
    showSettings(currentSettings);

    this->exec();
}

//обработчик нажатия кнопки "ОК"
void DialogSettings::on_pbOK_clicked()
{
    //проверка полей
    if(authDataHasBeenChanged)
    {
        if(ui->lineEditPassword->text().size()==0 ||ui->lineEditLogin->text().size()==0)
        {
            QMessageBox::warning(this,"Внимание!","Поля Логин и Пароль должны быть заполнены!");
            return;
        }
    }

    //сохраняем новые настройки приложения в реестре
    saveSettings();

    //уведомляем главное окно приложения о обновлении настроек
    emit updateConfig();

    this->hide();
}

//обработчик нажатия кнопки выбора папки для сохранения файла
void DialogSettings::on_pbSelectFolder_clicked()
{
    //вызываем диалоговое окно выбора папки
    QString pathFolder = QFileDialog::getExistingDirectory(this,tr("Выбор папки"), "/home");

    //показываем полный путь к выбранной папке в диалоговом окне
    if(pathFolder!="")
    {
        ui->labelNameFolder->setText(pathFolder);
    }

}

//сохранение настроек приложения в реестр
void DialogSettings::saveSettings()
{
    //переменная для хранения текущих настроек приложения
    Configuration newConfiguration;

    //настройки соединения
    newConfiguration.IP=ui->lineEditIP->text();
    newConfiguration.port=ui->spinBoxPort->value();
    newConfiguration.autoConnect=ui->cbAutoConnect->isChecked();

    //настройки учетной записи
    if(authDataHasBeenChanged)
    {
        newConfiguration.login=ui->lineEditLogin->text();

        //хэшируем пароль
        QString hashPassword = QString(QCryptographicHash::hash(ui->lineEditPassword->text().toLocal8Bit(),QCryptographicHash::Md5).toHex());
        newConfiguration.password=hashPassword;
    }

    //настройки приложения
    newConfiguration.fontSize=ui->spinBoxFontSize->value();
    newConfiguration.showPopUp=ui->cbShowPopUpMsg->isChecked();
    newConfiguration.downloadDirectory=ui->labelNameFolder->text();
    newConfiguration.sound = ui->cbSound->currentText();

    //записываем новую конфигурацию в реест
    writeConfigToRegistry(newConfiguration);
}

//запись конфигурации приложения в реестр
void DialogSettings::writeConfigToRegistry(const Configuration &settings)
{
    QSettings settingsRegistry("SEALB","ClientMessenger");
    settingsRegistry.beginGroup("/Settings");

    settingsRegistry.setValue("/IP",settings.IP);
    settingsRegistry.setValue("/port",settings.port);
    settingsRegistry.setValue("/autoConnect",settings.autoConnect);

    //если были изменения логина и пароля то в этом случае записываем их в реестр
    if(authDataHasBeenChanged)
    {
        settingsRegistry.setValue("/login",settings.login);
        settingsRegistry.setValue("/password",settings.password);
    }

    settingsRegistry.setValue("/fontSize",settings.fontSize);
    settingsRegistry.setValue("/showPopUp",settings.showPopUp);
    settingsRegistry.setValue("/downloadDirectory",settings.downloadDirectory);
    settingsRegistry.setValue("/sound",settings.sound);
    settingsRegistry.endGroup();

}

//отображение настроек приложения в диалоговом окне
void DialogSettings::showSettings(const Configuration &settings)
{
    //настройки соединения
    ui->lineEditIP->setText(settings.IP);
    ui->spinBoxPort->setValue(settings.port);
    ui->cbAutoConnect->setChecked(settings.autoConnect);

    //настройки приложения
    ui->lineEditLogin->setText(settings.login);
    ui->spinBoxFontSize->setValue(settings.fontSize);
    ui->cbShowPopUpMsg->setChecked(settings.showPopUp);
    ui->labelNameFolder->setText(settings.downloadDirectory);

    if(settings.sound=="Выкл")
        ui->cbSound->setCurrentIndex(0);
        else if (settings.sound=="Whistle") {
            ui->cbSound->setCurrentIndex(1);
        }else if (settings.sound=="Catch") {
            ui->cbSound->setCurrentIndex(2);
        }else {
            ui->cbSound->setCurrentIndex(0);
        }

}

//устанавливаем регулярные выражения для полей IP-адреса, пароля и логина
void DialogSettings::setRegExpression()
{
    /* Создаем строку для регулярного выражения */
    QString ipRange = "(?:[0-1]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])";
    /* Создаем регулярное выражение с применением строки, как
     * повторяющегося элемента
     */
    QRegExp ipRegex ("^" + ipRange
                     + "\\." + ipRange
                     + "\\." + ipRange
                     + "\\." + ipRange + "$");
    /* Создаем Валидатор регулярного выражения с применением
     * созданного регулярного выражения
     */
    QRegExpValidator *ipValidator = new QRegExpValidator(ipRegex, this);
    /* Устанавливаем Валидатор на QLineEdit */
    ui->lineEditIP->setValidator(ipValidator);

    //устанавливаем регулярное выражение для пароля
    //строчные и прописные латинские буквы, цифры
    QRegExp passwdRegex("^[a-zA-Z0-9]+$");
    QRegExpValidator *passValidator = new QRegExpValidator(passwdRegex, this);
    ui->lineEditPassword->setValidator(passValidator);

    //устанавливаем регулярное выражение для логина
    //с ограничением 2-20 символов, которыми могут быть и буквы и цифры, первый символ обязательно буква
    QRegExp loginRegex("^[a-zA-Z][a-zA-Z0-9-_\\.]{1,20}$");
    QRegExpValidator *loginValidator = new QRegExpValidator(loginRegex, this);
    ui->lineEditLogin->setValidator(loginValidator);
}

//чтение конфигурации приложения из реестра
void DialogSettings::readConfigFromRegistry(Configuration &configApp)
{
    QSettings settingsRegistry("SEALB","ClientMessenger");
    settingsRegistry.beginGroup("/Settings");
    configApp.IP = settingsRegistry.value("/IP","127.0.0.1").toString();
    configApp.port = settingsRegistry.value("/port",2323).toInt();
    configApp.autoConnect = settingsRegistry.value("/autoConnect",true).toBool();

    configApp.login = settingsRegistry.value("/login","").toString();
    configApp.password = settingsRegistry.value("/password","").toString();
    configApp.fontSize = settingsRegistry.value("/fontSize",10).toInt();
    configApp.showPopUp = settingsRegistry.value("/showPopUp",true).toBool();
    configApp.downloadDirectory = settingsRegistry.value("/downloadDirectory","C:").toString();
    configApp.sound = settingsRegistry.value("/sound","Выкл.").toString();
    settingsRegistry.endGroup();
}

//обработчик чекбокса отображения пароля в диалоговом окне
void DialogSettings::on_cbShowPassword_clicked(bool checked)
{
    if(checked == true)
    {
        ui->lineEditPassword->setEchoMode(QLineEdit::Normal);
        ui->lineEditPassword->update();
    }
    else
    {
        ui->lineEditPassword->setEchoMode(QLineEdit::Password);
        ui->lineEditPassword->update();
    }
}

//обработчик кнопки регистрации нового аккаунта
void DialogSettings::on_pbRegisterAccount_clicked()
{
    //проверка полей
    if(ui->lineEditPassword->text().size()==0 ||ui->lineEditLogin->text().size()==0)
    {
        QMessageBox::warning(this,"Внимание!","Поля Логин пользователя и Пароль должны быть заполнены!");
        return;
    }

    //сохраняем настройки
    on_pbOK_clicked();

    //отправляем сигнал в главную форму для регистрации
    emit registerAccount();
}

//если имя пользователя или пароль были изменены, выставляем соответствующий флаг, для того что бы осуществить проверку перед сохранением данных
void DialogSettings::on_lineEditLogin_textEdited(const QString &arg1)
{
    authDataHasBeenChanged=true;
}

void DialogSettings::on_lineEditPassword_textEdited(const QString &arg1)
{
    authDataHasBeenChanged=true;
}
