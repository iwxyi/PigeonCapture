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

    // 设置截图模式
    int mode = settings.value("capture/mode", 0).toInt();
    ui->modeTab->setCurrentIndex(mode);
    areaSelector = new AreaSelector(this);
    areaSelector->hide();

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

    // 设置截图信号槽
    connect(fastCaptureShortcut, &QxtGlobalShortcut::activated,[=]() {
        runCapture();

        ui->fastCaptureShortcut->setText("截图成功");
    });

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
    if (mode == FullScreen) // 全屏截图
    {
        QScreen *screen = QGuiApplication::primaryScreen();
        QPixmap pixmap = screen->grabWindow(0);
        return pixmap;
    }
    else if (mode == ScreenArea) // 区域截图
    {

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

/**
 * 设置截图快捷键
 */
void MainWindow::setFastShortcut(QString s)
{
    if (s.isEmpty())
        return ;

    if(fastCaptureShortcut->setShortcut(QKeySequence(s)))
    {
       qDebug() << "设置快捷键成功：" << s;
    }
    else
    {
       qDebug() << "快捷键设置失败，或许是冲突了";
    }
}

void MainWindow::setSerialShortcut(QString s)
{
    if (s.isEmpty())
        return ;

    if(serialCaptureShortcut->setShortcut(QKeySequence(s)))
    {
       qDebug() << "设置快捷键成功：" << s;
    }
    else
    {
       qDebug() << "快捷键设置失败，或许是冲突了";
    }
}

/**
 * 显示预览图
 */
void MainWindow::showPreview(QPixmap pixmap)
{
    ui->previewLabel->setPixmap(pixmap.scaledToWidth(ui->previewLabel->width()));
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


}

void MainWindow::on_modeTab_currentChanged(int index)
{
    qDebug() << "设置截图模式：" << index;
    settings.setValue("capture/mode", index);
    showPreview(getScreenShot());
}
