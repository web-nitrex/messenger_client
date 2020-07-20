#include "textedit.h"


TextEdit::TextEdit(QWidget* pwgt):QTextEdit(pwgt)
{
    setAcceptDrops(true);
}

//обработчик нажатия клавиши
void TextEdit::keyPressEvent(QKeyEvent* e)
{
    QTextEdit::keyPressEvent(e);
    QWidget::keyPressEvent(e); //передаем событие далше, виджету
}

void TextEdit::dragEnterEvent(QDragEnterEvent* pe)
{
    if (pe->mimeData()->hasFormat("text/uri-list"))
    {
        pe->acceptProposedAction();
    }

}

void TextEdit::dropEvent(QDropEvent* pe)
{
    QList<QUrl> urlList = pe->mimeData()->urls();
    QString str;
    foreach(QUrl url, urlList)
    {
        str += url.toString();
        emit sendPathToFile(str);
    }
}
