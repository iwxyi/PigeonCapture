#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 读取路径
    saveDir = settings.value("path/save").toString();
    if (saveDir.isEmpty())
        saveDir = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    ui->pathEdit->setText(saveDir);

    // 设置快捷键
    fastCaptureShortcut = new QxtGlobalShortcut(this);
    QString skey = settings.value("key/capture", "alt+z").toString();
    ui->fastCaptureEdit->setText(skey);
    setFastShortcut(skey);

    serialCaptureShortcut = new QxtGlobalShortcut(this);
    skey = settings.value("key/serial", "alt+shift+z").toString();
    ui->serialCaptureEdit->setText(skey);
    setSerialShortcut(skey);

    // 截图区域选择器
    areaSelector = new AreaSelector(this);
    if (settings.contains("capture/area"))
        areaSelector->setGeometry(settings.value("capture/area").toRect());
    connect(areaSelector, SIGNAL(areaChanged(QRect)), this, SLOT(areaSelectorMoved(QRect)));

    // 设置截图模式
    int mode = settings.value("capture/mode", 0).toInt();
    ui->modeTab->setCurrentIndex(mode);
    if (mode != FullScreen && settings.value("capture/showAreaSelector").toBool())
    {
        areaSelector->show();
        ui->showAreaSelector->setText("隐藏截图区域");
    }
    else
    {
        areaSelector->hide();
        ui->showAreaSelector->setText("显示截图区域");
    }

    // 提示信息
    tipTimer = new QTimer(this);
    tipTimer->setInterval(2000);
    tipTimer->setSingleShot(0);
    connect(tipTimer, &QTimer::timeout, this, [=]{
        ui->fastCaptureShortcut->setText("设置");
        ui->serialCaptureShortcut->setText("设置");
    });

    // 连续截图
    serialTimer = new QTimer(this);
    int interval = settings.value("serial/interval", 100).toInt();
    serialTimer->setInterval(interval);
    connect(serialTimer, &QTimer::timeout, this, [=]{
        runCapture();
        serialCaptureCount++;
        ui->serialCaptureShortcut->setText("已截" + QString::number(serialCaptureCount) + "张");
        qDebug() << "连续截图：已截" << serialCaptureCount << "张";
    });

    // 单次截图信号槽
    connect(fastCaptureShortcut, &QxtGlobalShortcut::activated,[=]() {
        runCapture();

        ui->fastCaptureShortcut->setText("截图成功");
        tipTimer->start();
    });

    // 连续截图信号槽
    connect(serialCaptureShortcut, &QxtGlobalShortcut::activated,[=]() {
        if (serialTimer->isActive())
        {
            // 停止连续截图
            serialTimer->stop();
            tipTimer->start();
            qDebug() << "停止连续截图";

            ui->selectDirButton->setEnabled(true);
            ui->fastCaptureShortcut->setEnabled(true);
            ui->serialCaptureShortcut->setEnabled(true);
        }
        else
        {
            serialCaptureDir = QDateTime::currentDateTime().toString("yyyy-MM-dd hh-mm-ss-zzz");
            QDir(saveDir).mkdir(serialCaptureDir);

            serialCaptureCount = 0;
            // 开始连续截图
            serialTimer->start();
            // 先截一张
            runCapture();
            qDebug() << "开启连续截图";

            ui->selectDirButton->setEnabled(false);
            ui->fastCaptureShortcut->setEnabled(false);
            ui->serialCaptureShortcut->setEnabled(false);
        }
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

/**
 * 选择截图区域
 */
void MainWindow::selectArea()
{

}

QPixmap MainWindow::getScreenShot()
{
    int mode = ui->modeTab->currentIndex();
    QScreen *screen = QGuiApplication::primaryScreen();
    if (mode == FullScreen) // 全屏截图
    {
        QPixmap pixmap = screen->grabWindow(0);
        return pixmap;
    }
    else if (mode == ScreenArea) // 区域截图
    {
        // 隐藏区域截图
        areaSelector->setPaint(false);
        QRect rect = areaSelector->geometry();

        QPixmap pixmap = screen->grabWindow(QApplication::desktop()->winId(),
                                        rect.left(), rect.top(),
                                       rect.width(), rect.height());

        areaSelector->setPaint(true);
        return pixmap;
    }

    return QPixmap();
}

/**
 * 获取保存的位置
 */
QString MainWindow::getCurrentSavePath()
{
    QString fileName = QDateTime::currentDateTime().toString("yyyy-MM-dd hh-mm-ss-zzz.jpg");
    QDir dir(saveDir);
    if (serialTimer->isActive())
        dir = QDir(dir.filePath(serialCaptureDir));
    QString savePath = dir.filePath(fileName);
    return savePath;
}

/**
 * 开始截图
 */
void MainWindow::runCapture()
{
    QString savePath = getCurrentSavePath();

    QPixmap pixmap = getScreenShot();
    if (pixmap.save(savePath, "jpg"))
    {
        qDebug() << "截图成功：" << savePath;
    }
    else
    {
        qDebug() << "保存失败";
    }
}

void MainWindow::areaSelectorMoved(QRect rect)
{
    showPreview(getScreenShot());
}

/**
 * 设置截图快捷键
 */
void MainWindow::setFastShortcut(QString s)
{
    if (s.isEmpty())
        return ;

    if(fastCaptureShortcut->setShortcut(QKeySequence(s)))
    {
        ui->fastCaptureShortcut->setText("设置成功");
       qDebug() << "设置快速截图快捷键：" << s;
    }
    else
    {
        ui->fastCaptureShortcut->setText("冲突");
       qDebug() << "快捷键设置失败，或许是冲突了" << s;
    }
    if (tipTimer)
        tipTimer->start();
}

void MainWindow::setSerialShortcut(QString s)
{
    if (s.isEmpty())
        return ;

    if(serialCaptureShortcut->setShortcut(QKeySequence(s)))
    {
        ui->serialCaptureShortcut->setText("设置成功");
       qDebug() << "设置连续截图快捷键：" << s;
    }
    else
    {
        ui->serialCaptureShortcut->setText("冲突");
       qDebug() << "快捷键设置失败，或许是冲突了" << s;
    }
    if (tipTimer)
        tipTimer->start();
}

/**
 * 显示预览图
 */
void MainWindow::showPreview(QPixmap pixmap)
{
    if (pixmap.width() > ui->previewLabel->width())
        pixmap = pixmap.scaledToWidth(ui->previewLabel->width());
    ui->previewLabel->setPixmap(pixmap);
}


void MainWindow::on_selectDirButton_clicked()
{
    QString path = QFileDialog::getExistingDirectory(this, "选择保存目录", saveDir);
    if (path.isEmpty())
        return ;

    ui->pathEdit->setText(saveDir = path);
    settings.setValue("path/save", saveDir);
}

void MainWindow::on_fastCaptureShortcut_clicked()
{
    QString s = ui->fastCaptureEdit->text();
    setFastShortcut(s);
    settings.setValue("key/capture", s);
}

void MainWindow::on_serialCaptureShortcut_clicked()
{
    QString s = ui->serialCaptureEdit->text();
    setSerialShortcut(s);
    settings.setValue("key/serial", s);
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);

    showPreview(getScreenShot());
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    settings.setValue("capture/area", areaSelector->geometry());
    areaSelector->deleteLater();

    QMainWindow::closeEvent(event);
}

void MainWindow::on_modeTab_currentChanged(int index)
{
    if (index == FullScreen && !areaSelector->isHidden())
        areaSelector->hide();
    else if (index != FullScreen && settings.value("capture/showAreaSelector", true).toBool())
        areaSelector->show();

    settings.setValue("capture/mode", index);
    showPreview(getScreenShot());
    qDebug() << "设置截图模式：" << index;
}

void MainWindow::on_showAreaSelector_clicked()
{
    if (areaSelector->isHidden())
    {
        areaSelector->show();
        settings.setValue("capture/showAreaSelector", true);
        ui->showAreaSelector->setText("隐藏截图区域");
        qDebug() << "显示区域选择器" << areaSelector->geometry();
    }
    else
    {
        areaSelector->hide();
        settings.setValue("capture/showAreaSelector", false);
        ui->showAreaSelector->setText("显示截图区域");
        settings.setValue("capture/area", areaSelector->geometry());
        qDebug() << "隐藏区域选择器" << areaSelector->geometry();
    }
}
