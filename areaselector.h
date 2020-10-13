#ifndef AREASELECTOR_H
#define AREASELECTOR_H

#include <QWidget>
#include <windows.h>
#include <windowsx.h>
#include <QMouseEvent>

class AreaSelector : public QWidget
{
    Q_OBJECT
public:
    AreaSelector(QWidget *parent = nullptr);

protected:
    bool nativeEvent(const QByteArray &eventType, void *message, long *result);
    void mousePressEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);

private:
    int boundaryWidth;
    QPoint clickPos;
};

#endif // AREASELECTOR_H
