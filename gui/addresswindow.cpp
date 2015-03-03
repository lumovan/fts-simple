#include <QDialog>
#include <QListWidgetItem>
#include <QMessageBox>
#include "knxcfgmain.h"
#include "addresswindow.h"
#include "galineedit.h"
#include "util.h"

AddressWindow::AddressWindow(BackEnd *backend, QWidget *parent) : QDialog(parent) {

    layout = new QGridLayout;
    setLayout(layout);
    layout->setColumnStretch(0,20);
    layout->setColumnStretch(1,10);
    layout->setColumnStretch(2,10);
    layout->setRowStretch(1,10);
    layout->setRowStretch(2,10);
    layout->setRowStretch(3,10);

    layout->addWidget(new QLabel("Communication Object"),0,0,1,1);

    lst_ko = new QListWidget(this);
    lst_ko->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    layout->addWidget(lst_ko,1,0,4,1);
    lst_ko->setSelectionMode(QAbstractItemView::SingleSelection);
    QObject::connect(lst_ko,SIGNAL(itemSelectionChanged()),this,SLOT(coChanged()));

    QGroupBox *pnl_ga = new QGroupBox("Group Addresses");
    pnl_ga->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    pnl_ga->setLayout(new QGridLayout());
    layout->addWidget(pnl_ga,1,1,3,2);

    lst_ga = new QListWidget(pnl_ga);
    lst_ga->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    ((QGridLayout*)(pnl_ga->layout()))->addWidget(lst_ga,0,0,1,2);
    lst_ga->setSelectionMode(QAbstractItemView::SingleSelection);
    QObject::connect(lst_ga,SIGNAL(itemSelectionChanged()),this,SLOT(gaChanged()));
    QObject::connect(lst_ga,SIGNAL(itemDoubleClicked(QListWidgetItem*)),this,SLOT(gaMakePrimary(QListWidgetItem*)));

    btn_add = new QPushButton("Add...",pnl_ga);
    btn_add->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
    ((QGridLayout*)(pnl_ga->layout()))->addWidget(btn_add,1,0,1,1);
    QObject::connect(btn_add,SIGNAL(clicked()),this,SLOT(gaAdd()));

    btn_remove = new QPushButton("Remove",pnl_ga);
    btn_remove->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
    ((QGridLayout*)(pnl_ga->layout()))->addWidget(btn_remove,1,1,1,1);
    btn_remove->setEnabled(false);
    QObject::connect(btn_remove,SIGNAL(clicked()),this,SLOT(gaRemove()));

    pnl_flags = new QGroupBox(this);
    pnl_flags->setLayout(new QGridLayout(pnl_flags));
    pnl_flags->setTitle("Flags");
    pnl_flags->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
    layout->addWidget(pnl_flags,4,1,1,2);
    ckb_flags[0] = new QCheckBox(tr("C")); ckb_flags[0]->setToolTip(tr("<b>Communication</b><br/>Enable object, must be set to transmit or receive any values"));
    ckb_flags[1] = new QCheckBox(tr("R")); ckb_flags[1]->setToolTip(tr("<b>Read</b><br/>Respond to read telegrams"));
    ckb_flags[2] = new QCheckBox(tr("W")); ckb_flags[2]->setToolTip(tr("<b>Write</b><br/>Change value upon receiving a write telegram"));
    ckb_flags[3] = new QCheckBox(tr("U")); ckb_flags[3]->setToolTip(tr("<b>Update</b><br/>Change value upon receiving the response to a read telegram"));
    ckb_flags[4] = new QCheckBox(tr("T")); ckb_flags[4]->setToolTip(tr("<b>Transmit</b><br/>Send telegrams to the bus"));
    ckb_flags[5] = new QCheckBox(tr("I")); ckb_flags[5]->setToolTip(tr("<b>Read on Init</b>"));

    for (int n=0; n < 6; n++) {
        ((QGridLayout*)(pnl_flags->layout()))->addWidget(ckb_flags[n],0,n,1,1);
        QObject::connect(ckb_flags[n],SIGNAL(toggled(bool)),this,SLOT(flagsChanged()));
    }

    QGroupBox *frm_pa = new QGroupBox(this);
    frm_pa->setTitle("Physical Address");
    frm_pa->setLayout(new QHBoxLayout(frm_pa));
    txf_pa = new QLineEdit(frm_pa);
    frm_pa->layout()->addWidget(txf_pa);
    btn_paSet = new QPushButton(tr("Set"),frm_pa);
    btn_paSet->setEnabled(false);
    frm_pa->layout()->addWidget(btn_paSet);
    frm_pa->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
    layout->addWidget(frm_pa,5,1,1,2);
    QObject::connect(btn_paSet,SIGNAL(clicked()),this,SLOT(paChanged()));
    QObject::connect(txf_pa,SIGNAL(textEdited(QString)),this,SLOT(paCheck(QString)));

    QGroupBox *frm_name = new QGroupBox(this);
    frm_name->setTitle("Device Name");
    frm_name->setLayout(new QHBoxLayout(frm_name));
    txf_name = new QLineEdit(frm_name);
    frm_name->layout()->addWidget(txf_name);
    btn_nameSet = new QPushButton("Rename",frm_name);
    frm_name->layout()->addWidget(btn_nameSet);
    frm_name->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
    layout->addWidget(frm_name,5,0,1,1);
    QObject::connect(btn_nameSet,SIGNAL(clicked()),this,SLOT(nameChanged()));

    activeDevice = 0;
    activeCO = 0;
    this->backend = backend;
}

