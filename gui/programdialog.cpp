#include "programdialog.h"

ProgramDialog::ProgramDialog(QWidget *parent) : QDialog(parent) {
    layout = new QGridLayout(this);
    setLayout(layout);

    ckb_pa = new QCheckBox(tr("Program PA"),this);
    ckb_pa->setChecked(false);
    layout->addWidget(ckb_pa,0,0,1,2);
    QObject::connect(ckb_pa,SIGNAL(toggled(bool)),this,SLOT(enabler()));
/*    ckb_pa->setEnabled(false);
#pragma message("Warning: PA programming is tbd");
*/

    ckb_ga = new QCheckBox(tr("Program Group Addresses"),this);
    ckb_ga->setChecked(true);
    layout->addWidget(ckb_ga,1,0,1,2);
    QObject::connect(ckb_ga,SIGNAL(toggled(bool)),this,SLOT(enabler()));

    ckb_params = new QCheckBox(tr("Program Parameters"),this);
    ckb_params->setChecked(true);
    layout->addWidget(ckb_params,2,0,1,2);
    QObject::connect(ckb_params,SIGNAL(toggled(bool)),this,SLOT(enabler()));

    btn_ok = new QPushButton(tr("OK"));
    layout->addWidget(btn_ok,3,0,1,1);
    QObject::connect(btn_ok,SIGNAL(clicked()),this,SLOT(okClicked()));

    btn_cancel = new QPushButton(tr("Cancel"));
    layout->addWidget(btn_cancel,3,1,1,1);
    QObject::connect(btn_cancel,SIGNAL(clicked()),this,SLOT(hide()));


    setWindowTitle(tr("Program Device"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setModal(true);

}

void ProgramDialog::okClicked() {
    emit(startProgramming(
                (ckb_pa->isChecked()?PROGRAM_PA:0) |
                (ckb_ga->isChecked()?PROGRAM_GA:0) |
                (ckb_params->isChecked()?PROGRAM_PARAMS:0)));
}

void ProgramDialog::enabler() {
    if (ckb_pa->isChecked() || ckb_ga->isChecked() || ckb_params->isChecked()) {
        btn_ok->setEnabled(true);
    } else {
        btn_ok->setEnabled(false);
    }
}

void ProgramDialog::programmingDone(bool result) {
    setVisible(false);
}
