#-------------------------------------------------
#
# Project created by QtCreator 2014-07-08T11:38:36
#
#-------------------------------------------------

QT       += core gui xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = knxcfg
TEMPLATE = app


SOURCES += main.cpp\
        knxcfgmain.cpp \
    gui/addresswindow.cpp \
    gui/paramwindow.cpp \
    backend/properties/property.cpp \
    backend/devicelist.cpp \
    util.cpp \
    gui/galineedit.cpp \
    falcon_if.cpp \
    backend/falcon/FalconConnectionManager.c \
    backend/falcon/FalconInterfaceDefines.c \
    backend/falcon/Falcon.c \
    gui/programdialog.cpp \
    backend/kdevice.cpp \
#    plugin_import.cpp

HEADERS  += knxcfgmain.h \
    gui/addresswindow.h \
    gui/paramwindow.h \
    backend/properties/property.h \
    backend/devicelist.h \
    util.h \
    gui/galineedit.h \
    falcon_if.h \
    gui/programdialog.h \
    backend/kdevice.h

INCLUDEPATH += gui/.. ../.. ../knxcfg ../../knxcfg gui backend backend/properties backend/falcon

FORMS    += knxcfgmain.ui \
    gui/ga_add.ui

RESOURCES += \
    icons.qrc

win32:RC_ICONS += gui/kalassi.ico

LIBS += ole32.lib advapi32.lib oleaut32.lib comsuppw.lib Qt5PlatformSupport.lib rpcrt4.lib # windows.lib

#INCLUDEPATH += "/Program Files/MinGW/include"

QMAKE_CXXFLAGS = /DNOMINMAX

CONFIG += build_all
