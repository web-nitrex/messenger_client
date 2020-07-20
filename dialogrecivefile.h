#ifndef DIALOGRECIVEFILE_H
#define DIALOGRECIVEFILE_H

#include <QDialog>

namespace Ui {
class DialogReciveFile;
}

class DialogReciveFile : public QDialog
{
    Q_OBJECT

public:
    explicit DialogReciveFile(QWidget *parent = nullptr);
    ~DialogReciveFile();

private:
    Ui::DialogReciveFile *ui;
    QString reciveFile,fromUser_; //имя принимаемого файла и от какого пользователя файл

public slots:
    void requestReciveFile(QString fileName, QString fromUser);

signals:
    void answerReciveFile(bool answer, QString fileName, QString from); //ответ на запрос о принятии файла

private slots:
    void on_pbRecive_clicked();
    void on_pbAbort_clicked();
};

#endif // DIALOGRECIVEFILE_H
