#include "picturebrowser.h"
#include "ui_picturebrowser.h"

PictureBrowser::PictureBrowser(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::PictureBrowser)
{
    ui->setupUi(this);

    // 图标大小
    QActionGroup* iconSizeGroup = new QActionGroup(this);
    iconSizeGroup->addAction(ui->actionIcon_Largest);
    iconSizeGroup->addAction(ui->actionIcon_Large);
    iconSizeGroup->addAction(ui->actionIcon_Middle);
    iconSizeGroup->addAction(ui->actionIcon_Small);
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

    // 排序
    QActionGroup* sortTypeGroup = new QActionGroup(this);
    sortTypeGroup->addAction(ui->actionSort_By_Time);
    sortTypeGroup->addAction(ui->actionSort_By_Name);
    if (sortFlags & QDir::Name)
        ui->actionSort_By_Name->setChecked(true);
    else // 默认按时间
        ui->actionSort_By_Time->setChecked(true);

    QActionGroup* sortReverseGroup = new QActionGroup(this);
    sortReverseGroup->addAction(ui->actionSort_DESC);
    sortReverseGroup->addAction(ui->actionSort_AESC);
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

        // 切换到下一帧
        int targetRow = 0;
        auto selectedItems = ui->listWidget->selectedItems();

        if (slideInSelected && selectedItems.size() > 1 && selectedItems.contains(ui->listWidget->currentItem())) // 在多个选中项中
        {
            int index = selectedItems.indexOf(ui->listWidget->currentItem());
            index++;
            if (index == selectedItems.size())
                index = 0;
            targetRow = ui->listWidget->row(selectedItems.at(index));
        }
        else
        {
            int count = ui->listWidget->count();
            if (row < count-1)
            {
                targetRow = row + 1; // 播放下一帧
            }
            else if (count > 1 && row == count-1
                     && settings.value("picturebrowser/slideReturnFirst", false).toBool())
            {
                if (ui->listWidget->item(0)->data(FilePathRole).toString() == BACK_PREV_DIRECTORY)
                    // 子目录第一项是返回上一级
                    targetRow = 1; // 从头(第一张图)开始播放
                else
                    targetRow = 0; // 从头开始播放
            }
        }
        ui->listWidget->setCurrentRow(targetRow, QItemSelectionModel::Current);
    });
    QActionGroup* intervalGroup = new QActionGroup(this);
    intervalGroup->addAction(ui->actionSlide_16ms);
    intervalGroup->addAction(ui->actionSlide_33ms);
    intervalGroup->addAction(ui->actionSlide_100ms);
    intervalGroup->addAction(ui->actionSlide_200ms);
    intervalGroup->addAction(ui->actionSlide_500ms);
    intervalGroup->addAction(ui->actionSlide_1000ms);
    intervalGroup->addAction(ui->actionSlide_3000ms);
    if (interval == 16)
        ui->actionSlide_16ms->setChecked(true);
    else if (interval == 33)
        ui->actionSlide_33ms->setChecked(true);
    else if (interval == 50)
        ui->actionSlide_50ms->setChecked(true);
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

    // GIF录制参数
    QActionGroup* gifRecordGroup = new QActionGroup(this);
    gifRecordGroup->addAction(ui->actionGIF_Use_Record_Interval);
    gifRecordGroup->addAction(ui->actionGIF_Use_Display_Interval);
    bool gifUseRecordInterval = settings.value("gif/recordInterval", true).toBool();
    ui->actionGIF_Use_Record_Interval->setChecked(gifUseRecordInterval);
    ui->actionGIF_Use_Display_Interval->setChecked(!gifUseRecordInterval);

    QActionGroup* gifCompressGroup = new QActionGroup(this);
    gifCompressGroup->addAction(ui->actionGIF_Compress_None);
    gifCompressGroup->addAction(ui->actionGIF_Compress_x2);
    gifCompressGroup->addAction(ui->actionGIF_Compress_x4);
    gifCompressGroup->addAction(ui->actionGIF_Compress_x8);
    int gifCompress = settings.value("gif/compress", 0).toInt();
    if (gifCompress == 0)
        ui->actionGIF_Compress_None->setChecked(true);
    else if (gifCompress == 1)
        ui->actionGIF_Compress_x2->setChecked(true);
    else if (gifCompress == 2)
        ui->actionGIF_Compress_x4->setChecked(true);
    else if (gifCompress == 3)
        ui->actionGIF_Compress_x8->setChecked(true);

    connect(this, &PictureBrowser::signalGeneralGIFFinished, this, [=](QString path){
        // 显示在当前列表中
        QFileInfo info(path);
        if (info.absoluteDir() == currentDirPath)
        {
            // 还是这个目录，直接加到这个地方
            QString name = info.baseName();
            if (name.contains(" "))
                name = name.right(name.length() - name.indexOf(" ") - 1);

            QListWidgetItem* item = new QListWidgetItem(QIcon(path), info.fileName(), ui->listWidget);
            item->setData(FilePathRole, info.absoluteFilePath());
            item->setToolTip(info.fileName());
        }

        // 操作对话框
        auto result = QMessageBox::information(this, "处理完毕", "路径：" + path, "打开文件", "打开文件夹", "取消", 0, 2);
        if(result == 0) // 打开文件
        {
        #ifdef Q_OS_WIN32
            QString m_szHelpDoc = QString("file:///") + path;
            bool is_open = QDesktopServices::openUrl(QUrl(m_szHelpDoc, QUrl::TolerantMode));
            if(!is_open)
                qDebug() << "打开图片失败：" << path;
        #else
            QString  cmd = QString("xdg-open ")+ m_szHelpDoc;　　　　　　　　//在linux下，可以通过system来xdg-open命令调用默认程序打开文件；
            system(cmd.toStdString().c_str());
        #endif
        }
        else if (result == 1) // 打开文件夹
        {
            if (path.isEmpty() || !QFileInfo(path).exists())
                return ;

            QProcess process;
            path.replace("/", "\\");
            QString cmd = QString("explorer.exe /select, \"%1\"").arg(path);
            process.startDetached(cmd);
        }
    });
}

