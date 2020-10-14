#include "picturebrowser.h"
#include "ui_picturebrowser.h"

PictureBrowser::PictureBrowser(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::PictureBrowser)
{
    ui->setupUi(this);

    connect(ui->splitter, &QSplitter::splitterMoved, this, [=](int, int){
//        showCurrentItemPreview();

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
    ui->actionBack_Prev_Directory->setEnabled(currentDirPath != rootDirPath);

    ui->listWidget->clear();
    if (targetDir.isEmpty())
        return ;

    if (currentDirPath != rootDirPath)
    {
        QListWidgetItem* item = new QListWidgetItem(QIcon(":/images/cd_up"), BACK_PREV_DIRECTORY, ui->listWidget);
        item->setData(FilePathRole, BACK_PREV_DIRECTORY);
    }

    // 读取目录的图片和文件夹
    QDir dir(targetDir);
    QList<QFileInfo> infos =dir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot, QDir::Time);
    QSize maxIconSize = ui->listWidget->iconSize();
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
                name = name.left(name.lastIndexOf("."));
            QString suffix = info.suffix();
            if (suffix != "jpg" && suffix != "png"
                    && suffix != "jpeg" && suffix != "gif")
                continue;
            QPixmap pixmap(info.absoluteFilePath());
            if (pixmap.width() > maxIconSize.width() || pixmap.height() > maxIconSize.height())
                pixmap = pixmap.scaled(maxIconSize, Qt::AspectRatioMode::KeepAspectRatio);
            QIcon icon(pixmap);
            item = new QListWidgetItem(icon, name, ui->listWidget);
        }
        else
            continue;
        item->setData(FilePathRole, info.absoluteFilePath());
        item->setToolTip(info.fileName());
    }

    restoreCurrentViewPos();
}

void PictureBrowser::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
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
    on_actionRefresh_triggered();
}

void PictureBrowser::saveCurrentViewPos()
{
    if (currentDirPath.isEmpty())
        return ;
    if (ui->listWidget->currentItem() == nullptr)
        return ;
    ListProgress progress{ui->listWidget->currentRow(),
                         ui->listWidget->verticalScrollBar()->sliderPosition(),
                         ui->listWidget->currentItem()->data(FilePathRole).toString()};
    viewPoss[currentDirPath] = progress;
}

void PictureBrowser::restoreCurrentViewPos()
{
    if (currentDirPath.isEmpty() || !viewPoss.contains(currentDirPath))
        return ;
    ListProgress progress = viewPoss.value(currentDirPath);
    ui->listWidget->setCurrentRow(progress.index);
    ui->listWidget->verticalScrollBar()->setSliderPosition(progress.scroll);
    if (ui->listWidget->currentItem()->data(FilePathRole) != progress.file)
    {
        for (int i = 0; i < ui->listWidget->count(); i++)
        {
            auto item = ui->listWidget->item(i);
            if (item->data(FilePathRole).toString() == progress.file)
            {
                ui->listWidget->setCurrentRow(i);
                break;
            }
        }
    }
}

void PictureBrowser::on_actionRefresh_triggered()
{
    enterDirectory(currentDirPath);
}

void PictureBrowser::on_listWidget_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    if (!current)
    {
        ui->previewPicture->setPixmap(QPixmap());
        return ;
    }

    QString path = current->data(FilePathRole).toString();
    QFileInfo info(path);
    if (info.isFile() && path.endsWith(".jpg"))
    {
        // 显示图片预览
        if (!ui->previewPicture->setPixmap(QPixmap(info.absoluteFilePath())))
            qDebug() << "打开图片失败：" << info.absoluteFilePath() << QPixmap(info.absoluteFilePath()).isNull();
    }
    else if (info.isDir())
    {
        QList<QFileInfo> infos = QDir(info.absoluteFilePath()).entryInfoList(QDir::Files, QDir::SortFlag::Time | QDir::SortFlag::Reversed);
        if (infos.size())
        {
            ui->previewPicture->setPixmap(QPixmap(infos.first().absoluteFilePath()));
        }
    }
}

