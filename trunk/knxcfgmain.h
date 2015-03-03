#ifndef KNXCFGMAIN_H
#define KNXCFGMAIN_H

#include <stdio.h>
#include <QMainWindow>
#include <QGridLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSettings>
#include <QString>
#include <QTabWidget>
#include <QTextEdit>
#include <QToolBar>
#include <backend.h>
#include "addresswindow.h"
#include "devicelist.h"
#include "falcon_if.h"
#include "paramwindow.h"
#include "programdialog.h"
#include "programmer.h"

namespace Ui {
class KnxCfgMain;
}

class KnxCfgMain : public QMainWindow
{
    Q_OBJECT

public:
    explicit KnxCfgMain(QWidget *parent = 0);
    void afterConstructor();
    ~KnxCfgMain();

    void closeEvent(QCloseEvent *);

    void setPreference(QString key, QString value);
    void setPreference(QString key, int value);
    QString getPreference(QString key);
    int getPreferenceInt(QString key);
    bool readDeviceList(QString filename);
    bool writeDeviceList(QString filename);
    bool addDevice(QString filename);
    void deviceSelected(QString device_index);
    void removeDevice(QString device_index);

signals:
    void statusChanged(QString status);
    void deviceListChanged();
    void activeDeviceChanged(KDevice*);

public slots:
    void fileOpen();
    void fileSave();
    void fileSaveAs();
    void connectionSelect();
    void logging(bool);
    void updateDeviceList();
    void updateDeviceList(KDevice*);
    void addDevice();
    void removeDevice();
    void deviceSelected();
    void controlButtonEnable(KDevice*);
    void startProgramming(uint8_t);
    void reflash();
    void restart();
    void setStatusBar(QString);

private:
    void fileOpenLast();

    Ui::KnxCfgMain *ui;

    QGridLayout *layout;
    QLabel *lbl_devices;
    QListWidget *lst_devices;
    QPushButton *btn_deviceAdd, *btn_deviceRemove, *btn_Program;
    QTabWidget *cfgPane;

    AddressWindow *addressWindow;
    ParamWindow *paramWindow;

    QMenu *menu_file;
    QAction *item_load, *item_save, *item_saveas, *item_connect, *item_log, *item_exit;

    QMenu *menu_config;
    QAction *item_program, *item_reflash, *item_restart;

    QMenu *menu_help;
    QAction *item_about;

    QString lastFilename;

    QToolBar *toolbar;
    QPushButton btn_load, btn_save, btn_connect;

    QDialog dlg_about;
    QLabel lbl_about;
    QPushButton btn_about_ok;

    ProgramDialog *programDialog;

    QTextEdit *txe_logging;
    QString status;

    BackEnd *backend;
};

#endif // KNXCFGMAIN_H