PictureBrowser::~PictureBrowser()
{
    delete ui;

    QDir tempDir(tempDirPath);
    if (tempDir.exists())
        tempDir.removeRecursively();
}

void PictureBrowser::readDirectory(QString targetDir)
{
    rootDirPath = targetDir;
    tempDirPath = QDir(targetDir).absoluteFilePath(TEMP_DIRECTORY);
    recycleDir = QDir(QDir(tempDirPath).absoluteFilePath(RECYCLE_DIRECTORY));
    recycleDir.mkpath(recycleDir.absolutePath());
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
        if (name == TEMP_DIRECTORY)
            continue;
        if (name.contains(" "))
            name = name.right(name.length() - name.indexOf(" ") - 1);
        QListWidgetItem* item;
        if (info.isDir())
        {
            item = new QListWidgetItem(QIcon(":/images/directory"), name, ui->listWidget);
        }
        else if (info.isFile())
        {
            if (name.endsWith(".jpg") || name.endsWith(".png") || name.endsWith(".jpeg"))
                name = name.left(name.lastIndexOf("."));
            // gif后缀显示出来
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

    QSize size(ui->listWidget->iconSize());
    ui->listWidget->setIconSize(QSize(1, 1));
    ui->listWidget->setIconSize(size);
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

     slideTimer->stop();
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
    {
        if (ui->listWidget->count() > 0)
        {
            if (ui->listWidget->count() > 1
                    && ui->listWidget->item(0)->data(FilePathRole) == BACK_PREV_DIRECTORY)
                ui->listWidget->setCurrentRow(1);
            else
                ui->listWidget->setCurrentRow(0);
        }
        return ;
    }
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
}

void PictureBrowser::on_actionRefresh_triggered()
{
    saveCurrentViewPos();
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
    if (info.isFile())
    {
        // 显示图片预览
        if ((path.endsWith(".jpg") || path.endsWith(".png") || path.endsWith(".jpeg")))
        {
            if (!ui->previewPicture->setPixmap(QPixmap(info.absoluteFilePath())))
                qDebug() << "打开图片失败：" << info.absoluteFilePath() << QPixmap(info.absoluteFilePath()).isNull();
        }
        else if (path.endsWith(".gif"))
        {
            ui->previewPicture->setGif(path);
        }
        else
        {
            qDebug() << "无法识别文件：" << info.absoluteFilePath();
        }
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
}

void PictureBrowser::on_actionIcon_Middle_triggered()
{
    setListWidgetIconSize(128);
    ui->actionIcon_Middle->setChecked(true);
}

void PictureBrowser::on_actionIcon_Large_triggered()
{
    setListWidgetIconSize(256);
    ui->actionIcon_Large->setChecked(true);
}

void PictureBrowser::on_actionIcon_Largest_triggered()
{
    setListWidgetIconSize(512);
    ui->actionIcon_Largest->setChecked(true);
}

void PictureBrowser::on_listWidget_customContextMenuRequested(const QPoint &pos)
{
    QMenu* menu = new QMenu(this);
    menu->addAction(ui->actionOpen_Select_In_Explore);
    menu->addAction(ui->actionCopy_File);
    menu->addSeparator();
    menu->addAction(ui->actionMark_Red);
    menu->addAction(ui->actionMark_Green);
    menu->addAction(ui->actionMark_None);
    menu->addSeparator();
    menu->addAction(ui->actionExtra_Selected);
    menu->addAction(ui->actionExtra_And_Delete);
    menu->addAction(ui->actionDelete_Selected);
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
    ui->previewPicture->unbindMovie();
    int firstRow = ui->listWidget->row(items.first());
    foreach (auto item, items)
    {
        QString path = item->data(FilePathRole).toString();
        if (path == BACK_PREV_DIRECTORY)
            continue;

        deleteFileOrDir(path);

        int row = ui->listWidget->row(item);
        ui->listWidget->takeItem(row);
    }
    commitDeleteCommand();
    ui->listWidget->setCurrentRow(firstRow);
}

void PictureBrowser::on_actionExtra_Selected_triggered()
{
    auto items = ui->listWidget->selectedItems();
    if (!items.size())
        return ;
    ui->previewPicture->unbindMovie();
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

        deleteFileOrDir(path);

        ui->listWidget->takeItem(i--);
    }
    commitDeleteCommand();
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
    deleteFileOrDir(up.absoluteFilePath(dir.dirName()));
    commitDeleteCommand();
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
    commitDeleteCommand();
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

        deleteFileOrDir(path);

        ui->listWidget->takeItem(start);
    }
    commitDeleteCommand();
    ui->listWidget->scrollToTop();
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

        deleteFileOrDir(path);

        ui->listWidget->takeItem(row);
    }
    commitDeleteCommand();
    ui->listWidget->scrollToBottom();
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
}

