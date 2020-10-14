#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
qDebug() << settings.fileName();
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
        ui->fastCaptureShortcut->setText("开始");
        ui->serialCaptureShortcut->setText("开始");
    });

    // 连续截图
    serialTimer = new QTimer(this);
    int interval = settings.value("serial/interval", 100).toInt();
    ui->spinBox->setValue(interval);
    serialTimer->setInterval(interval);
    connect(serialTimer, &QTimer::timeout, this, [=]{
        runCapture();
        serialCaptureCount++;
        ui->serialCaptureShortcut->setText("已截" + QString::number(serialCaptureCount) + "张");
        qDebug() << "连续截图：已截" << serialCaptureCount << "张";
    });

    // 单次截图信号槽
    connect(fastCaptureShortcut, &QxtGlobalShortcut::activated,[=]() {
        triggerFastCapture();
    });

    // 连续截图信号槽
    connect(serialCaptureShortcut, &QxtGlobalShortcut::activated,[=]() {
        triggerSerialCapture();
    });

    // 获取显示器的信息
    QDesktopWidget * desktop = QApplication::desktop();
    int monitorCount = desktop->screenCount();
    int currentMonitor = desktop->screenNumber(this);
    for (int i = 0; i < monitorCount; i++)
    {
        QRect rect = desktop->screenGeometry(i);
        QString name = QString("%1 (%2,%3 %4×%5)")
                .arg(i).arg(rect.left()).arg(rect.top()).arg(rect.width()).arg(rect.height());
        ui->comboBox->addItem(name);
    }
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
        QPixmap pixmap = screen->grabWindow(ui->comboBox->currentIndex());
        return pixmap;
    }
    else if (mode == ScreenArea) // 区域截图
    {
        // 隐藏区域截图
        areaSelector->setPaint(false);
        QRect rect = areaSelector->getArea();

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
    QString fileName = QDateTime::currentDateTime().toString("yyyy-MM-dd hh-mm-ss.zzz.jpg");
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

void MainWindow::triggerFastCapture()
{
    runCapture();

    ui->fastCaptureShortcut->setText("截图成功");
    tipTimer->start();
}

void MainWindow::triggerSerialCapture()
{
    if (serialTimer->isActive())
    {
        // 停止连续截图
        serialTimer->stop();
        tipTimer->start();
        qDebug() << "停止连续截图";

        ui->selectDirButton->setEnabled(true);
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
        if (tipTimer)
            ui->fastCaptureShortcut->setText("设置成功");
       qDebug() << "设置快速截图快捷键：" << s;
    }
    else
    {
        if (tipTimer)
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
        if (tipTimer)
            ui->serialCaptureShortcut->setText("设置成功");
       qDebug() << "设置连续截图快捷键：" << s;
    }
    else
    {
        if (tipTimer)
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
    if (pixmap.width() > ui->previewLabel->width() || pixmap.height() > ui->previewLabel->height())
        pixmap = pixmap.scaled(ui->previewLabel->size(), Qt::KeepAspectRatio);
    ui->previewLabel->setPixmap(pixmap);
    ui->previewLabel->setMinimumSize(1, 1);
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
    triggerFastCapture();
}

void MainWindow::on_serialCaptureShortcut_clicked()
{
    triggerSerialCapture();
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);

    restoreGeometry(settings.value("mainwindow/geometry").toByteArray());
    restoreState(settings.value("mainwindow/state").toByteArray());

    showPreview(getScreenShot());
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    settings.setValue("capture/area", areaSelector->geometry());
    areaSelector->deleteLater();
    settings.setValue("mainwindow/geometry", this->saveGeometry());
    settings.setValue("mainwindow/state", this->saveState());

    QMainWindow::closeEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    showPreview(getScreenShot());
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

void MainWindow::on_actionOpen_Directory_triggered()
{
    QDesktopServices::openUrl(QUrl("file:///" + saveDir, QUrl::TolerantMode));
}

void MainWindow::on_actionCapture_History_triggered()
{
    PictureBrowser* pb = new PictureBrowser(this);
    pb->readDirectory(saveDir);
    pb->show();
}

void MainWindow::on_fastCaptureEdit_textEdited(const QString &arg1)
{
    setFastShortcut(arg1);
    settings.setValue("key/capture", arg1);
}

void MainWindow::on_serialCaptureEdit_textEdited(const QString &arg1)
{
    setSerialShortcut(arg1);
    settings.setValue("key/serial", arg1);
}

void MainWindow::on_spinBox_valueChanged(int arg1)
{
    settings.setValue("serial/interval", arg1);
    serialTimer->setInterval(arg1);
}

void MainWindow::on_comboBox_activated(int index)
{
    QDesktopWidget * desktop = QApplication::desktop();
    QRect rect = desktop->screenGeometry(index);
}

void MainWindow::on_actionRestore_Geometry_triggered()
{
    settings.remove("capture/area");
    settings.remove("mainwindow/geometry");
    settings.remove("mainwindow/state");
    settings.remove("picturebrowser/geometry");
    settings.remove("picturebrowser/state");
    settings.remove("picturebrowser/splitGeometry");
    settings.remove("picturebrowser/splitState");
    areaSelector->deleteLater();
    areaSelector = new AreaSelector(nullptr);
}
