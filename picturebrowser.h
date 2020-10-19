#ifndef PICTUREBROWSER_H
#define PICTUREBROWSER_H

#include <QMainWindow>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QListWidget>
#include <QSettings>
#include <QDebug>
#include <QProcess>
#include <QDesktopServices>
#include <QUrl>
#include <QHash>
#include <QScrollBar>
#include <QClipboard>
#include <QMimeData>
#include <QTimer>
#include <QDateTime>
#include <QFileDialog>
#include <QMessageBox>
#include <QtConcurrent/QtConcurrent>
#include "gif.h"

#define BACK_PREV_DIRECTORY ".."
#define TEMP_DIRECTORY "temp"
#define RECYCLE_DIRECTORY "recycle"
#define SEQUENCE_PARAM_FILE "params.ini"
#define FilePathRole (Qt::UserRole)
#define FileMarkRole (Qt::UserRole+1)

namespace Ui {
class PictureBrowser;
}

class PictureBrowser : public QMainWindow
{
    Q_OBJECT

public:
    PictureBrowser(QWidget *parent = nullptr);
    ~PictureBrowser();

    struct ListProgress
    {
        int index;
        int scroll;
        QString file;
    };

    void enterDirectory(QString targetDir);
    void readDirectory(QString targetDir);

protected:
    void resizeEvent(QResizeEvent*) override;
    void showEvent(QShowEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

    void readSortFlags();
    void showCurrentItemPreview();
    void setListWidgetIconSize(int x);
    void saveCurrentViewPos();
    void restoreCurrentViewPos();
    void setSlideInterval(int ms);

private slots:
    void on_actionRefresh_triggered();

    void on_listWidget_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);

    void on_listWidget_itemActivated(QListWidgetItem *item);

    void on_actionIcon_Small_triggered();

    void on_actionIcon_Middle_triggered();

    void on_actionIcon_Large_triggered();

    void on_actionIcon_Largest_triggered();

    void on_listWidget_customContextMenuRequested(const QPoint &pos);

    void on_actionIcon_Lager_triggered();

    void on_actionIcon_Smaller_triggered();

    void on_actionZoom_In_triggered();

    void on_actionZoom_Out_triggered();

    void on_actionRestore_Size_triggered();

    void on_actionFill_Size_triggered();

    void on_actionOrigin_Size_triggered();

    void on_actionDelete_Selected_triggered();

    void on_actionExtra_Selected_triggered();

    void on_actionDelete_Unselected_triggered();

    void on_actionExtra_And_Delete_triggered();

    void on_actionOpen_Select_In_Explore_triggered();

    void on_actionOpen_In_Explore_triggered();

    void on_actionBack_Prev_Directory_triggered();

    void on_actionCopy_File_triggered();

    void on_actionCut_File_triggered();

    void on_actionDelete_Up_Files_triggered();

    void on_actionDelete_Down_Files_triggered();

    void on_listWidget_itemSelectionChanged();

    void on_actionSort_By_Time_triggered();

    void on_actionSort_By_Name_triggered();

    void on_actionSort_AESC_triggered();

    void on_actionSort_DESC_triggered();

    void on_listWidget_itemPressed(QListWidgetItem *item);

    void on_actionStart_Play_GIF_triggered();

    void on_actionSlide_100ms_triggered();

    void on_actionSlide_200ms_triggered();

    void on_actionSlide_500ms_triggered();

    void on_actionSlide_1000ms_triggered();

    void on_actionSlide_3000ms_triggered();

    void on_actionSlide_Return_First_triggered();

    void on_actionSlide_16ms_triggered();

    void on_actionSlide_33ms_triggered();

    void on_actionSlide_50ms_triggered();

    void on_actionMark_Red_triggered();

    void on_actionMark_Green_triggered();

    void on_actionMark_None_triggered();

    void on_actionSelect_Green_Marked_triggered();

    void on_actionSelect_Red_Marked_triggered();

    void on_actionPlace_Red_Top_triggered();

    void on_actionPlace_Green_Top_triggered();

    void on_actionOpen_Directory_triggered();

    void on_actionSelect_Reverse_triggered();

    void on_actionUndo_Delete_Command_triggered();

    void on_actionGeneral_GIF_triggered();

    void on_actionGIF_Use_Record_Interval_triggered();

    void on_actionGIF_Use_Display_Interval_triggered();

    void on_actionGIF_Compress_None_triggered();

    void on_actionGIF_Compress_x2_triggered();

    void on_actionGIF_Compress_x4_triggered();

    void on_actionGIF_Compress_x8_triggered();

    void on_actionUnpack_GIF_File_triggered();

private:
    void deleteFileOrDir(QString path);
    void commitDeleteCommand();
    void removeUselessItemSelect();

signals:
    void signalGeneralGIFFinished(QString path);

private:
    Ui::PictureBrowser *ui;

    QSettings settings;
    QString rootDirPath;
    QString tempDirPath;
    QDir recycleDir;
    QString currentDirPath;
    QDir::SortFlags sortFlags;

    QHash<QString, ListProgress> viewPoss; // 缓存每个文件夹的浏览位置
    QTimer* slideTimer;
    bool slideInSelected = false;

    QColor redMark = QColor(240, 128, 128);
    QColor greenMark = QColor(115, 230, 140);

    QList<QList<QPair<QString, QString>>> deleteUndoCommands;
    QList<QPair<QString, QString>> deleteCommandsQueue;
};

#endif // PICTUREBROWSER_H
