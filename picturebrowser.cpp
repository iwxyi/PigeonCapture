#include "picturebrowser.h"
#include "ui_picturebrowser.h"

PictureBrowser::PictureBrowser(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::PictureBrowser)
{
    ui->setupUi(this);

    connect(ui->splitter, &QSplitter::splitterMoved, this, [=](int, int){
        showCurrentItemPreview();

        QSize size(ui->listWidget->iconSize());
        ui->listWidget->setIconSize(QSize(1, 1));
        ui->listWidget->setIconSize(size);
    });
}

PictureBrowser::~PictureBrowser()
{
    delete ui;
}

void PictureBrowser::readDirectory(QString targetDir)
{
    rootDirPath = targetDir;
    enterDirectory(targetDir);
}

void PictureBrowser::enterDirectory(QString targetDir)
{
    currentDirPath = targetDir;
    ui->listWidget->clear();
    if (targetDir.isEmpty())
        return ;

    if (currentDirPath != rootDirPath)
    {
        QListWidgetItem* item = new QListWidgetItem(QIcon(":/images/cd_up"), BACK_PREV_DIRECTORY, ui->listWidget);
        item->setData(Qt::UserRole, BACK_PREV_DIRECTORY);
    }

    // 读取目录的图片和文件夹
    QDir dir(rootDirPath);
    QList<QFileInfo> infos =dir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot, QDir::Time | QDir::Reversed);
    foreach (QFileInfo info, infos)
    {
        QString name = info.baseName();
        if (name.contains(" "))
            name = name.right(name.length() - name.indexOf(" ") - 1);
        QListWidgetItem* item;
        if (info.isDir())
        {
            item = new QListWidgetItem(QIcon(":/images/directory"), name, ui->listWidget);
        }
        else if (info.isFile())
        {
            if (name.contains("."))
                name = name.left(name.indexOf("."));
            QIcon icon(info.absoluteFilePath());
            item = new QListWidgetItem(icon, name, ui->listWidget);
        }
        else
            continue;
        item->setData(Qt::UserRole, info.absoluteFilePath());
        item->setToolTip(info.baseName());
    }

    restoreCurrentViewPos();
}

void PictureBrowser::resizeEvent(QResizeEvent *event)
{
    showCurrentItemPreview();
}

void PictureBrowser::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);

    restoreGeometry(settings.value("picturebrowser/geometry").toByteArray());
    restoreState(settings.value("picturebrowser/state").toByteArray());
    ui->splitter->restoreGeometry(settings.value("picturebrowser/splitterGeometry").toByteArray());
    ui->splitter->restoreState(settings.value("picturebrowser/splitterState").toByteArray());

    int is = settings.value("picturebrowser/iconSize", 64).toInt();
    ui->listWidget->setIconSize(QSize(is, is));
}

void PictureBrowser::closeEvent(QCloseEvent *event)
{
    settings.setValue("picturebrowser/geometry", this->saveGeometry());
    settings.setValue("picturebrowser/state", this->saveState());
    settings.setValue("picturebrowser/splitterGeometry", ui->splitter->saveGeometry());
    settings.setValue("picturebrowser/splitterState", ui->splitter->saveState());
}

void PictureBrowser::showCurrentItemPreview()
{
    on_listWidget_currentItemChanged(ui->listWidget->currentItem(), nullptr);
}

void PictureBrowser::setListWidgetIconSize(int x)
{
    ui->listWidget->setIconSize(QSize(x, x));
    settings.setValue("picturebrowser/iconSize", x);
}

void PictureBrowser::saveCurrentViewPos()
{
    if (currentDirPath.isEmpty())
        return ;
    ListProgress progress{ui->listWidget->currentRow(),
                         ui->listWidget->verticalScrollBar()->sliderPosition()};
    viewPoss[currentDirPath] = progress;
}

void PictureBrowser::restoreCurrentViewPos()
{
    if (currentDirPath.isEmpty() || !viewPoss.contains(currentDirPath))
        return ;
    ListProgress progress = viewPoss.value(currentDirPath);
    ui->listWidget->setCurrentRow(progress.index);
    ui->listWidget->verticalScrollBar()->setSliderPosition(progress.scroll);
}

void PictureBrowser::on_actionRefresh_triggered()
{
    readDirectory(rootDirPath);
}

void PictureBrowser::on_listWidget_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    if (!current)
    {
        ui->label->setPixmap(QPixmap());
        return ;
    }

    QString path = current->data(Qt::UserRole).toString();
    QFileInfo info(path);
    if (info.isFile() && path.endsWith(".jpg"))
    {
        // 显示图片预览
        QPixmap pixmap(path);
        if (pixmap.width() > ui->label->width() || pixmap.height() > ui->label->height())
            pixmap = pixmap.scaled(ui->label->size(), Qt::KeepAspectRatio);
        ui->label->setPixmap(pixmap);
        ui->label->setMinimumSize(1, 1); // 避免无法缩放
    }
    else if (info.isDir())
    {
        QList<QFileInfo> infos = QDir(info.absoluteFilePath()).entryInfoList(QDir::Files, QDir::SortFlag::Time | QDir::SortFlag::Reversed);
        if (infos.size())
        {
            QPixmap pixmap(infos.first().absoluteFilePath());
            if (pixmap.width() > ui->label->width() || pixmap.height() > ui->label->height())
                pixmap = pixmap.scaled(ui->label->size(), Qt::KeepAspectRatio);
            ui->label->setPixmap(pixmap);
            ui->label->setMinimumSize(1, 1); // 避免无法缩放
        }
    }
}

void PictureBrowser::on_listWidget_itemActivated(QListWidgetItem *item)
{
    if (!item)
    {
        ui->label->setPixmap(QPixmap());
        return ;
    }

    // 返回上一级
    if (item->data(Qt::UserRole).toString() == BACK_PREV_DIRECTORY)
    {
        saveCurrentViewPos();

        QDir dir(currentDirPath);
        dir.cdUp();
        enterDirectory(dir.absolutePath());
        return ;
    }

    QString path = item->data(Qt::UserRole).toString();
    QFileInfo info(path);
    if (info.isFile())
    {
#ifdef Q_OS_WIN32
    QString m_szHelpDoc = QString("file:///") + path;
    bool is_open = QDesktopServices::openUrl(QUrl(m_szHelpDoc, QUrl::TolerantMode));
    if(!is_open)
    {
        qDebug() << "打开图片失败：" << path;
    }
#else
    QString  cmd = QString("xdg-open ")+ m_szHelpDoc;　　　　　　　　//在linux下，可以通过system来xdg-open命令调用默认程序打开文件；
    system(cmd.toStdString().c_str());
#endif
    }
    else if (info.isDir())
    {
        saveCurrentViewPos();

        // 进入文件夹
        currentDirPath = info.absoluteFilePath();
        enterDirectory(currentDirPath);
    }
}

void PictureBrowser::on_actionIcon_Small_triggered()
{
    setListWidgetIconSize(64);
}

void PictureBrowser::on_actionIcon_Middle_triggered()
{
    setListWidgetIconSize(128);
}

void PictureBrowser::on_actionIcon_Large_triggered()
{
    setListWidgetIconSize(256);
}

void PictureBrowser::on_actionIcon_Largest_triggered()
{
    setListWidgetIconSize(512);
}
