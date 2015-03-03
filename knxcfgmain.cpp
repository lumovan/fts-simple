#include <malloc.h>
#include <stdio.h>
#include <stdint.h>

#include "knxcfgmain.h"
#include "ui_knxcfgmain.h"
#include "programmer.h"
#include "util.h"

#include <QCloseEvent>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>

KnxCfgMain::KnxCfgMain(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::KnxCfgMain) {
}

void KnxCfgMain::afterConstructor() {
    ui->setupUi(this);

    backend = new BackEnd();
    if (!backend->programmer->isWorking()) {
        QMessageBox msgBox;
        msgBox.setText(tr("<html><body><b>Driver error</b><br/>Cannot initialize Falcon Library for bus access<br/>Download Falcon 2 runtime from http://www.knx.org/knx-en/software/falcon/download/index.php and run again</body></html>"));
        msgBox.setStandardButtons(QMessageBox::Cancel);
        msgBox.setWindowTitle(tr("Kalassi Configuration Utility"));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
        exit(1);
    }
    if (backend->programmer->connectionDesc().length() > 0) {
        statusBar()->showMessage(QString(tr("Connected to ").append(backend->programmer->connectionDesc())));
    } else {
        statusBar()->showMessage(QString(tr("Not connected to bus, choose a gateway")));
    }

    layout = new QGridLayout;
    centralWidget()->setLayout(layout);
    layout->setColumnStretch(0,10);
    layout->setColumnStretch(1,0);
    layout->setColumnStretch(2,30);
    layout->setColumnStretch(3,30);

    /* Menu */
    menu_file = this->menuBar()->addMenu(tr("&File"));
    menu_config = this->menuBar()->addMenu(tr("&Configure"));
    menu_help = this->menuBar()->addMenu(tr("&Help"));
    item_load = new QAction(QIcon(":/kalassi/gui/load.png"),tr("Open Project..."),this);
    item_save = new QAction(QIcon(":/kalassi/gui/save.png"),tr("Save Project"),this);
    item_saveas = new QAction(QIcon(":/kalassi/gui/saveas.png"),tr("Save Project as..."),this);
    item_connect = new QAction(QIcon(":/kalassi/gui/connect.png"),tr("Connect to Bus..."),this);
    item_log = new QAction(tr("Enable Logging"),this);
    item_log->setCheckable(true);
    item_log->setChecked(getPreferenceInt("showLogging") > 0);
    item_exit = new QAction("Exit",this);
    item_program = new QAction(tr("Program..."),this);
    item_restart = new QAction(tr("Restart"),this);
    item_reflash = new QAction(tr("Write Firmware..."),this);
    item_about = new QAction("About...",this);
    menu_file->addAction(item_load);
    menu_file->addAction(item_save);
    menu_file->addAction(item_saveas);
    menu_file->addSeparator();
    menu_file->addAction(item_connect);
    menu_file->addAction(item_log);
    menu_file->addAction(item_exit);
    menu_config->addAction(item_program);
    menu_config->addAction(item_restart);
    menu_config->addAction(item_reflash);
    menu_help->addAction(item_about);
    menu_config->setEnabled(false);
    this->setWindowIcon(QIcon(":/kalassi/gui/kalassi.png"));
    this->setWindowTitle(QString("Kalassi Device Configuration"));

    QObject::connect(item_load,SIGNAL(triggered()),this,SLOT(fileOpen()));
    QObject::connect(item_save,SIGNAL(triggered()),this,SLOT(fileSave()));
    QObject::connect(item_saveas,SIGNAL(triggered()),this,SLOT(fileSaveAs()));
    QObject::connect(item_connect,SIGNAL(triggered()),this,SLOT(connectionSelect()));
    QObject::connect(item_log,SIGNAL(toggled(bool)),this,SLOT(logging(bool)));
    QObject::connect(item_restart,SIGNAL(triggered()),this,SLOT(restart()));
    QObject::connect(item_reflash,SIGNAL(triggered()),this,SLOT(reflash()));

    QList<QToolBar *> toolbars = findChildren<QToolBar *>();
    if (toolbars.count() > 0) {
        toolbar = toolbars.at(0);
    } else {
        toolbar = this->addToolBar("");
    }
    toolbar->addAction(item_load);
    toolbar->addAction(item_save);
    toolbar->addAction(item_saveas);
    toolbar->addAction(item_connect);

    /* Device Chooser */
    lbl_devices = new QLabel("Device");
    layout->addWidget(lbl_devices,0,0,1,2,Qt::AlignJustify);

    lst_devices = new QListWidget(this);
    lst_devices->setSelectionMode(QAbstractItemView::SingleSelection);
    lst_devices->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Expanding);
    layout->addWidget(lst_devices,1,0,1,2,Qt::AlignJustify);
    QObject::connect(lst_devices,SIGNAL(itemSelectionChanged()),this,SLOT(deviceSelected()));

    layout->setRowStretch(0,0);
    layout->setRowStretch(1,10);

    btn_deviceAdd = new QPushButton("Add...");
    layout->addWidget(btn_deviceAdd,2,0,1,1,Qt::AlignRight);
    QObject::connect(btn_deviceAdd,SIGNAL(clicked()),this,SLOT(addDevice()));

    btn_deviceRemove = new QPushButton("Remove");
    layout->addWidget(btn_deviceRemove,2,1,1,1,Qt::AlignLeft);
    btn_deviceRemove->setEnabled(false);
    QObject::connect(btn_deviceRemove,SIGNAL(clicked()),this,SLOT(removeDevice()));

    txe_logging = new QTextEdit(this);
    txe_logging->setReadOnly(true);
    layout->addWidget(txe_logging,3,0,1,4);
    Logger::setTxe(txe_logging);
    txe_logging->setVisible(getPreferenceInt("showLogging") > 0);

    addressWindow = new AddressWindow(backend,this);
    paramWindow = new ParamWindow(this);
    QObject::connect(this,SIGNAL(activeDeviceChanged(KDevice*)),addressWindow,SLOT(updateActiveDevice(KDevice*)));
    QObject::connect(this,SIGNAL(activeDeviceChanged(KDevice*)),paramWindow,SLOT(updateActiveDevice(KDevice*)));
    QObject::connect(addressWindow,SIGNAL(deviceInfoChanged(KDevice*)),this,SLOT(updateDeviceList(KDevice*)));

    cfgPane = new QTabWidget();
    cfgPane->addTab(addressWindow,"Addresses");
    cfgPane->addTab(paramWindow,"Parameters");
    cfgPane->setEnabled(false);

    cfgPane->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    layout->addWidget(cfgPane,0,2,2,2);

    layout->addItem(new QSpacerItem(1000,0,QSizePolicy::Maximum),2,1,1,2);

    programDialog = new ProgramDialog(this);
    btn_Program = new QPushButton(tr("Program..."));
    layout->addWidget(btn_Program,2,3,1,1,Qt::AlignRight);
    btn_Program->setEnabled(false);
    QObject::connect(btn_Program,SIGNAL(clicked()),programDialog,SLOT(show()));
    QObject::connect(item_program,SIGNAL(triggered()),programDialog,SLOT(show()));

    QObject::connect(this,SIGNAL(deviceListChanged()),this,SLOT(updateDeviceList()));
    QObject::connect(this,SIGNAL(activeDeviceChanged(KDevice*)),this,SLOT(controlButtonEnable(KDevice*)));
    QObject::connect(paramWindow,SIGNAL(paramModified()),addressWindow,SLOT(onParamModified()));

    QObject::connect(backend->programmer,SIGNAL(statusChanged(QString)),this,SLOT(setStatusBar(QString)));
    QObject::connect(backend->programmer,SIGNAL(programmingDone(bool)),programDialog,SLOT(programmingDone(bool)));
    QObject::connect(programDialog,SIGNAL(startProgramming(uint8_t)),this,SLOT(startProgramming(uint8_t)));

    fileOpenLast();
}