void AddressWindow::updateActiveDevice(KDevice *device) {
    activeDevice = device;

    lst_ko->clear();
    for (int n=0; n < 6; n++) {
        ckb_flags[n]->setChecked(false);
    }
    if (activeDevice == 0) {
        txf_pa->setText("");
        txf_name->setText("");
        lst_ga->clear();
        return;
    }
    lst_ko->addItems(activeDevice->coList());
    for (int n=0; n < lst_ko->count(); n++) {
        uint8_t co;
        lst_ko->item(n)->setSelected(false);
        if (!getCO(&co,lst_ko->item(n))) {
            printf("ERROR: cannot find CO\n");
        } else {
            lst_ko->item(n)->setHidden(!device->getCOEnabled(co));
        }
    }

    txf_pa->setText(activeDevice->getPA());
    txf_name->setText(activeDevice->getName());

    lst_ga->clear();
}

void AddressWindow::onParamModified() {
    for (int n=0; n < lst_ko->count(); n++) {
        uint8_t co;
        if (!getCO(&co,lst_ko->item(n))) {
            printf("ERROR: cannot find CO\n");
        } else {
            lst_ko->item(n)->setHidden(!activeDevice->getCOEnabled(co));
        }
    }
}

uint8_t AddressWindow::getCO(uint8_t *co, QListWidgetItem *item) {
    QString str_ko = item->text();
    QStringList stl_ko = str_ko.split(" ");
    if (stl_ko.count() < 1) {
        return 0;
    }
    bool ok;
    *co = stl_ko[0].toUInt(&ok);
    if (!ok) {
        return 0;
    }

    return 1;
}

void AddressWindow::coChanged() {
    int16_t selected_row = lst_ga->currentRow();
    lst_ga->clear();
    QList<QListWidgetItem*> llwi_ko = lst_ko->selectedItems();
    if (llwi_ko.count() < 1) {
        return;
    }

    if (!getCO(&activeCO,llwi_ko[0])) {
        return;
    }
    lst_ga->addItems(activeDevice->assocList(activeCO));
    /* Flag 0x01: Communicate */
    lst_ga->setEnabled(activeDevice->getFlags(activeCO) & 0x01);
    btn_add->setEnabled(activeDevice->getFlags(activeCO) & 0x01);

    /* Flag 0x10: Transmit */
    QListWidgetItem *item0 = lst_ga->item(0);
    if (item0 != NULL) {
        QFont font = item0->font();
        font.setBold((activeDevice->getFlags(activeCO) & 0x10)?true:false);
        item0->setFont(font);
    }

//    printf("--%X\n",activeDevice->getFlags(activeCO)); fflush(stdout);
    for (int n=0; n < 6; n++) {
        QObject::disconnect(ckb_flags[n],SIGNAL(toggled(bool)),this,SLOT(flagsChanged()));
        ckb_flags[n]->setChecked(activeDevice->getFlags(activeCO) & (1 << n));
        QObject::connect(ckb_flags[n],SIGNAL(toggled(bool)),this,SLOT(flagsChanged()));
    }
    if (lst_ga->count() > 0) {
        if (selected_row <= 0) {
            lst_ga->setCurrentRow(0);
        } else if (selected_row >= lst_ga->count()) {
            lst_ga->setCurrentRow(lst_ga->count()-1);
        } else {
            lst_ga->setCurrentRow(selected_row);
        }
    }
}

