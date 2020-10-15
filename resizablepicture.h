#ifndef RESIZABLEPICTURE_H
#define RESIZABLEPICTURE_H

#include <QObject>
#include <QWidget>
#include <QLabel>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QDebug>

class ResizablePicture : public QWidget
{
    Q_OBJECT
public:
    ResizablePicture(QWidget *parent = nullptr);

    bool setPixmap(const QPixmap& pixmap);
    void resetScale();

    void setScaleCache(bool enable);
    void scaleTo(double scale, QPoint pos);
    void scaleToFill();
    void scaleToOrigin();

protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *) override;

    qint64 sizeToLL(QSize size);

signals:

public slots:

private:
    QLabel* label;

    QPoint pressPos;
    QPixmap originPixmap;
    QPixmap currentPixmap;
    bool scaleCacheEnabled = true;
    QHash<qint64, QRect> scaleCache; // 缓存相同大小的图片位置信息（相同大小的图片都只看同一部分区域）
};

#endif // RESIZABLEPICTURE_H
