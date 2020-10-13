#ifndef AREASELECTOR_H
#define AREASELECTOR_H

#include <QWidget>
#include <windows.h>
#include <windowsx.h>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QDebug>
#include <QFontMetrics>
#include <QTimer>

class AreaSelector : public QWidget
{
    Q_OBJECT
public:
    AreaSelector(QWidget *parent = nullptr);

    void setPaint(bool paint);
    QRect getArea();
    void setArea(QRect rect);

protected:
    bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

signals:
    void areaChanged(QRect rect);

private:
    int fontHeight = 0;
    int boundaryWidth = 8;
    int boundaryShowed = 2;
    QPoint clickPos;
    bool shouldPaint = true;
};

#endif // AREASELECTOR_H