#if 0
void KnxCfgMain::afterConstructor() {
    this->setWindowIcon(QIcon(":/kalassi/gui/kalassi.png"));
    this->setWindowTitle(QString("Kalassi Device Configuration"));
    QObject::connect(lst_devices,SIGNAL(itemSelectionChanged()),this,SLOT(deviceSelected()));
    QObject::connect(btn_deviceAdd,SIGNAL(clicked()),this,SLOT(addDevice()));
    QObject::connect(btn_deviceRemove,SIGNAL(clicked()),this,SLOT(removeDevice()));
    QObject::connect(this,SIGNAL(programmingDone(bool)),programDialog,SLOT(programmingDone(bool)));

    QObject::connect(this,SIGNAL(deviceListChanged()),this,SLOT(updateDeviceList()));
    QObject::connect(this,SIGNAL(activeDeviceChanged(KDevice*)),this,SLOT(controlButtonEnable(KDevice*)));
    fileOpenLast();
}
#endif

KnxCfgMain::~KnxCfgMain() {
    delete ui;
    delete backend->programmer;
}

void KnxCfgMain::fileOpenLast() {
#if 0
    QString fileName = getPreference("mru_project_file");
    if (fileName.isEmpty()) {
        return;
    }
    if (backend->devicelist.readFile(fileName)) {
        QFileInfo fileinfo(fileName);
        emit(deviceListChanged());
        emit(activeDeviceChanged(0));
        cfgPane->setEnabled(false);
        menu_config->setEnabled(false);
        emit(statusChanged(QString("Project read from ").append(fileName)));
    }
#else
    QString fileName = QStandardPaths::locate(QStandardPaths::DataLocation,"cache.project");
    if (backend->devicelist.readFile(fileName)) {
        QFileInfo fileinfo(fileName);
        emit(deviceListChanged());
        emit(activeDeviceChanged(0));
        cfgPane->setEnabled(false);
        menu_config->setEnabled(false);

        QString mru = getPreference("mru_project_file");
        if (!mru.isEmpty()) {
            emit(statusChanged(QString("Project ").append(mru)));
        }
    }
#endif
}

