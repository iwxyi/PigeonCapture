QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

#RC_FILE += pigeoncapture.rc

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000
# disables all the APIs deprecated before Qt 6.0.0

include($$PWD/qxtglobalshortcut5/qxt.pri)

INCLUDEPATH += gif/ \
    capture/ \
    picture_browser/

SOURCES += \
    capture/areaselector.cpp \
    gif/gif.cpp \
    main.cpp \
    capture/mainwindow.cpp \
    picture_browser/picturebrowser.cpp \
    picture_browser/resizablepicture.cpp

HEADERS += \
    picture_browser/ASCII_Art.h \
    capture/areaselector.h \
    gif/gif.h \
    capture/mainwindow.h \
    picture_browser/picturebrowser.h \
    picture_browser/resizablepicture.h \
    capture/windowselector.h \
    capture/windowshwnd.h \
    pigeoncapture.rc

FORMS += \
    capture/mainwindow.ui \
    picture_browser/picturebrowser.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resource.qrc

DISTFILES += \
    README.md \
    images/picture.png
