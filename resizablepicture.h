#ifndef RESIZABLEPICTURE_H
#define RESIZABLEPICTURE_H

#include <QObject>
#include <QWidget>
#include <QLabel>
#include <QWheelEvent>
#include <QMouseEvent>

class ResizablePicture : public QWidget
{
    Q_OBJECT
public:
    ResizablePicture(QWidget *parent = nullptr);

    struct ScaleInfo
    {
        ScaleInfo(){}
        ScaleInfo(double s, QPointF o) : scale(s), offset(o){}
        double scale = 1.0; // 缩放的百分比
        QPointF offset = QPointF(0, 0); // 中心点的偏移
    };

    void setPixmap(const QPixmap& pixmap);
    void resetScale();
    void restoreScale(ScaleInfo scale);

    void setScaleCache(bool enable);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void resizeEvent(QResizeEvent *) override;

    qint64 sizeToLL(QSize size);

signals:

public slots:

private:
    QLabel* label;

    QPixmap originPixmap;
    QPixmap currentPixmap;
    bool scaleCacheEnabled = true;
    QHash<qint64, ScaleInfo> scaleCache; // 缓存相同大小的图片位置信息（相同大小的图片都只看同一部分区域）
};

#endif // RESIZABLEPICTURE_H
