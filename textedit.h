#ifndef TEXTEDIT_H
#define TEXTEDIT_H
#include <QObject>
#include <QTextEdit>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QDropEvent>

// делаем свой textEdit, для того что бы перехватить событие нажатия клавиши Enter
// и по этому нажатию отправить сообщение
class TextEdit: public QTextEdit
{
    Q_OBJECT
public:
    TextEdit(QWidget* pwgt = nullptr);

    void keyPressEvent(QKeyEvent* e);
protected:
    // с помощью методов dragEnterEvent и dropEvent реализуем функционал
    //прикрепления файла путем его перетаскивания в область набора сообщения
    virtual void dragEnterEvent(QDragEnterEvent* pe);
    virtual void dropEvent(QDropEvent* pe);

signals:
    void sendPathToFile(QString); //отправка полного пути к файлу
};

#endif // TEXTEDIT_H