void AddressWindow::gaChanged() {
    btn_remove->setEnabled(lst_ga->selectedItems().count() > 0);
}

void AddressWindow::flagsChanged() {
    uint8_t flags = 0;
    for (int n=0; n < 6; n++) {
        if (ckb_flags[n]->isChecked()) {
            flags |= (1 << n);
        }
    }
    activeDevice->setFlags(activeCO,flags);
    coChanged();
}


#include <stdio.h>
void AddressWindow::gaAdd() {
    if ((activeDevice == 0) || (activeCO == 0)) {
        return;
    }
    QDialog  *dlg_add = new QDialog (this);
    GALineEdit *txf_ga = new GALineEdit(dlg_add);
    QPushButton *btn_ok = new QPushButton(tr("OK"));
    QPushButton *btn_cancel = new QPushButton(tr("Cancel"));

    dlg_add->setLayout(new QGridLayout(dlg_add));
    ((QGridLayout*)(dlg_add->layout()))->addWidget(new QLabel("Group Address"),0,0,1,1);
    ((QGridLayout*)(dlg_add->layout()))->addWidget(txf_ga,1,0,1,1);
    ((QGridLayout*)(dlg_add->layout()))->addWidget(btn_ok,0,1,1,1);
    ((QGridLayout*)(dlg_add->layout()))->addWidget(btn_cancel,1,1,1,1);
    ((QGridLayout*)(dlg_add->layout()))->setColumnMinimumWidth(0,120);
    btn_ok->setEnabled(false);
    QObject::connect(btn_ok,SIGNAL(clicked()),dlg_add,SLOT(accept()));
    QObject::connect(btn_cancel,SIGNAL(clicked()),dlg_add,SLOT(reject()));
    QObject::connect(txf_ga,SIGNAL(valid(bool)),btn_ok,SLOT(setEnabled(bool)));

    if (dlg_add->exec() == 0) {
        return;
    }
    uint16_t ga = ga2uint(txf_ga->text());
    activeDevice->addAssoc(activeCO,ga);
    coChanged();
}

void AddressWindow::gaRemove() {
    QList<QListWidgetItem*> llwi_ga = lst_ga->selectedItems();
    if (llwi_ga.count() < 1) {
        return;
    }
    QListWidgetItem *lwi_ga = llwi_ga[0];
    QString str_ga = lwi_ga->text();
    uint16_t ga = ga2uint(str_ga);
    if (ga == 0) {
        return;
    }
    activeDevice->removeAssoc(activeCO,ga);
    coChanged();
}

void AddressWindow::gaMakePrimary(QListWidgetItem *item) {
    if (item == NULL) {
        return;
    }
    uint16_t ga = ga2uint(item->text());
    activeDevice->gaMakePrimary(activeCO,ga);
    coChanged();
    lst_ga->setCurrentRow(0);
}


void AddressWindow::paCheck(QString pa) {
    if (pa2uint(pa) != 0) {
        btn_paSet->setEnabled(true);
    } else {
        btn_paSet->setEnabled(false);
    }
}

void AddressWindow::paChanged() {
    if (activeDevice == 0) {
        return;
    }
    uint16_t pa = pa2uint(txf_pa->text());
    if (pa == 0) {
        return;
    }
    QStringList devices = backend->devicelist.getDevices();
    for (QStringList::iterator iter = devices.begin(); iter != devices.end(); iter++) {
        if (iter->startsWith(uint2pa(pa))) {
            QMessageBox box;
            box.setWindowTitle(tr("PA").append(" ").append(uint2pa(pa)));
            box.setText(tr("Address already in use"));
            box.exec();
            return;
        }
    }

//    printf("$1"); fflush(stdout);
    activeDevice->setPA(pa);
//    printf("$2"); fflush(stdout);
    emit deviceInfoChanged(activeDevice);
//    printf("$3"); fflush(stdout);
}

void AddressWindow::nameChanged() {
    if (activeDevice == 0) {
        return;
    }

    QString name = txf_name->text();
    if (!name.compare("")) {
        return;
    }

    printf("$1"); fflush(stdout);
    activeDevice->setName(name);
    printf("$2"); fflush(stdout);
    emit deviceInfoChanged(activeDevice);
}