void PictureBrowser::on_listWidget_itemActivated(QListWidgetItem *item)
{
    if (!item)
    {
        ui->previewPicture->setPixmap(QPixmap());
        return ;
    }

    // 返回上一级
    if (item->data(FilePathRole).toString() == BACK_PREV_DIRECTORY)
    {
        on_actionBack_Prev_Directory_triggered();
        return ;
    }

    // 使用系统程序打开图片
    QString path = item->data(FilePathRole).toString();
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

void PictureBrowser::on_listWidget_customContextMenuRequested(const QPoint &pos)
{
    QMenu* menu = new QMenu(this);
    menu->addAction(ui->actionOpen_Select_In_Explore);
    menu->addAction(ui->actionCopy_File);
    menu->addSeparator();
    menu->addAction(ui->actionExtra_Selected);
    menu->addAction(ui->actionExtra_And_Delete);
    menu->addAction(ui->actionDelete_Selected);
    menu->addAction(ui->actionDelete_Unselected);
    menu->addSeparator();
    menu->addAction(ui->actionDelete_Up_Files);
    menu->addAction(ui->actionDelete_Down_Files);
    menu->exec(QCursor::pos());
}

void PictureBrowser::on_actionIcon_Lager_triggered()
{
    int size = ui->listWidget->iconSize().width();
    size = size * 5 / 4;
    setListWidgetIconSize(size);
}

void PictureBrowser::on_actionIcon_Smaller_triggered()
{
    int size = ui->listWidget->iconSize().width();
    size = size * 4 / 5;
    setListWidgetIconSize(size);
}

void PictureBrowser::on_actionRestore_Size_triggered()
{
    // 还原预览图默认大小
}

void PictureBrowser::on_actionFill_Size_triggered()
{
    // 预览图填充屏幕
}

void PictureBrowser::on_actionOrigin_Size_triggered()
{
    // 原始大小
}

void PictureBrowser::on_actionDelete_Selected_triggered()
{
    auto items = ui->listWidget->selectedItems();
    foreach (auto item, items)
    {
        QString path = item->data(FilePathRole).toString();
        if (path == BACK_PREV_DIRECTORY)
            continue;

        QFileInfo info(path);
        if (info.isFile())
        {
            QFile::remove(path);
        }
        else if (info.isDir())
        {
            QDir dir(path);
            dir.removeRecursively();
        }

        int row = ui->listWidget->row(item);
        ui->listWidget->takeItem(row);
    }
}

void PictureBrowser::on_actionExtra_Selected_triggered()
{
    auto items = ui->listWidget->selectedItems();
    foreach(auto item, items)
    {
        QString path = item->data(FilePathRole).toString();
        if (path == BACK_PREV_DIRECTORY)
            continue;
        QFileInfo info(path);
        if (!info.exists())
            continue;
        if (info.isFile() || info.isDir())
        {
            QFile file(path);
            QDir dir(info.absoluteDir());
            dir.cdUp();
            file.rename(dir.absoluteFilePath(info.fileName()));
        }

        int row = ui->listWidget->row(item);
        ui->listWidget->takeItem(row);
    }
}

void PictureBrowser::on_actionDelete_Unselected_triggered()
{
    auto items = ui->listWidget->selectedItems();
    for (int i = 0; i < ui->listWidget->count(); i++)
    {
        auto item = ui->listWidget->item(i);
        if (items.contains(item))
            continue;

        QString path = item->data(FilePathRole).toString();
        if (path == BACK_PREV_DIRECTORY)
            continue;
        QFileInfo info(path);
        if (path.isEmpty() || !info.exists())
            continue;

        QFile file(path);
        file.remove();

        ui->listWidget->takeItem(i--);
    }
}

void PictureBrowser::on_actionExtra_And_Delete_triggered()
{
    auto items = ui->listWidget->selectedItems();
    QStringList paths;

    // 提取文件
    foreach (auto item, items)
    {
        QString path = item->data(FilePathRole).toString();
        if (path == BACK_PREV_DIRECTORY)
            return ;
        if (path.isEmpty() || !QFileInfo(path).exists())
            return ;

        paths.append(path);

        // 提取文件到外面
        QFileInfo info(path);
        if (!info.exists())
            continue;
        if (info.isFile() || info.isDir())
        {
            QFile file(path);
            QDir dir(info.absoluteDir());
            dir.cdUp();
            file.rename(dir.absoluteFilePath(info.fileName()));
        }
    }

    // 删除剩下的整个文件夹
    QDir dir(currentDirPath);
    QDir up(currentDirPath);
    up.cdUp();
    dir.removeRecursively();
    enterDirectory(up.absolutePath());

    // 选中刚提取的
    ui->listWidget->selectionModel()->clear();
    for (int i = 0; i < ui->listWidget->count(); i++)
    {
        auto item = ui->listWidget->item(i);
        if (paths.contains(item->data(FilePathRole).toString()))
        {
            ui->listWidget->setCurrentItem(item, QItemSelectionModel::Select);
        }
    }
}

void PictureBrowser::on_actionOpen_Select_In_Explore_triggered()
{
    auto item = ui->listWidget->currentItem();
    QString path = item->data(FilePathRole).toString();
    if (path == BACK_PREV_DIRECTORY)
        return ;
    if (path.isEmpty() || !QFileInfo(path).exists())
        return ;

    QProcess process;
    path.replace("/", "\\");
    QString cmd = QString("explorer.exe /select, \"%1\"").arg(path);
    process.startDetached(cmd);
}


void PictureBrowser::on_actionOpen_In_Explore_triggered()
{
    QDesktopServices::openUrl(QUrl("file:///" + currentDirPath, QUrl::TolerantMode));
}

void PictureBrowser::on_actionBack_Prev_Directory_triggered()
{
    if (currentDirPath == rootDirPath)
        return ;
    saveCurrentViewPos();

    QDir dir(currentDirPath);
    dir.cdUp();
    enterDirectory(dir.absolutePath());
}

void PictureBrowser::on_actionCopy_File_triggered()
{
    auto items = ui->listWidget->selectedItems();
    QList<QUrl> urls;
    foreach (auto item, items)
    {
        urls.append(QUrl("file:///" + item->data(FilePathRole).toString()));
    }

    QMimeData* mime = new QMimeData();
    mime->setUrls(urls);
    QApplication::clipboard()->setMimeData(mime);
}

void PictureBrowser::on_actionCut_File_triggered()
{
    // 不支持剪切文件
}

void PictureBrowser::on_actionDelete_Up_Files_triggered()
{
    auto item = ui->listWidget->currentItem();
    int row = ui->listWidget->row(item);
    int start = 0;
    while (row--)
    {
        auto item = ui->listWidget->item(start);
        QString path = item->data(FilePathRole).toString();
        if (path == BACK_PREV_DIRECTORY)
        {
            start++;
            continue;
        }

        QFileInfo info(path);
        if (info.isFile())
        {
            QFile::remove(path);
        }
        else if (info.isDir())
        {
            QDir dir(path);
            dir.removeRecursively();
        }

        ui->listWidget->takeItem(start);
    }
}

void PictureBrowser::on_actionDelete_Down_Files_triggered()
{
    auto item = ui->listWidget->currentItem();
    int row = ui->listWidget->row(item) + 1;
    while (row < ui->listWidget->count())
    {
        auto item = ui->listWidget->item(row);
        QString path = item->data(FilePathRole).toString();
        if (path == BACK_PREV_DIRECTORY)
            continue;

        QFileInfo info(path);
        if (info.isFile())
        {
            QFile::remove(path);
        }
        else if (info.isDir())
        {
            QDir dir(path);
            dir.removeRecursively();
        }

        ui->listWidget->takeItem(row);
    }
}

void PictureBrowser::on_listWidget_itemSelectionChanged()
{
    int count = ui->listWidget->selectedItems().size();
    ui->actionDelete_Up_Files->setEnabled(count == 1);
    ui->actionDelete_Down_Files->setEnabled(count == 1);
}
