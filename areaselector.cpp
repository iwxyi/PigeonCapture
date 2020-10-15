#include "areaselector.h"

AreaSelector::AreaSelector(QWidget *)
    : QWidget(nullptr)
{
    this->setWindowTitle("选择截图区域");
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);      //设置为无边框置顶窗口
    this->setMinimumSize(45,45);                        //设置最小尺寸
    this->setAttribute(Qt::WA_TranslucentBackground, true); // 设置窗口透明

    QFontMetrics fm(this->font());
    fontHeight = fm.height();
}

void AreaSelector::setPaint(bool paint)
{
    this->shouldPaint = paint;
    repaint();
}

QRect AreaSelector::getArea()
{
    return QRect(x() + boundaryWidth, y() + fontHeight + boundaryWidth, width()-boundaryWidth*2, height()-fontHeight-boundaryWidth*2);
}

void AreaSelector::setArea(QRect rect)
{
    this->setGeometry(rect);
}

bool AreaSelector::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
    Q_UNUSED(eventType)
    MSG* msg = static_cast<MSG*>(message);
    switch(msg->message)
    {
    case WM_NCHITTEST:
        const auto ratio = devicePixelRatioF(); // 解决4K下的问题
        int xPos = static_cast<int>(GET_X_LPARAM(msg->lParam) / ratio - this->frameGeometry().x());
        int yPos = static_cast<int>(GET_Y_LPARAM(msg->lParam) / ratio - this->frameGeometry().y());
        if(xPos < boundaryWidth && yPos>=fontHeight&&yPos<boundaryWidth+fontHeight)                    //左上角
            *result = HTTOPLEFT;
        else if(xPos>=width()-boundaryWidth&&yPos>=fontHeight&&yPos<boundaryWidth+fontHeight)          //右上角
            *result = HTTOPRIGHT;
        else if(xPos<boundaryWidth&&yPos>=height()-boundaryWidth)         //左下角
            *result = HTBOTTOMLEFT;
        else if(xPos>=width()-boundaryWidth&&yPos>=height()-boundaryWidth)//右下角
            *result = HTBOTTOMRIGHT;
        else if(xPos < boundaryWidth)                                     //左边
            *result =  HTLEFT;
        else if(xPos>=width()-boundaryWidth)                              //右边
            *result = HTRIGHT;
        else if(yPos>=fontHeight&&yPos<boundaryWidth+fontHeight)                                       //上边
            *result = HTTOP;
        else if(yPos>=height()-boundaryWidth)                             //下边
            *result = HTBOTTOM;
        else              //其他部分不做处理，返回false，留给其他事件处理器处理
           return false;
        return true;
    }
    return false;         //此处返回false，留给其他事件处理器处理
}

void AreaSelector::mousePressEvent(QMouseEvent *e)
{
    if(e->button()==Qt::LeftButton)
        clickPos=e->pos();
}
void AreaSelector::mouseMoveEvent(QMouseEvent *e)
{
    if(e->buttons()&Qt::LeftButton)
    {
        move(e->pos()+pos()-clickPos);
        emit areaChanged(geometry());
    }
}

void AreaSelector::resizeEvent(QResizeEvent *)
{
    QTimer::singleShot(0, [=]{
        emit areaChanged(geometry());
    });
}

void AreaSelector::paintEvent(QPaintEvent *)
{
    QColor c(30, 144, 255, 192);
    int penW = boundaryShowed;
    QPainter painter(this);

    // 绘制用来看的边框
    painter.setPen(QPen(c, penW/2, Qt::PenStyle::DashDotLine));
    painter.drawRect(boundaryWidth-boundaryShowed/2, fontHeight + boundaryWidth-boundaryShowed/2, width()-boundaryWidth*2+boundaryShowed, height()-fontHeight-boundaryWidth*2+boundaryShowed);

    // 绘制坐标文字
    QRect g = getArea();
    painter.drawText(penW, fontHeight,
                     QString("(%1,%2) %3×%4").arg(g.left()).arg(g.top()).arg(g.width()).arg(g.height()));

    // 绘制用来拖动的边框
    penW = 32;
    c.setAlpha(1);
    painter.setPen(QPen(c, penW/2, Qt::PenStyle::DashDotLine));
    painter.drawRect(boundaryWidth/2, fontHeight-boundaryWidth/2, width()-boundaryWidth, height()-fontHeight-boundaryWidth);

    // 绘制顶部可移动的边框
    painter.drawRect(0, 0, width(), fontHeight);
}
