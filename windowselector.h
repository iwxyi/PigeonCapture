#ifndef WINDOWSELECTOR_H
#define WINDOWSELECTOR_H

#include <QDialog>
#include <QPainter>
#include <QKeyEvent>
#include <QApplication>
#include <QScreen>
#include <QDesktopWidget>
#include <QDebug>
#include <QPropertyAnimation>
#include <QLabel>
#include "windowshwnd.h"

class WindowSelector : public QDialog
{
public:
    static QRect getArea()
    {
        QRect rect(1,1,2,2);
        auto selector = new WindowSelector(&rect);
        selector->exec();
        return rect;
    }

protected:
    WindowSelector(QRect* p_result) : QDialog(nullptr), p_result(p_result)
    {
        this->setWindowTitle("选择窗口区域");
        this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);      //设置为无边框置顶窗口
        this->setAttribute(Qt::WA_TranslucentBackground, true); // 设置窗口透明
        this->setAttribute(Qt::WA_DeleteOnClose, true); // 自动删除
        this->setMouseTracking(true); // 监听鼠标位置
        this->setWindowModality(Qt::WindowModal);

        borderLabel = new QLabel(this);
        borderLabel->setStyleSheet("background: #2F4F4F64; border: 3px solid #F08080; color: black; font: 20px;");
        borderLabel->setMouseTracking(false);
        borderLabel->setAlignment(Qt::AlignCenter);
        borderLabel->resize(1, 1);
        borderLabel->move(QCursor::pos());

        this->grabMouse();
        this->grabKeyboard();
    }

    struct WindowInfo
    {
        QString title;
        QRect geometry;
        HWND hwnd;
    };

    void showEvent(QShowEvent *) override
    {
        // 获取显示器的所有位置
        auto screens = QGuiApplication::screens();
        int minLeft = 0;
        int minTop = 0;
        int maxRight = 0;
        int maxBottom = 0;
        foreach (auto screen, screens)
        {
            auto geom = screen->geometry();
            if (geom.left() < minLeft)
                minLeft = geom.left();
            if (geom.top() < minTop)
                minTop = geom.top();
            if (geom.right() > maxRight)
                maxRight = geom.right();
            if (geom.bottom() > maxBottom)
                maxBottom = geom.bottom();
        }
        // 固定自己的位置，强制全屏置顶
        setGeometry(minLeft, minTop, maxRight, maxBottom);

        // 黑名单
        QStringList blacks{"", "选择截图区域", "选择窗口区域", "Snipaste"};

        // 获取所有窗口信息
        HWND pWnd = first_window(EXCLUDE_MINIMIZED); // 得到第一个窗口句柄
        while (pWnd)
        {
            QString title = get_window_title(pWnd);
            if (!blacks.contains(title)) // 最上面的肯定是自己窗口
            {
                RECT rect;
                ::GetWindowRect(pWnd, &rect);
                QRect qr(rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top);
                screenWindows.append(WindowInfo{title, qr, pWnd});
            }

            pWnd = next_window(pWnd, EXCLUDE_MINIMIZED); // 得到下一个窗口句柄
        }
    }

    void keyPressEvent(QKeyEvent *event) override
    {
        auto key = event->key();
        if (key == Qt::Key_Escape)
        {
            *p_result = QRect();
            this->hide();
        }
        else if (key == Qt::Key_Return || key == Qt::Key_Enter)
        {
            this->hide();
        }

        QDialog::keyPressEvent(event);
    }

    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton)
        {
            pressPos = event->pos();

            if (movingAni)
            {
                borderLabel->setGeometry(currentRect);
                movingAni->deleteLater();
                movingAni = nullptr;
            }
        }
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        if (event->buttons() & Qt::LeftButton) // 拖拽矩形区域
        {
            borderLabel->setCursor(Qt::CrossCursor);
            QPoint pos1 = pressPos;
            QPoint pos2 = event->pos();
            int minX = qMin(pos1.x(), pos2.x());
            int minY = qMin(pos1.y(), pos2.y());
            int maxX = qMax(pos1.x(), pos2.x());
            int maxY = qMax(pos1.y(), pos2.y());
            currentRect = QRect(minX, minY, maxX - minX, maxY - minY);
            borderLabel->setGeometry(currentRect);
            borderLabel->setText("");
            borderLabel->show();
            return ;
        }
        else // 显示位置
        {
            QPoint pos = event->pos();
            for (int i = 0; i < screenWindows.size(); i++)
            {
                QRect rect = screenWindows.at(i).geometry;
                if (rect.contains(pos))
                {
                    borderLabel->setText(screenWindows.at(i).title);
                    if (currentRect == rect)
                    {
                    }
                    if (!borderLabel->isHidden())
                    {
                        if (movingAni)
                            movingAni->deleteLater();
                        movingAni = new QPropertyAnimation(borderLabel, "geometry");
                        movingAni->setStartValue(borderLabel->geometry());
                        movingAni->setEndValue(rect);
                        movingAni->setDuration(300);
                        movingAni->setEasingCurve(QEasingCurve::OutCubic);
                        connect(movingAni, &QPropertyAnimation::finished, this, [=]{
                            movingAni->deleteLater();
                            movingAni = nullptr;
                        });
                        movingAni->start();
                    }
                    else // 隐藏了，直接设置大小
                    {
                        borderLabel->setGeometry(rect);
                    }
                    currentRect = rect;
                    borderLabel->show();
                    borderLabel->setCursor(Qt::PointingHandCursor);
                    return ;
                }
            }
        }

        // 没找到窗口
        borderLabel->hide();
        borderLabel->setText("");
        currentRect = QRect();
    }

    void mouseReleaseEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton)
        {
            if ( (event->pos() - pressPos).manhattanLength() < QApplication::startDragDistance() )
            {
                *p_result = currentRect;
                this->hide();
            }
            else // 拖动了
            {
                // 想了想没啥操作，暂且一样是确定吧
                *p_result = currentRect;
                this->hide();
            }
        }
        else if (event->button() == Qt::RightButton) // 取消
        {
            *p_result = QRect();
            this->hide();
        }
    }

    void paintEvent(QPaintEvent* ) override
    {
        QPainter painter(this);
        painter.fillRect(this->rect(), QColor(112, 128, 144, 0x32));
    }

private:
    QString get_window_title(HWND hwnd)
    {
        QString retStr;
        wchar_t *temp;
        int len;

        len = GetWindowTextLengthW(hwnd);
        if (!len)
            return retStr;

        temp = (wchar_t *)malloc(sizeof(wchar_t) * (len+1));
        if (GetWindowTextW(hwnd, temp, len+1))
        {
            retStr = QString::fromWCharArray(temp);
        }
        free(temp);
        return retStr;
    }

    QString get_window_class(HWND hwnd)
    {
        QString retStr;
        wchar_t temp[256];

        temp[0] = 0;
        if (GetClassNameW(hwnd, temp, sizeof(temp) / sizeof(wchar_t)))
        {
            retStr = QString::fromWCharArray(temp);
        }
        return retStr;
    }

private:
    QPoint pressPos;
    QList<WindowInfo> screenWindows;
    QRect currentRect;
    QLabel* borderLabel;
    QRect* p_result;
    QPropertyAnimation* movingAni = nullptr;
};

#endif // WINDOWSELECTOR_H
