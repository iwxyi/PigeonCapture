#include "resizablepicture.h"

ResizablePicture::ResizablePicture(QWidget *parent) : QWidget(parent)
{
    label = new QLabel(this);
    label->show();
}

void ResizablePicture::setPixmap(const QPixmap &pixmap)
{
    // 保存原先的位置
    if (!originPixmap.isNull())
    {
        ScaleInfo info;
        int widthHint = qMin(this->width(), originPixmap.width());
        info.scale = label->width() / widthHint;

        info.offset = label->geometry().center() - QRect(0,0,width(),height()).center();
        scaleCache[sizeToLL(originPixmap.size())] = info;
    }

    // 加载图片
    originPixmap = pixmap;
    currentPixmap = pixmap.scaled(label->size());
    label->setPixmap(currentPixmap);

    // 恢复相同的位置
    if (scaleCacheEnabled)
    {
        QSize size = pixmap.size();
        if (scaleCache.contains(sizeToLL(size)))
        {
            ScaleInfo scale = scaleCache.value(sizeToLL(size));
            restoreScale(scale);
        }
    }
}

void ResizablePicture::resetScale()
{
    restoreScale(ScaleInfo(1, QPointF()));
}

void ResizablePicture::restoreScale(ResizablePicture::ScaleInfo scale)
{
    label->resize(static_cast<int>(width()*scale.scale),
                  static_cast<int>(height()*scale.scale));
    label->move(static_cast<int>(width()/2 + width() * scale.offset.x() - label->width()/2),
                static_cast<int>(height()/2 + height() * scale.offset.y() - label->height()/2));
}

void ResizablePicture::setScaleCache(bool enable)
{
    this->scaleCacheEnabled = enable;
}

void ResizablePicture::wheelEvent(QWheelEvent *event)
{
    QPoint pos = event->pos();

    // 根据不同的位置进行缩放
}

void ResizablePicture::mouseMoveEvent(QMouseEvent *)
{

}

void ResizablePicture::resizeEvent(QResizeEvent *)
{
    // 清理全部缓存
    scaleCache.clear();
}

qint64 ResizablePicture::sizeToLL(QSize size)
{
    return size.width() * 1000000 + size.height();
}
