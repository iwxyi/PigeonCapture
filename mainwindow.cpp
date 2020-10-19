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
    connect(areaSelector, SIGNAL(areaChanged()), this, SLOT(areaSelectorMoved()));
    connect(areaSelector, SIGNAL(toHide()), this, SLOT(on_showAreaSelector_clicked()));
    connect(areaSelector, SIGNAL(toSelectWindow()), this, SLOT(on_selectScreenWindow_clicked()));

    // 设置截图模式
    int mode = settings.value("capture/mode", 0).toInt();
    ui->modeTab->setCurrentIndex(mode);
    if (mode == ScreenArea && settings.value("capture/showAreaSelector").toBool())
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
        if (!serialTimer->isActive())
            ui->serialCaptureShortcut->setText("开始");
    });

    // 连续截图
    serialTimer = new QTimer(this);
    int interval = settings.value("serial/interval", 100).toInt();
    serialTimer->setInterval(interval);
    connect(serialTimer, &QTimer::timeout, this, [=]{
        QString fileName = timeToFile() + "." + saveMode;
        QDir dir(saveDir);
        dir = QDir(dir.filePath(serialCaptureDir));
        QString savePath = dir.filePath(fileName);

        try {
            QPixmap pixmap = getScreenShot();
            pixmap.save(savePath, saveMode.toLocal8Bit());
        } catch (...) {
            qDebug() << "截图失败，可能是内存不足";
        }

        serialCaptureCount++;
        ui->serialCaptureShortcut->setText("已截" + QString::number(serialCaptureCount) + "张");
    });

    // 预先截图
    prevTimer = new QTimer(this);
    prevTimer->setInterval(interval);
    ui->spinBox->setValue(interval);
    connect(prevTimer, &QTimer::timeout, this, [=]{
        if (!prevCapturedList) // 可能是多线程冲突
            return ;
        qint64 timestamp = getTimestamp();
        bool failed = false;
        try {
            prevCapturedList->append(CaptureInfo{timestamp,
                                                 timeToFile(),
                                                 new QPixmap(getScreenShot())});
        } catch (...) {
            failed = true;

            // 强制删除一张，空出内存
            if (prevCapturedList->size())
            {
                delete prevCapturedList->takeFirst().pixmap;
            }
        }

        while (prevCapturedList->size())
        {
            if (prevCapturedList->first().time + prevCaptureMaxTime < timestamp)
                delete prevCapturedList->takeFirst().pixmap;
            else
                break;
        }
        ui->prevCaptureCheckBox->setText(QString("已有%1张(%2s)%3")
                                         .arg(prevCapturedList->size())
                                         .arg((timestamp-prevCapturedList->first().time)/1000)
                                         .arg(failed ? " 内存不足" : ""));
    });
    if (settings.value("capture/prev", false).toBool())
    {
        ui->prevCaptureCheckBox->setChecked(true);
    }

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
    auto screens = QGuiApplication::screens();
    int monitorCount = screens.size();
    int currentMonitor = desktop->screenNumber(this);
    ui->screensCombo->clear();
    for (int i = 0; i < monitorCount; i++)
    {
        QRect rect = screens.at(i)->geometry();
        QString name = QString("%1 (%2,%3 %4×%5)")
                .arg(i).arg(rect.left()).arg(rect.top()).arg(rect.width()).arg(rect.height());
        ui->screensCombo->addItem(name);
    }
    ui->screensCombo->setCurrentIndex(currentMonitor);

    if (mode == OneWindow)
    {
        on_modeTab_currentChanged(mode);
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
        QPixmap pixmap = screen->grabWindow(ui->screensCombo->currentIndex());
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
    else if (mode == OneWindow) // 窗口截图
    {
        if (!currentHwnd)
            return QPixmap();
        try {
            QScreen *screen = QGuiApplication::primaryScreen();
            QPixmap map = screen->grabWindow(reinterpret_cast<WId>(currentHwnd)); // 参数0表示全屏
            return map;
        } catch (...) {
            qDebug() << "窗口截图失败";
        }
    }

    return QPixmap();
}

/**
 * 开始截图
 */