void KnxCfgMain::fileOpen() {
    QString mru = getPreference("mru_project");
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),mru.isNull()?"":mru,tr("Files (*.project)"));

    if (backend->devicelist.readFile(fileName)) {
        QFileInfo fileinfo(fileName);
        setPreference("mru_project",fileinfo.canonicalPath());
        setPreference("mru_project_file",fileinfo.canonicalFilePath());
        emit(deviceListChanged());
        emit(activeDeviceChanged(0));
        cfgPane->setEnabled(false);
        menu_config->setEnabled(false);
        emit(statusChanged(QString("Project read from ").append(fileName)));
    } else {
        emit(statusChanged(QString("Could not read project from ").append(fileName)));
    }
}

void KnxCfgMain::fileSave() {
    QString fileName = getPreference("mru_project_file");
    if (fileName.isEmpty()) {
        printf("Empty filename\n"); fflush(stdout);
        fileSaveAs();
        return;
    }

    if (backend->devicelist.writeFile(fileName)) {
        printf("Written\n"); fflush(stdout);
        emit(statusChanged(QString("Project written to ").append(fileName)));
    } else {
        printf("Not written\n"); fflush(stdout);
        emit(statusChanged(QString("Could not write project to ").append(fileName)));
    }
}

void KnxCfgMain::fileSaveAs() {
    QString mru = getPreference("mru_project");
    QString fileName = QFileDialog::getSaveFileName(this, tr("Open File"),mru.isNull()?"":mru,tr("Files (*.project)"));

    if (backend->devicelist.writeFile(fileName)) {
        QFileInfo fileinfo(fileName);
        setPreference("mru_project",fileinfo.canonicalPath());
        setPreference("mru_project_file",fileinfo.canonicalFilePath());
        emit(statusChanged(QString("Project written to ").append(fileName)));
    } else {
        emit(statusChanged(QString("Could not write project to ").append(fileName)));
    }
}