void PictureBrowser::on_actionSort_By_Name_triggered()
{
    settings.setValue("picturebrowser/sortType", QDir::Name);
    readSortFlags();
    on_actionRefresh_triggered();
}

void PictureBrowser::on_actionSort_AESC_triggered()
{
    settings.setValue("picturebrowser/sortReversed", false);
    readSortFlags();
    on_actionRefresh_triggered();
}

void PictureBrowser::on_actionSort_DESC_triggered()
{
    settings.setValue("picturebrowser/sortReversed", true);
    readSortFlags();
    on_actionRefresh_triggered();
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
    {
        // 调整选中项
        removeUselessItemSelect();
        auto selectedItems = ui->listWidget->selectedItems();

        if (selectedItems.size() > 1 && selectedItems.contains(ui->listWidget->currentItem()))
        {
            slideInSelected = true;

            // 排序
            std::sort(selectedItems.begin(), selectedItems.end(), [=](QListWidgetItem*a, QListWidgetItem* b){
                return ui->listWidget->row(a) < ui->listWidget->row(b);
            });
            ui->listWidget->clearSelection();
            for (int i = 0; i < selectedItems.size(); i++)
            {
                ui->listWidget->setCurrentItem(selectedItems.at(i), QItemSelectionModel::Select);
            }
            ui->listWidget->setCurrentItem(selectedItems.first(), QItemSelectionModel::Current);
        }
        else
            slideInSelected = false;

        slideTimer->start();
    }
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
    ui->actionSlide_200ms->setChecked(true);
}

void PictureBrowser::on_actionSlide_500ms_triggered()
{
    setSlideInterval(500);
    ui->actionSlide_500ms->setChecked(true);
}

void PictureBrowser::on_actionSlide_1000ms_triggered()
{
    setSlideInterval(1000);
    ui->actionSlide_1000ms->setChecked(true);
}

