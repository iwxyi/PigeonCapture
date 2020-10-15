#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QStandardPaths>
#include <QFileDialog>
#include <QDebug>
#include <QDateTime>
#include <QScreen>
#include <QTimer>
#include <QApplication>
#include <QDesktopWidget>
#include <QDesktopServices>
#include <QtConcurrent/QtConcurrent>
#include <QMessageBox>
#include "qxtglobalshortcut.h"
#include "areaselector.h"
#include "picturebrowser.h"
#include "windowshwnd.h"
#include "windowselector.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    enum CaptureMode
    {
        FullScreen,
        ScreenArea,
        OneWindow
    };

    struct CaptureInfo
    {
        qint64 time;
        QString name;
        QPixmap* pixmap;
    };

    void selectArea();

    QPixmap getScreenShot();

    void setFastShortcut(QString s);
    void setSerialShortcut(QString s);
    void showPreview(QPixmap pixmap);

private slots:
    void on_selectDirButton_clicked();
    void on_fastCaptureShortcut_clicked();
    void on_serialCaptureShortcut_clicked();
    void on_modeTab_currentChanged(int index);

    void runCapture();
    void triggerFastCapture();
    void triggerSerialCapture();
    void startPrevCapture();
    void savePrevCapture(qint64 delta);
    void clearPrevCapture();
    void areaSelectorMoved(QRect rect);

    void on_showAreaSelector_clicked();

    void on_actionOpen_Directory_triggered();

    void on_actionCapture_History_triggered();

    void on_fastCaptureEdit_editingFinished();

    void on_serialCaptureEdit_editingFinished();

    void on_spinBox_valueChanged(int arg1);

    void on_actionRestore_Geometry_triggered();

    void on_prevCaptureCheckBox_stateChanged(int arg1);

    void on_capturePrev5sButton_clicked();

    void on_capturePrev13sButton_clicked();

    void on_capturePrev30sButton_clicked();

    void on_capturePrev60sButton_clicked();

    void on_actionAbout_triggered();

    void on_actionGitHub_triggered();

    void on_windowsCombo_activated(int index);

    void on_refreshWindows_clicked();

    void on_selectScreenWindow_clicked();

protected:
    void showEvent(QShowEvent* event);
    void closeEvent(QCloseEvent* event);
    void resizeEvent(QResizeEvent*);

    QString timeToFile();
    qint64 getTimestamp();

    QString get_window_title(HWND hwnd);
    QString get_window_class(HWND hwnd);

private:
    Ui::MainWindow *ui;
    QSettings settings;
    QString saveDir;
    QxtGlobalShortcut *fastCaptureShortcut = nullptr;
    QxtGlobalShortcut *serialCaptureShortcut = nullptr;
    AreaSelector* areaSelector = nullptr;

    QTimer* tipTimer = nullptr;
    QTimer* serialTimer = nullptr;
    QString serialCaptureDir;
    int serialCaptureCount = 0;

    QTimer* prevTimer = nullptr;
    QList<CaptureInfo>* prevCapturedList = nullptr; // 预先截图的工具
    qint64 prevCaptureMaxTime = 61000; // 最大提前截取60s，超过的舍弃掉

    HWND currentHwnd = nullptr;
};
#endif // MAINWINDOW_H
