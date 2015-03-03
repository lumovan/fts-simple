#-------------------------------------------------
#
# Project created by QtCreator 2014-10-08T11:27:23
#
#-------------------------------------------------

QT       += core gui xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = knxcfg2
TEMPLATE = app


SOURCES += \
    falcon_if.cpp \
    knxcfgmain.cpp \
    main.cpp \
    util.cpp \
    backend/devicelist.cpp \
    backend/kdevice.cpp \
    gui/addresswindow.cpp \
    gui/galineedit.cpp \
    gui/paramwindow.cpp \
    gui/programdialog.cpp \
    backend/falcon/Falcon.c \
    backend/falcon/FalconClientComponents.c \
    backend/falcon/FalconConnectionManager.c \
    backend/falcon/FalconInterfaceDefines.c \
    backend/properties/parameter.cpp \
    backend/commobject.cpp \
    backend/dependencies.cpp \
    backend/paramtypes.cpp \
    backend/programmer.cpp \
    backend/programmer.cpp \
    backend/backend.cpp

HEADERS  += \
    falcon_if.h \
    knxcfgmain.h \
    resource.h \
    util.h \
    backend/falcon/falcon.h \
    backend/falcon/FalconClientComponents.h \
    backend/falcon/FalconConnectionManager.h \
    backend/falcon/FalconHResults.h \
    backend/falcon/FalconInterfaceDefines.h \
    backend/falcon/FalconSDKDefines.h \
    backend/devicelist.h \
    backend/programmer.h \
    gui/addresswindow.h \
    gui/galineedit.h \
    gui/paramwindow.h \
    gui/programdialog.h \
    backend/properties/parameter.h \
    backend/properties/propertydatatype.h \
    backend/kdevice.h \
    backend/commobject.h \
    backend/dependencies.h \
    backend/paramtypes.h \
    backend/backend.h

FORMS    += \
    knxcfgmain.ui \
    gui/ga_add.ui

RESOURCES += \
    icons.qrc

win32:RC_ICONS += gui/kalassi.ico


INCLUDEPATH += gui/.. ../.. ../knxcfg ../../knxcfg gui backend backend/properties backend/falcon

LIBS += ole32.lib advapi32.lib oleaut32.lib comsuppw.lib Qt5PlatformSupport.lib rpcrt4.lib # windows.lib

QMAKE_CXXFLAGS = /DNOMINMAX /DUSE_FALCON_IF

QMAKE_LFLAGS += /INCREMENTAL:NO

CONFIG += build_all
