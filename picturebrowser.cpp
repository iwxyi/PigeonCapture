#include "picturebrowser.h"
#include "ui_picturebrowser.h"

PictureBrowser::PictureBrowser(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::PictureBrowser)
{
    ui->setupUi(this);

    if (settings.contains("picturebrowser/iconSize"))
    {
        int size = settings.value("picturebrowser/iconSize").toInt();
        ui->listWidget->setIconSize(QSize(size, size));

        if (size == 64)
            ui->actionIcon_Small->setChecked(true);
        else if (size == 128)
            ui->actionIcon_Middle->setChecked(true);
        else if (size == 256)
            ui->actionIcon_Large->setChecked(true);
        else if (size == 512)
            ui->actionIcon_Largest->setChecked(true);
    }

    connect(ui->splitter, &QSplitter::splitterMoved, this, [=](int, int){
        QSize size(ui->listWidget->iconSize());
        ui->listWidget->setIconSize(QSize(1, 1));
        ui->listWidget->setIconSize(size);
    });

    readSortFlags();

    // 恢复菜单
    if (sortFlags & QDir::Name)
        ui->actionSort_By_Name->setChecked(true);
    else // 默认按时间
        ui->actionSort_By_Time->setChecked(true);
    if (sortFlags &QDir::Reversed)
        ui->actionSort_DESC->setChecked(true);
    else // 默认从小到大（新的在后面）
        ui->actionSort_AESC->setChecked(true);

    // 播放动图
    slideTimer = new QTimer(this);
    int interval = settings.value("picturebrowser/slideInterval", 100).toInt();
    slideTimer->setInterval(interval);
    connect(slideTimer, &QTimer::timeout, this, [=]{
        int row = ui->listWidget->currentRow();
        if (row == -1)
            return ;
        int count = ui->listWidget->count();
        if (row < count-1)
        {
            ui->listWidget->setCurrentRow(row+1); // 播放下一帧
        }
        else if (count > 1 && row == count-1
                 && settings.value("picturebrowser/slideReturnFirst", false).toBool())
        {
            if (ui->listWidget->item(0)->data(FilePathRole).toString() == BACK_PREV_DIRECTORY)
                // 子目录第一项是返回上一级
                ui->listWidget->setCurrentRow(1); // 从头(第一张图)开始播放
            else
                ui->listWidget->setCurrentRow(0); // 从头开始播放
        }
    });
    if (interval == 16)
        ui->actionSlide_16ms->setChecked(true);
    else if (interval == 33)
        ui->actionSlide_33ms->setChecked(true);
    else if (interval == 100)
        ui->actionSlide_100ms->setChecked(true);
    else if (interval == 200)
        ui->actionSlide_200ms->setChecked(true);
    else if (interval == 500)
        ui->actionSlide_500ms->setChecked(true);
    else if (interval == 1000)
        ui->actionSlide_1000ms->setChecked(true);
    else if (interval == 3000)
        ui->actionSlide_3000ms->setChecked(true);
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
    bool isSubDir = currentDirPath != rootDirPath;
    ui->actionBack_Prev_Directory->setEnabled(isSubDir);
    ui->actionExtra_Selected->setEnabled(isSubDir);
    ui->actionExtra_And_Delete->setEnabled(isSubDir);
    ui->actionDelete_Unselected->setEnabled(isSubDir);
    ui->actionDelete_Up_Files->setEnabled(isSubDir);
    ui->actionDelete_Down_Files->setEnabled(isSubDir);

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
    QList<QFileInfo> infos =dir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot,
                                              sortFlags);
    QSize maxIconSize = ui->listWidget->iconSize();
    if (maxIconSize.width() <= 16 || maxIconSize.height() <= 16)
        maxIconSize = QSize(32, 32);
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
        item->setToolTip(info.fileName() + "\n" + info.lastModified().toString("yyyy-MM-dd hh-mm-ss.zzz"));
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
    ui->listWidget->setIconSize(QSize(1, 1));
    ui->listWidget->setIconSize(QSize(is, is));
}

void PictureBrowser::closeEvent(QCloseEvent *event)
{
    settings.setValue("picturebrowser/geometry", this->saveGeometry());
    settings.setValue("picturebrowser/state", this->saveState());
    settings.setValue("picturebrowser/splitterGeometry", ui->splitter->saveGeometry());
    settings.setValue("picturebrowser/splitterState", ui->splitter->saveState());
}

