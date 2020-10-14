#include "resizablepicture.h"

ResizablePicture::ResizablePicture(QWidget *parent) : QWidget(parent)
{
    label = new QLabel(this);
    label->show();
    resetScale();
}

bool ResizablePicture::setPixmap(const QPixmap &pixmap)
{
    // 保存原先的位置
    if (!originPixmap.isNull())
    {
       scaleCache[sizeToLL(originPixmap.size())] = label->geometry();
    }

    // 加载图片
    currentPixmap = originPixmap = pixmap;
    if (originPixmap.isNull())
        return false;


    // 缩小大图片大小
    if (originPixmap.width() > width() || originPixmap.height() >= height())
        currentPixmap = originPixmap.scaled(size(), Qt::AspectRatioMode::KeepAspectRatio);

    // label缩小到pixmap的大小
    label->resize(currentPixmap.size());
    label->move((width()-label->width())/2, (height()-label->height())/2);
    label->setPixmap(currentPixmap);

    // 恢复相同的位置
    if (scaleCacheEnabled)
    {
        QSize size = originPixmap.size();
        if (scaleCache.contains(sizeToLL(size)))
        {
            label->setGeometry(scaleCache.value(sizeToLL(size)));
        }
    }

    return true;
}

void ResizablePicture::resetScale()
{
    if (originPixmap.isNull())
    {
        label->setGeometry(0, 0, width(), height());
        return ;
    }

    // 缩小大图片大小
    if (originPixmap.width() > width() || originPixmap.height() >= height())
        currentPixmap = originPixmap.scaled(size(), Qt::AspectRatioMode::KeepAspectRatio);

    // label缩小到pixmap的大小
    label->resize(currentPixmap.size());
    label->move((width()-label->width())/2, (height()-label->height())/2);
    label->setPixmap(currentPixmap);
}

void ResizablePicture::restoreScale()
{
    if (originPixmap.isNull())
        return ;

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

    if (originPixmap.isNull())
        return ;

    resetScale();
}

qint64 ResizablePicture::sizeToLL(QSize size)
{
    return size.width() * 1000000 + size.height();
}
