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
#include "qxtglobalshortcut.h"
#include "areaselector.h"
#include "picturebrowser.h"

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

    void selectArea();

    QPixmap getScreenShot();
    QString getCurrentSavePath();

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
    void areaSelectorMoved(QRect rect);

    void on_showAreaSelector_clicked();

    void on_actionOpen_Directory_triggered();

    void on_actionCapture_History_triggered();

    void on_fastCaptureEdit_textEdited(const QString &arg1);

    void on_serialCaptureEdit_textEdited(const QString &arg1);

    void on_spinBox_valueChanged(int arg1);

    void on_comboBox_activated(int index);

protected:
    void showEvent(QShowEvent* event);
    void closeEvent(QCloseEvent* event);
    void resizeEvent(QResizeEvent* event);

private:
    Ui::MainWindow *ui;
    QSettings settings;
    QString saveDir;
    QxtGlobalShortcut *fastCaptureShortcut = nullptr;
    QxtGlobalShortcut *serialCaptureShortcut = nullptr;

    QTimer* tipTimer = nullptr;
    QTimer* serialTimer = nullptr;
    QString serialCaptureDir;
    int serialCaptureCount = 0;

    AreaSelector* areaSelector = nullptr;
};
#endif // MAINWINDOW_H