void MainWindow::runCapture()
{
    QString fileName = timeToFile() + "." + saveMode;
    QString savePath = QDir(saveDir).filePath(fileName);

    try {
        QPixmap pixmap = getScreenShot();
        if (pixmap.save(savePath, saveMode.toLocal8Bit()))
        {
            qDebug() << "截图成功：" << savePath;
        }
        else
        {
            qDebug() << "保存失败" << savePath;
        }
    } catch (...) {
        qDebug() << "截图失败，可能是内存不足";
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
        serialCaptureDir = "连"+timeToFile();
        QDir(saveDir).mkdir(serialCaptureDir);
        QDir currentDir = QDir(saveDir).absoluteFilePath(serialCaptureDir);

        // 保存录制参数
        QSettings params(currentDir.absoluteFilePath(SEQUENCE_PARAM_FILE), QSettings::IniFormat);
        params.setValue("gif/interval", prevTimer->interval());
        params.sync();

        serialCaptureCount = 0;
        // 开始连续截图
        serialTimer->start();
        // 先截一张
        runCapture();
        qDebug() << "开启连续截图";

        ui->selectDirButton->setEnabled(false);
    }
}

/**
 * 开启预先截图
 */
void MainWindow::startPrevCapture()
{
    if (prevCapturedList)
    {
        clearPrevCapture();
    }

    prevTimer->start();
    prevCapturedList = new QList<CaptureInfo>();
}

/**
 * 保存一段时间之前的预先截图
 * 所有的截图都将转移至另一线程，超出指定时间的截图都会舍弃掉
 * 接着会重新预先截图
 */
void MainWindow::savePrevCapture(qint64 delta)
{
    auto list = this->prevCapturedList;
    this->prevCapturedList = nullptr;
    startPrevCapture(); // 重新开始一轮新的
    if (!list)
        return ;
    qint64 currentTime = getTimestamp();
    QtConcurrent::run([=]{
        QString dirName = timeToFile();
        // 获取保存的路径
        QDir rootDir(saveDir);
        QDir saveDir(rootDir.absoluteFilePath("预"+dirName));
        saveDir.mkdir(saveDir.absolutePath());

        // 保存录制参数
        QSettings params(saveDir.absoluteFilePath(SEQUENCE_PARAM_FILE), QSettings::IniFormat);
        params.setValue("gif/interval", prevTimer->interval());
        params.sync();

        // 计算要保存的起始位置
        int maxSize = list->size();
        int start = maxSize;
        while (start > 0 && list->at(start-1).time + delta >= currentTime)
            start--;

        // 清理无用的
        for (int i = 0; i < start; i++)
            delete list->at(i).pixmap;

        // 开始保存
        for (int i = start; i < maxSize; i++)
        {
            auto cap = list->at(i);
            cap.pixmap->save(saveDir.absoluteFilePath(cap.name + "." + saveMode), saveMode.toLocal8Bit());
            delete cap.pixmap;
        }
        qDebug() << "已保存" << (maxSize-start) << "张预先截图";
        delete list;
    });
}

void MainWindow::clearPrevCapture()
{
    prevTimer->stop();
    if (prevCapturedList)
    {
        qDebug() << "清理" << prevCapturedList->size() << "张截图";
        foreach (auto cap, *prevCapturedList)
        {
            delete cap.pixmap;
        }
        delete prevCapturedList;
        prevCapturedList = nullptr;
    }
}

void MainWindow::areaSelectorMoved()
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
    }
    else
    {
        if (tipTimer)
            ui->fastCaptureShortcut->setText("冲突");
       qDebug() << "快速截图快捷键设置失败，或许是冲突了" << s;
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
    }
    else
    {
        if (tipTimer)
            ui->serialCaptureShortcut->setText("冲突");
       qDebug() << "连续截图快捷键设置失败，或许是冲突了" << s;
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
    clearPrevCapture();

    settings.setValue("capture/area", areaSelector->geometry());
    areaSelector->deleteLater();
    settings.setValue("mainwindow/geometry", this->saveGeometry());
    settings.setValue("mainwindow/state", this->saveState());

    QMainWindow::closeEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent *)
{
    showPreview(getScreenShot());
}

QString MainWindow::timeToFile()
{
    return QDateTime::currentDateTime().toString("yyyy-MM-dd hh-mm-ss.zzz");
}

qint64 MainWindow::getTimestamp()
{
    return QDateTime::currentDateTime().toMSecsSinceEpoch();
}

QString MainWindow::get_window_title(HWND hwnd)
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

QString MainWindow::get_window_class(HWND hwnd)
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