void PictureBrowser::readSortFlags()
{
    sortFlags = static_cast<QDir::SortFlags>(settings.value("picturebrowser/sortType", QDir::Time).toInt());

    if (settings.value("picturebrowser/sortReversed", false).toBool())
        sortFlags |= QDir::Reversed;
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

void PictureBrowser::setSlideInterval(int ms)
{
    settings.setValue("picturebrowser/slideInterval", ms);
    slideTimer->setInterval(ms);

    ui->actionSlide_16ms->setChecked(false);
    ui->actionSlide_33ms->setChecked(false);
    ui->actionSlide_100ms->setChecked(false);
    ui->actionSlide_200ms->setChecked(false);
    ui->actionSlide_500ms->setChecked(false);
    ui->actionSlide_1000ms->setChecked(false);
    ui->actionSlide_3000ms->setChecked(false);
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

    ui->actionIcon_Small->setChecked(true);
    ui->actionIcon_Middle->setChecked(false);
    ui->actionIcon_Large->setChecked(false);
    ui->actionIcon_Largest->setChecked(false);
}

void PictureBrowser::on_actionIcon_Middle_triggered()
{
    setListWidgetIconSize(128);

    ui->actionIcon_Small->setChecked(false);
    ui->actionIcon_Middle->setChecked(true);
    ui->actionIcon_Large->setChecked(false);
    ui->actionIcon_Largest->setChecked(false);
}

void PictureBrowser::on_actionIcon_Large_triggered()
{
    setListWidgetIconSize(256);

    ui->actionIcon_Small->setChecked(false);
    ui->actionIcon_Middle->setChecked(false);
    ui->actionIcon_Large->setChecked(true);
    ui->actionIcon_Largest->setChecked(false);
}

void PictureBrowser::on_actionIcon_Largest_triggered()
{
    setListWidgetIconSize(512);

    ui->actionIcon_Small->setChecked(false);
    ui->actionIcon_Middle->setChecked(false);
    ui->actionIcon_Large->setChecked(false);
    ui->actionIcon_Largest->setChecked(true);
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
    menu->addSeparator();
    menu->addAction(ui->actionStart_Play_GIF);
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

void PictureBrowser::on_actionZoom_In_triggered()
{
    ui->previewPicture->scaleTo(1.25, ui->previewPicture->geometry().center());
}

void PictureBrowser::on_actionZoom_Out_triggered()
{
    ui->previewPicture->scaleTo(0.8, ui->previewPicture->geometry().center());
}

void PictureBrowser::on_actionRestore_Size_triggered()
{
    // 还原预览图默认大小
    ui->previewPicture->resetScale();
}

void PictureBrowser::on_actionFill_Size_triggered()
{
    // 预览图填充屏幕
    ui->previewPicture->scaleToFill();
}

void PictureBrowser::on_actionOrigin_Size_triggered()
{
    // 原始大小
    ui->previewPicture->scaleToOrigin();
}

void PictureBrowser::on_actionDelete_Selected_triggered()
{
    auto items = ui->listWidget->selectedItems();
    if (!items.size())
        return ;
    int firstRow = ui->listWidget->row(items.first());
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
    ui->listWidget->setCurrentRow(firstRow);
}

void PictureBrowser::on_actionExtra_Selected_triggered()
{
    auto items = ui->listWidget->selectedItems();
    if (!items.size())
        return ;
    int firstRow = ui->listWidget->row(items.first());
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
    ui->listWidget->setCurrentRow(firstRow);
}

void PictureBrowser::on_actionDelete_Unselected_triggered()
{
    auto items = ui->listWidget->selectedItems();
    if (!items.size())
        return ;
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
    if (!items.size())
        return ;
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
    ui->actionDelete_Up_Files->setEnabled(count == 1 && currentDirPath != rootDirPath);
    ui->actionDelete_Down_Files->setEnabled(count == 1 && currentDirPath != rootDirPath);
}

void PictureBrowser::on_actionSort_By_Time_triggered()
{
    settings.setValue("picturebrowser/sortType", QDir::Time);
    readSortFlags();
    on_actionRefresh_triggered();
    ui->actionSort_By_Name->setChecked(false);
}

void PictureBrowser::on_actionSort_By_Name_triggered()
{
    settings.setValue("picturebrowser/sortType", QDir::Name);
    readSortFlags();
    on_actionRefresh_triggered();
    ui->actionSort_By_Time->setChecked(false);
}

void PictureBrowser::on_actionSort_AESC_triggered()
{
    settings.setValue("picturebrowser/sortReversed", false);
    readSortFlags();
    on_actionRefresh_triggered();
    ui->actionSort_DESC->setChecked(false);
}

void PictureBrowser::on_actionSort_DESC_triggered()
{
    settings.setValue("picturebrowser/sortReversed", true);
    readSortFlags();
    on_actionRefresh_triggered();
    ui->actionSort_AESC->setChecked(false);
}

void PictureBrowser::on_listWidget_itemPressed(QListWidgetItem *item)
{
    // 停止播放
    if (slideTimer->isActive())
        slideTimer->stop();
}

void PictureBrowser::on_actionStart_Play_GIF_triggered()
{
    if (!slideTimer->isActive())
        slideTimer->start();
    else
        slideTimer->stop();
}

void PictureBrowser::on_actionSlide_100ms_triggered()
{
    setSlideInterval(100);
    ui->actionSlide_100ms->setChecked(true);
}

void PictureBrowser::on_actionSlide_200ms_triggered()
{
    setSlideInterval(200);
    ui->actionSlide_100ms->setChecked(true);
}

void PictureBrowser::on_actionSlide_500ms_triggered()
{
    setSlideInterval(500);
    ui->actionSlide_100ms->setChecked(true);
}

void PictureBrowser::on_actionSlide_1000ms_triggered()
{
    setSlideInterval(1000);
    ui->actionSlide_100ms->setChecked(true);
}

void PictureBrowser::on_actionSlide_3000ms_triggered()
{
    setSlideInterval(3000);
    ui->actionSlide_100ms->setChecked(true);
}

void PictureBrowser::on_actionSlide_Return_First_triggered()
{
    bool first = !settings.value("picturebrowser/slideReturnFirst", false).toBool();
    settings.setValue("picturebrowser/slideReturnFirst", first);

    ui->actionSlide_Return_First->setChecked(first);
}

void PictureBrowser::on_actionSlide_16ms_triggered()
{
    setSlideInterval(16);
    ui->actionSlide_16ms->setChecked(true);
}

void PictureBrowser::on_actionSlide_33ms_triggered()
{
    setSlideInterval(33);
    ui->actionSlide_33ms->setChecked(true);
}
