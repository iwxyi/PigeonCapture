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
        int heightHint = qMin(this->height(), originPixmap.height());
        double wScale = (double)label->width() / widthHint;
        double hScale = (double)label->height() / heightHint;
        info.scale = qMax(wScale, hScale);
//        qDebug() << ">>>>>缩放：" << info.scale << widthHint << label->width() << heightHint << label->height();

        QPointF iPos = label->geometry().center();
        QPointF oPos = QRect(0,0,width(),height()).center();
        qDebug() << "偏移" << iPos << oPos;
        info.offset = QPointF(iPos.x() / oPos.x() - 1, iPos.y() / oPos.y() - 1);
        scaleCache[sizeToLL(originPixmap.size())] = info;
        qDebug() << "保存缩放：" << originPixmap.size() << info.toString();
    }

    // 加载图片
    currentPixmap = originPixmap = pixmap;
    if (pixmap.isNull())
        return restoreScale(ScaleInfo(1.0, QPoint(0,0)));

    // 缩小图片大小
    if (pixmap.width() > width() || originPixmap.height() >= height())
        currentPixmap = originPixmap.scaled(label->size(), Qt::AspectRatioMode::KeepAspectRatio);

    // label缩小到pixmap的大小
    label->setFixedSize(currentPixmap.size());
    qDebug() << "setLabel = pixmap.size" << label->size();
    label->move((width()-label->width())/2, (height()-label->height())/2);
    label->setPixmap(currentPixmap);

    // 恢复相同的位置
    if (scaleCacheEnabled)
    {
        QSize size = pixmap.size();
        if (scaleCache.contains(sizeToLL(size)))
        {
            ScaleInfo scale = scaleCache.value(sizeToLL(size));
            restoreScale(scale);
            qDebug() << "还原位置" << size << scale.toString();
        }
    }
}

void ResizablePicture::resetScale()
{
    restoreScale(ScaleInfo(1, QPointF(0, 0)));
}

void ResizablePicture::restoreScale(ResizablePicture::ScaleInfo scale)
{
    if (originPixmap.isNull())
        return ;
    int widthHint = qMin(this->width(), originPixmap.width());
    int heightHint = qMin(this->height(), originPixmap.height());
    label->setFixedSize(static_cast<int>(widthHint*scale.scale),
                  static_cast<int>(heightHint*scale.scale));
    label->move(static_cast<int>((width()-label->width())/2 + width() * scale.offset.x()+0.5),
                static_cast<int>((height()-label->height())/2 + height() * scale.offset.y()+0.5));
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
    // 修改图片大小
    label->setGeometry(0, 0, width(), height());
    if (originPixmap.width() > width() || originPixmap.height() >= height())
        currentPixmap = originPixmap.scaled(label->size(), Qt::AspectRatioMode::KeepAspectRatio);

    // label缩小到pixmap的大小
    label->resize(currentPixmap.size());
    label->move((width()-label->width())/2, (height()-label->height())/2);
    label->setPixmap(currentPixmap);
}

qint64 ResizablePicture::sizeToLL(QSize size)
{
    return size.width() * 1000000 + size.height();
}
