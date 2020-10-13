#include "areaselector.h"

AreaSelector::AreaSelector(QWidget *parent)
    : QWidget(nullptr)
{
    boundaryWidth=8;                                    //设置触发resize的宽度
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);      //设置为无边框置顶窗口
    this->setMinimumSize(45,45);                        //设置最小尺寸
    this->setStyleSheet("background:#D1EEEE");          //设置背景颜色
    this->setAttribute(Qt::WA_TranslucentBackground, true); // 设置窗口透明
}

void AreaSelector::setPaint(bool paint)
{
    this->shouldPaint = paint;
    repaint();
}

bool AreaSelector::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
    MSG* msg = (MSG*)message;
    switch(msg->message)
    {
    case WM_NCHITTEST:
        const auto ratio = devicePixelRatioF(); // 解决4K下的问题
        int xPos = GET_X_LPARAM(msg->lParam) / ratio - this->frameGeometry().x();
        int yPos = GET_Y_LPARAM(msg->lParam) / ratio - this->frameGeometry().y();
        if(xPos < boundaryWidth && yPos<boundaryWidth)                    //左上角
            *result = HTTOPLEFT;
        else if(xPos>=width()-boundaryWidth&&yPos<boundaryWidth)          //右上角
            *result = HTTOPRIGHT;
        else if(xPos<boundaryWidth&&yPos>=height()-boundaryWidth)         //左下角
            *result = HTBOTTOMLEFT;
        else if(xPos>=width()-boundaryWidth&&yPos>=height()-boundaryWidth)//右下角
            *result = HTBOTTOMRIGHT;
        else if(xPos < boundaryWidth)                                     //左边
            *result =  HTLEFT;
        else if(xPos>=width()-boundaryWidth)                              //右边
            *result = HTRIGHT;
        else if(yPos<boundaryWidth)                                       //上边
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

void AreaSelector::resizeEvent(QResizeEvent *event)
{
    emit areaChanged(geometry());
}

void AreaSelector::paintEvent(QPaintEvent *event)
{
    if (!shouldPaint)
        return ;

    QColor c(30, 144, 255, 192);
    int penW = 4;
    QPainter painter(this);

    // 绘制用来看的边框
    painter.setPen(QPen(c, penW/2, Qt::PenStyle::DashDotLine));
    painter.drawRect(0, 0, width(), height());

    QRect g = geometry();
    painter.drawText(QRect(penW, penW, width()-penW, height()-penW), Qt::AlignLeft|Qt::AlignTop,
                     QString("(%1,%2) %3×%4").arg(g.left()).arg(g.top()).arg(g.width()).arg(g.height()));

    // 绘制用来拖动的边框
    penW = 32;
    c.setAlpha(1);
    painter.setPen(QPen(c, penW/2, Qt::PenStyle::DashDotLine));
    painter.drawRect(0, 0, width(), height());

    // 绘制用来移动的中心点
    QPainterPath path;
    const int radius = 4;
    c.setAlpha(64);
    path.addEllipse(width()/2-radius,height()/2-radius, radius*2, radius*2);
    painter.setBrush(c);
    painter.setPen(QPen(c, 0));
    painter.drawPath(path);
}