/** Store current data set to default location (will be reloaded after next start).
 * Open save dialog if writing to the default location fails.
*/
void KnxCfgMain::closeEvent(QCloseEvent *event) {
    bool ok = true;

    QString fileName = QStandardPaths::locate(QStandardPaths::DataLocation,"cache.project");
    if (fileName.isEmpty()) {
        QString dir = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
        if (dir.isEmpty()) {
            ok = false;
        } else {
            QStringList dirlist = dir.split(QDir::separator());
            QString dir = QDir::separator();
            for (QStringList::iterator iter = dirlist.begin(); iter != dirlist.end(); iter++) {
                if (!QDir("dir").exists(*iter)) {
                    QDir("dir").mkdir(*iter);
                }
                dir = dir.append(QDir::separator()).append(*iter);
            }
            fileName = QStandardPaths::writableLocation(QStandardPaths::DataLocation).append(QDir::separator()).append("cache.project");
        }
        ok = backend->devicelist.writeFile(fileName);
    }
    if (!ok) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Confirmation", "Save project?",
                                      QMessageBox::Yes|QMessageBox::No|QMessageBox::Abort);
        if (reply == QMessageBox::Abort) {
            event->ignore();
            return;
        }
        if (reply == QMessageBox::Yes) {
            fileSave();
        }
        event->accept();
    }
}

void KnxCfgMain::connectionSelect() {
    if (backend->programmer->setConnection()) {
        emit(statusChanged((QString("Bus connection set to ").append(backend->programmer->connectionDesc()))));
    }
}

void KnxCfgMain::startProgramming(uint8_t what) {
    backend->programmer->startProgramming(backend->devicelist.activeDevice(),what);
}

void KnxCfgMain::deviceSelected() {
    int row = lst_devices->currentRow();
    if (row < 0) {
        cfgPane->setEnabled(false);
        emit(activeDeviceChanged(0));
        backend->devicelist.setActiveDevice(0);
        return;
    }
    QString devString = lst_devices->item(row)->text();
    QStringList parts = devString.split(" ");
    if (parts.count() < 2) {
        return;
    }
    uint16_t pa = pa2uint(parts[0]);
    backend->devicelist.setActiveDevice(pa);
    cfgPane->setEnabled(true);
    menu_config->setEnabled(true);
    emit(activeDeviceChanged(backend->devicelist.activeDevice()));
}


void KnxCfgMain::setPreference(QString key, QString value) {
    backend->preferences->setValue(key, value);
}

QString KnxCfgMain::getPreference(QString key) {
    return backend->preferences->value(key).toString();
}

void KnxCfgMain::setPreference(QString key,int value) {
    backend->preferences->setValue(key,value);
}

int KnxCfgMain::getPreferenceInt(QString key) {
    if (backend->preferences->contains(key)) {
        return backend->preferences->value(key).toInt();
    } else {
        return -1;
    }
}

void KnxCfgMain::addDevice() {
    QString mru = getPreference("mru_device");
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),mru.isNull()?"":mru,tr("Files (*.device)"));
    QFileInfo fileinfo(fileName);
    if (fileinfo.exists()) {
        setPreference("mru_device",fileinfo.canonicalPath());
        backend->devicelist.addDeviceFromFile(fileName);
    }
    updateDeviceList();
}

void KnxCfgMain::removeDevice() {
    if (backend->devicelist.activeDevice() == 0) {
        return;
    }
    backend->devicelist.removeActiveDevice();
    updateDeviceList();
}

void KnxCfgMain::updateDeviceList() {
    lst_devices->clear();
    backend->devicelist.rebuildList();
    lst_devices->addItems(backend->devicelist.getDevices());
    for (int n = 0; n < lst_devices->count(); n++) {
        lst_devices->item(n)->setSelected(false);
    }
    cfgPane->setEnabled(false);
    emit(activeDeviceChanged(0));
}

