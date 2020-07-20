#ifndef POPUP_H
#define POPUP_H

#include <QWidget>
#include <QLabel>
#include <QGridLayout>
#include <QPropertyAnimation>
#include <QTimer>

class PopUp : public QWidget
{
    Q_OBJECT

    // Свойство полупрозрачности
    Q_PROPERTY(float popupOpacity READ getPopupOpacity WRITE setPopupOpacity)

    void setPopupOpacity(float opacity);
    float getPopupOpacity() const;

public:
    explicit PopUp(QWidget *parent = 0);

protected:
    void paintEvent(QPaintEvent *event);    // Фон будет отрисовываться через метод перерисовки
    void mousePressEvent(QMouseEvent* event);
public slots:
    void setPopupText(const QString& text); // Установка текста в уведомление
    void show(int type);                            /* Собственный метод показа виджета
                                             * Необходимо для преварительной настройки анимации
                                             * */

private slots:
    void hideAnimation();                   // Слот для запуска анимации скрытия
    void hide();                            /* По окончании анимации, в данном слоте делается проверка,
                                             * виден ли виджет, или его необходимо скрыть
                                             * */

private:
    QLabel label;           // Label с сообщением
    QGridLayout layout;     // Размещение для лейбла
    QPropertyAnimation animation;   // Свойство анимации для всплывающего сообщения
    float popupOpacity;     // Свойства полупрозрачности виджета
    QTimer *timer;          // Таймер, по которому виджет будет скрыт
signals:
    void showMainWindow();
};

#endif // POPUP_H