void PictureBrowser::on_actionSlide_3000ms_triggered()
{
    setSlideInterval(3000);
    ui->actionSlide_3000ms->setChecked(true);
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

void PictureBrowser::on_actionSlide_50ms_triggered()
{
    setSlideInterval(50);
    ui->actionSlide_50ms->setChecked(true);
}

void PictureBrowser::on_actionMark_Red_triggered()
{
    auto items = ui->listWidget->selectedItems();
    foreach (auto item, items)
    {
        item->setBackground(redMark);
    }
}

void PictureBrowser::on_actionMark_Green_triggered()
{
    auto items = ui->listWidget->selectedItems();
    foreach (auto item, items)
    {
        item->setBackground(greenMark);
    }
}

void PictureBrowser::on_actionMark_None_triggered()
{
    auto items = ui->listWidget->selectedItems();
    foreach (auto item, items)
    {
        item->setBackground(Qt::transparent);
    }
}

void PictureBrowser::on_actionSelect_Green_Marked_triggered()
{
    for (int i = 0; i < ui->listWidget->count(); i++)
    {
        auto item = ui->listWidget->item(i);
        if (item->background() == greenMark)
        {
            ui->listWidget->setCurrentItem(item, QItemSelectionModel::Select);
        }
    }
}

void PictureBrowser::on_actionSelect_Red_Marked_triggered()
{
    for (int i = 0; i < ui->listWidget->count(); i++)
    {
        auto item = ui->listWidget->item(i);
        if (item->background() == redMark)
        {
            ui->listWidget->setCurrentItem(item, QItemSelectionModel::Select);
        }
    }
}

void PictureBrowser::on_actionPlace_Red_Top_triggered()
{
    int index = 0;
    if (ui->listWidget->item(0)->data(FilePathRole).toString() == BACK_PREV_DIRECTORY)
        index++;
    for (int i = index+1; i < ui->listWidget->count(); i++)
    {
        auto item = ui->listWidget->item(i);
        if (item->background() == redMark)
        {
            ui->listWidget->takeItem(i);
            ui->listWidget->insertItem(index++, item);
        }
    }
    ui->listWidget->scrollToTop();
}

void PictureBrowser::on_actionPlace_Green_Top_triggered()
{
    int index = 0;
    if (ui->listWidget->item(0)->data(FilePathRole).toString() == BACK_PREV_DIRECTORY)
        index++;
    for (int i = index+1; i < ui->listWidget->count(); i++)
    {
        auto item = ui->listWidget->item(i);
        if (item->background() == greenMark)
        {
            ui->listWidget->takeItem(i);
            ui->listWidget->insertItem(index++, item);
        }
    }
    ui->listWidget->scrollToTop();
}

void PictureBrowser::on_actionOpen_Directory_triggered()
{
    QString path = QFileDialog::getExistingDirectory(this, "选择要打开的文件夹", rootDirPath);
    if (!path.isEmpty())
        readDirectory(path);
}

void PictureBrowser::on_actionSelect_Reverse_triggered()
{
    auto selectedItems = ui->listWidget->selectedItems();
    if (selectedItems.size() == 0)
        return ;
    int start = 0;
    if (ui->listWidget->item(0)->data(FilePathRole).toString() == BACK_PREV_DIRECTORY)
        start++;
    ui->listWidget->setCurrentRow(0, QItemSelectionModel::Clear);
    for (int i = start; i < ui->listWidget->count(); i++)
    {
        auto item = ui->listWidget->item(i);
        if (!selectedItems.contains(item))
            ui->listWidget->setCurrentRow(i, QItemSelectionModel::Select);
    }
}

void PictureBrowser::deleteFileOrDir(QString path)
{
    QFileInfo info(path);
    QString newPath = recycleDir.absoluteFilePath(info.fileName());
    deleteCommandsQueue.append(QPair<QString, QString>(path, newPath));
    if (info.isFile())
    {
        if (QFileInfo(newPath).exists())
            QFile(newPath).remove();

        // QFile::remove(path);
        if (!QFile(path).rename(newPath))
            qDebug() << "删除文件失败：" << path;
    }
    else if (info.isDir())
    {
        if (QFileInfo(newPath).exists())
            QDir(newPath).removeRecursively();

        QDir dir(path);
        // dir.removeRecursively();
        if (!dir.rename(path, newPath))
            qDebug() << "删除文件夹失败：" << path;
    }
}

void PictureBrowser::commitDeleteCommand()
{
    deleteUndoCommands.append(deleteCommandsQueue);
    deleteCommandsQueue.clear(); // 等待下一次的commit
    ui->actionUndo_Delete_Command->setEnabled(true);
}

void PictureBrowser::removeUselessItemSelect()
{
    auto selectedItems = ui->listWidget->selectedItems();
    if (selectedItems.size() >= 1)
    {
        auto backItem = ui->listWidget->item(0);
        int  currentRow = ui->listWidget->currentRow();
        if (backItem->data(FilePathRole) == BACK_PREV_DIRECTORY && selectedItems.contains(backItem))
        {
            ui->listWidget->setCurrentRow(0, QItemSelectionModel::Deselect);
            selectedItems.removeOne(backItem);

            if (currentRow != 0)
                ui->listWidget->setCurrentRow(currentRow, QItemSelectionModel::Current);
            else
                ui->listWidget->setCurrentItem(selectedItems.first(), QItemSelectionModel::Current);
            qDebug() << "自动移除【返回上一级】项";
        }
    }
}

void PictureBrowser::on_actionUndo_Delete_Command_triggered()
{
    if (deleteUndoCommands.size() == 0)
        return ;

    QList<QPair<QString, QString>> deleteCommand = deleteUndoCommands.last();
    QStringList existOriginPaths;
    QStringList existRecyclePaths;
    for (int i = deleteCommand.size()-1; i >= 0; i--)
    {
        QString oldPath = deleteCommand.at(i).first;
        QString newPath = deleteCommand.at(i).second;

        // 回收站文件不存在（手动删除）
        if (!QFileInfo(newPath).exists())
            continue;

        // 文件已存在
        if (QFileInfo(oldPath).exists())
        {
            existOriginPaths.append(oldPath);
            existRecyclePaths.append(newPath);
            continue;
        }

        // 重命名
        QFileInfo info(newPath);
        if (info.isDir())
        {
            QDir(newPath).rename(newPath, oldPath);
        }
        else if (info.isFile())
        {
            QFile(newPath).rename(newPath, oldPath);
        }
    }

    deleteUndoCommands.removeLast();

    if (existOriginPaths.size())
    {
        if (QMessageBox::question(this, "文件已存在", "下列原文件存在，是否覆盖？\n\n" + existOriginPaths.join("\n"),
                                  QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Yes)
        {
            for (int i = existOriginPaths.size() - 1; i >= 0; i--)
            {
                QString oldPath = existOriginPaths.at(i);
                QString newPath = existRecyclePaths.at(i);

                // 文件已存在
                QFileInfo oldInfo(oldPath);
                if (oldInfo.exists())
                {
                    if (oldInfo.isDir())
                        QDir(oldPath).removeRecursively();
                    else if (oldInfo.isFile())
                        QFile(oldPath).remove();
                }

                // 重命名
                QFileInfo info(newPath);
                if (info.isDir())
                {
                    QDir(newPath).rename(newPath, oldPath);
                }
                else if (info.isFile())
                {
                    QFile(newPath).rename(newPath, oldPath);
                }
            }
        }
    }

    on_actionRefresh_triggered();
    ui->actionUndo_Delete_Command->setEnabled(deleteUndoCommands.size());
}

/**
 * 源代码参考自：https://github.com/douzhongqiang/EasyGifTool
 */
void PictureBrowser::on_actionGeneral_GIF_triggered()
{
    removeUselessItemSelect();
    auto selectedItems = ui->listWidget->selectedItems();

    if (selectedItems.size() < 2)
    {
        QMessageBox::warning(this, "生成GIF", "请选中至少2张图片");
        return ;
    }
    // 进行排序啊
    std::sort(selectedItems.begin(), selectedItems.end(), [=](QListWidgetItem*a, QListWidgetItem* b){
        return ui->listWidget->row(a) < ui->listWidget->row(b);
    });

    // 获取间隔
    bool gifUseRecordInterval = settings.value("gif/recordInterval", true).toBool();
    int interval = 0;
    if (gifUseRecordInterval)
    {
        // 从当前文件夹的配置文件中读取时间
        QFileInfo info(QDir(currentDirPath).absoluteFilePath(SEQUENCE_PARAM_FILE));
        if (info.exists())
        {
            QSettings st(info.absoluteFilePath(), QSettings::IniFormat);
            interval = st.value("gif/interval", slideTimer->interval()).toInt();
            qDebug() << "读取录制时间：" << interval;
        }
    }
    if (interval <= 0)
    {
        // 使用默认的播放时间
        interval = slideTimer->interval();
    }

    // 所有图片路径
    QStringList pixmapPaths;
    for (int i = 0; i < selectedItems.size(); i++)
        pixmapPaths.append(selectedItems.at(i)->data(FilePathRole).toString());

    // 获取图片大小
    auto item = selectedItems.first();
    QPixmap firstPixmap(item->data(FilePathRole).toString());
    QSize size = firstPixmap.size(); // 图片大小
    QString gifPath = QDir(currentDirPath).absoluteFilePath(QDateTime::currentDateTime().toString("yyyy-MM-dd hh-mm-ss.zzz.gif"));

    // 压缩程度
    int compress = settings.value("gif/compress", 0).toInt();
    int prop = 1;
    for (int i = 0; i < compress; i++)
        prop *= 2;
    size_t wt = static_cast<uint32_t>(size.width() / prop);
    size_t ht = static_cast<uint32_t>(size.height() / prop);
    size_t iv = static_cast<uint32_t>(interval / 8); // GIF合成的工具有问题，只能自己微调时间了

    // 创建GIF
    QtConcurrent::run([=]{
        Gif_H m_Gif;
        Gif_H::GifWriter* m_GifWriter = new Gif_H::GifWriter;
        if (!m_Gif.GifBegin(m_GifWriter, gifPath.toLocal8Bit().data(), wt, ht, iv))
        {
            qDebug() << "开启gif失败";
            delete m_GifWriter;
            return;
        }

        for (int i = 0; i < pixmapPaths.size(); i++)
        {
            QPixmap pixmap(pixmapPaths.at(i));
            if (!pixmap.isNull())
            {
                if (prop > 1)
                    pixmap = pixmap.scaled(static_cast<int>(wt), static_cast<int>(ht));
                m_Gif.GifWriteFrame(m_GifWriter, pixmap.toImage().convertToFormat(QImage::Format_RGBA8888).constBits(), wt, ht, iv);
            }
        }

        m_Gif.GifEnd(m_GifWriter);
        delete m_GifWriter;

        emit signalGeneralGIFFinished(gifPath);
        qDebug() << "GIF生成完毕：" << size << pixmapPaths.size() << interval;
    });
}

void PictureBrowser::on_actionGIF_Use_Record_Interval_triggered()
{
    settings.setValue("gif/recordInterval", true);
    ui->actionGIF_Use_Record_Interval->setChecked(true);
}

void PictureBrowser::on_actionGIF_Use_Display_Interval_triggered()
{
    settings.setValue("gif/recordInterval", false);
    ui->actionGIF_Use_Display_Interval->setChecked(true);
}

void PictureBrowser::on_actionGIF_Compress_None_triggered()
{
    settings.setValue("gif/compress", 0);
    ui->actionGIF_Compress_None->setChecked(true);
}

void PictureBrowser::on_actionGIF_Compress_x2_triggered()
{
    settings.setValue("gif/compress", 1);
    ui->actionGIF_Compress_x2->setChecked(true);
}

void PictureBrowser::on_actionGIF_Compress_x4_triggered()
{
    settings.setValue("gif/compress", 2);
    ui->actionGIF_Compress_x4->setChecked(true);
}

void PictureBrowser::on_actionGIF_Compress_x8_triggered()
{
    settings.setValue("gif/compress", 3);
    ui->actionGIF_Compress_x8->setChecked(true);
}

void PictureBrowser::on_actionUnpack_GIF_File_triggered()
{
    // 选择gif
    QString path = QFileDialog::getOpenFileName(this, "请选择GIF路径，拆分成为一帧一帧的图片", rootDirPath, "Images (*.gif)");
    if (path.isEmpty())
        return ;

    // 设置提取路径
    QFileInfo info(path);
    QString name = info.baseName();
    QString dir = QDir(currentDirPath).absoluteFilePath(name);
    if (QFileInfo(dir).exists())
    {
        int index = 0;
        while (QFileInfo(dir+"("+QString::number(++index)+")").exists());
        dir += "("+QString::number(++index)+")";
    }

    // 开始提取
    QDir saveDir(dir);
    saveDir.mkpath(saveDir.absolutePath());
    QMovie movie(path);
    movie.setCacheMode(QMovie::CacheAll);
    for (int i = 0; i < movie.frameCount(); ++i)
    {
        movie.jumpToFrame(i);
        QImage image = movie.currentImage();
        QFile file(saveDir.absoluteFilePath(QString("%1.jpg").arg(i)));
        file.open(QFile::WriteOnly);
        image.save(&file, "JPG");
        file.close();
    }

    // 打开文件夹
    auto result = QMessageBox::information(this, "分解GIF完毕", "路径：" + saveDir.absolutePath(), "进入文件夹", "显示在资源管理器", "取消", 0, 2);
    if (result == 0) // 进入文件夹
    {
        enterDirectory(saveDir.absolutePath());
    }
    else if (result == 1) // 显示在资源管理器
    {
        QDesktopServices::openUrl(QUrl("file:///" + saveDir.absolutePath(), QUrl::TolerantMode));
    }
}

/**
 * 字符画链接：https://zhuanlan.zhihu.com/p/65232824
 */
void PictureBrowser::on_actionGIF_ASCII_Art_triggered()
{
    // 选择gif
    QString path = QFileDialog::getOpenFileName(this, "请选择图片路径，制作字符画", rootDirPath, "Images (*.gif *.jpg *.png *.jpeg)");
    if (path.isEmpty())
        return ;

    // 保存路径
    QFileInfo info(path);
    QString suffix = info.suffix();
    QString savePath = QDir(info.absoluteDir()).absoluteFilePath(info.baseName() + "_ART");
    if (QFileInfo(savePath + "." + suffix).exists())
    {
        int index = 0;
        while (QFileInfo(savePath+"("+QString::number(++index)+")." + suffix).exists());
        savePath += "(" + QString::number(index) + ")";
    }
    savePath += "." + suffix;

    // 普通图片，马上弄
    if (!path.endsWith(".gif"))
    {
        ASCIIArt art;
        QPixmap pixmap(path);
        QPixmap cache(art.setImage(pixmap.toImage(),
                                   pixmap.hasAlpha() ? Qt::transparent : Qt::white));
        cache.save(savePath);
        emit signalGeneralGIFFinished(savePath);
        return ;
    }

    QtConcurrent::run([=]{
        // 初始化
        QMovie movie(path);
        ASCIIArt art;
        movie.jumpToFrame(0);

        // 生成cache
        QList<QPixmap> cachePixmaps;
        QList<QPixmap> srcPixmaps;
        int count = movie.frameCount();
        for (int i = 0; i < count; i++)
        {
            srcPixmaps.insert(i, movie.currentPixmap());
            cachePixmaps.insert(i, art.setImage(movie.currentImage(), Qt::white));
            movie.jumpToNextFrame();
        }

        QSize size = cachePixmaps.first().size();
        size_t wt = static_cast<uint32_t>(size.width());
        size_t ht = static_cast<uint32_t>(size.height());
        size_t iv = static_cast<uint32_t>(movie.speed()) / 8;

        Gif_H m_Gif;
        Gif_H::GifWriter* m_GifWriter = new Gif_H::GifWriter;
        if (!m_Gif.GifBegin(m_GifWriter, savePath.toLocal8Bit().data(), wt, ht, iv))
        {
            qDebug() << "开启gif失败";
            delete m_GifWriter;
            return;
        }

        for (int i = 0; i < cachePixmaps.size(); i++)
        {
            m_Gif.GifWriteFrame(m_GifWriter,
                                cachePixmaps.at(i).toImage()
                                    .convertToFormat(QImage::Format_RGBA8888).bits(),
                                wt, ht, iv);
        }

        m_Gif.GifEnd(m_GifWriter);
        delete m_GifWriter;

        emit signalGeneralGIFFinished(savePath);
        qDebug() << "GIF生成完毕：" << size << cachePixmaps.first().size() << iv;
    });
}