void KnxCfgMain::updateDeviceList(KDevice* device) {
//    printf("$A %i %s",lst_devices->count(),device->getPA().toLatin1().data()); fflush(stdout);
    while (lst_devices->count() > 0) {
        lst_devices->takeItem(0);
    }
//    printf("$B"); fflush(stdout);
    backend->devicelist.rebuildList();
//    printf("$C"); fflush(stdout);
    lst_devices->addItems(backend->devicelist.getDevices());
//    printf("$D %i",lst_devices->count()); fflush(stdout);
    for (int n = 0; n < lst_devices->count(); n++) {
//        printf("$E %i",n); fflush(stdout);
        lst_devices->item(n)->setSelected(false);
//        printf("$F"); fflush(stdout);

        QString devString = lst_devices->item(n)->text();
//        printf("$G"); fflush(stdout);
//        printf("\n$$%s",devString.toLatin1().data()); fflush(stdout);
        QStringList parts = devString.split(" ");
//        printf("$H"); fflush(stdout);
        if (parts.count() < 2) {
//            printf("$H2"); fflush(stdout);
            continue;
        }
//        printf("$I"); fflush(stdout);

        if (pa2uint(device->getPA()) != pa2uint(parts[0])) {
//            printf("$J"); fflush(stdout);
            continue;
        }
//        printf("$K"); fflush(stdout);
        lst_devices->item(n)->setSelected(true);
//        printf("$L"); fflush(stdout);
        break;

//        printf("%s %04X %04X\n",pa2uint(lst_devices->item(n)->text()),pa2uint(device->getPA())); fflush(stdout);
    }
    cfgPane->setEnabled(device != NULL);
//    printf("$M%s",device->getPA().toLatin1().data()); fflush(stdout);

    backend->devicelist.setActiveDevice(pa2uint(device->getPA()));
    emit(activeDeviceChanged(device));
//    printf("$N"); fflush(stdout);

}

void KnxCfgMain::restart() {
    if (backend->devicelist.activeDevice() == NULL) {
        return;
    }
    backend->programmer->restart(backend->devicelist.activeDevice());
}

void KnxCfgMain::reflash() {
    if (backend->devicelist.activeDevice() == NULL) {
        return;
    }
    QString mru = getPreference("mru_hex");
    if (mru.isEmpty()) {
        mru = getPreference("mru_device");
    }
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),mru.isNull()?"":mru,tr("Files (*.hex)"));
    if (fileName.isEmpty()) {
        return;
    }
    QFileInfo fileinfo(fileName);
    if (!(fileinfo.exists())) {
          QMessageBox::warning(this,"File not found",QString("Cannot open file ").append(fileName).append(", not found"),QMessageBox::Ok);
          return;
    }
    setPreference("mru_hex",fileinfo.canonicalPath());
    if (!backend->devicelist.activeDevice()->loadHexFile(fileinfo.canonicalFilePath())) {
        QMessageBox::warning(this,"Cannot read file",QString("Cannot read the contents of file ").append(fileName).append(", check format"),QMessageBox::Ok);
        return;
    }
    backend->programmer->reflash(backend->devicelist.activeDevice());
}

void KnxCfgMain::controlButtonEnable(KDevice* dev) {
    if (backend->devicelist.getDevices().contains(QString("0.0.0"))) {
        btn_deviceAdd->setEnabled(false);
    } else {
        btn_deviceAdd->setEnabled(true);
    }
    if (dev == 0) {
        btn_Program->setEnabled(false);
        btn_deviceRemove->setEnabled(false);
        return;
    }
    btn_Program->setEnabled(pa2uint(dev->getPA()) != 0);
    item_program->setEnabled(pa2uint(dev->getPA()) != 0);
    btn_deviceRemove->setEnabled(true);
}

void KnxCfgMain::setStatusBar(QString s) {
    statusBar()->showMessage(s);
}

void KnxCfgMain::logging(bool enable) {
    txe_logging->setVisible(enable);
    if (enable) {
        setPreference("showLogging",1);
    } else {
        setPreference("showLogging",0);
    }
}

