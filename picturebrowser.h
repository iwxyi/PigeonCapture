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

#define BACK_PREV_DIRECTORY ".."
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

    void showCurrentItemPreview();
    void setListWidgetIconSize(int x);
    void saveCurrentViewPos();
    void restoreCurrentViewPos();

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

private:

private:
    Ui::PictureBrowser *ui;

    QSettings settings;
    QString rootDirPath;
    QString currentDirPath;

    QHash<QString, ListProgress> viewPoss;
};

#endif // PICTUREBROWSER_H