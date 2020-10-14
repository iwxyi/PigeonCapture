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

#define BACK_PREV_DIRECTORY ".."

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
    };

    void enterDirectory(QString targetDir);
    void readDirectory(QString targetDir);

protected:
    void resizeEvent(QResizeEvent* event) override;
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

private:
    Ui::PictureBrowser *ui;

    QSettings settings;
    QString rootDirPath;
    QString currentDirPath;

    QHash<QString, ListProgress> viewPoss;
};

#endif // PICTUREBROWSER_H