void MainWindow::on_modeTab_currentChanged(int index)
{
    if (index != ScreenArea && !areaSelector->isHidden())
        areaSelector->hide();
    else if (index == ScreenArea && settings.value("capture/showAreaSelector", true).toBool())
        areaSelector->show();

    if (index == FullScreen)
    {
        // 刷新显示器
    }
    else if (index == OneWindow)
    {
        // 刷新窗口对象
        on_refreshWindows_clicked();
    }

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
    }
    else
    {
        areaSelector->hide();
        settings.setValue("capture/showAreaSelector", false);
        ui->showAreaSelector->setText("显示截图区域");
        settings.setValue("capture/area", areaSelector->geometry());
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

void MainWindow::on_fastCaptureEdit_editingFinished()
{
    QString s = ui->fastCaptureEdit->text();
    setFastShortcut(s);
    settings.setValue("key/capture", s);
}

void MainWindow::on_serialCaptureEdit_editingFinished()
{
    QString s = ui->serialCaptureEdit->text();
    setSerialShortcut(s);
    settings.setValue("key/serial", s);
}

void MainWindow::on_spinBox_valueChanged(int arg1)
{
    settings.setValue("serial/interval", arg1);
    serialTimer->setInterval(arg1);
    prevTimer->setInterval(arg1);
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

void MainWindow::on_prevCaptureCheckBox_stateChanged(int)
{
    bool check = ui->prevCaptureCheckBox->isChecked();
    if (check)
    {
        // 开启预先截图
        startPrevCapture();
    }
    else
    {
        // 关闭预先截图
        clearPrevCapture();
        ui->prevCaptureCheckBox->setText("未开启");
    }

    settings.setValue("capture/prev", check);

    ui->capturePrev5sButton->setEnabled(check);
    ui->capturePrev13sButton->setEnabled(check);
    ui->capturePrev30sButton->setEnabled(check);
    ui->capturePrev60sButton->setEnabled(check);
}

void MainWindow::on_capturePrev5sButton_clicked()
{
    savePrevCapture(5000);
}

void MainWindow::on_capturePrev13sButton_clicked()
{
    savePrevCapture(13000);
}

void MainWindow::on_capturePrev30sButton_clicked()
{
    savePrevCapture(30000);
}

void MainWindow::on_capturePrev60sButton_clicked()
{
    savePrevCapture(60000);
}

void MainWindow::on_actionAbout_triggered()
{
    QString text;
    text.append("支持穿越时空，通过快速截图、连续截图、预先截图等功能，\n获取 过去、当下、未来 的任意截图\n");
    text.append("自带极其方便的图片管理工具，针对连续截图进行优化，一键提取最佳图片。\n\n");
    text.append("开发者：MRXY001\n");
    text.append("GitHub：https://github.com/MRXY001");
    QMessageBox::information(this, "关于", text);
}

void MainWindow::on_actionGitHub_triggered()
{
    QDesktopServices::openUrl(QUrl(QLatin1String("https://github.com/MRXY001/LiveEmojiCapture")));
}

void MainWindow::on_windowsCombo_activated(int index)
{
    if (index == -1)
        return ;
    currentHwnd = reinterpret_cast<HWND>(ui->windowsCombo->currentData(Qt::UserRole).toLongLong());
    if (currentHwnd && ui->modeTab->currentIndex() == OneWindow)
        showPreview(getScreenShot());
}

void MainWindow::on_refreshWindows_clicked()
{
    ui->windowsCombo->clear();
    HWND pWnd = first_window(EXCLUDE_MINIMIZED); // 得到第一个窗口句柄
    while (pWnd)
    {
        QString title = get_window_title(pWnd);
        QString clss = get_window_class(pWnd);

        if (!title.isEmpty())
        {
            ui->windowsCombo->addItem(title + " (" + clss + ")", QVariant(reinterpret_cast<qint64>(pWnd)));
            if (pWnd == currentHwnd)
                ui->windowsCombo->setCurrentIndex(ui->windowsCombo->count()-1);
        }

        pWnd = next_window(pWnd, EXCLUDE_MINIMIZED); // 得到下一个窗口句柄
    }
}

void MainWindow::on_selectScreenWindow_clicked()
{
    bool hidden = areaSelector->isHidden();
    areaSelector->hide();
    QRect rect = WindowSelector::getArea();
    if (!hidden)
        areaSelector->show();

    if (rect.width() == 0)
        return ;

    // 设置area为结果
    areaSelector->setArea(rect);
}
