#ifndef ADDRESSWINDOW_H
#define ADDRESSWINDOW_H

#include <QDialog>
#include <QGridLayout>
#include <QLabel>
#include <QFrame>
#include <QListWidget>
#include <QGroupBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLineEdit>
#include <backend.h>
#include "kdevice.h"

class AddressWindow : public QDialog
{
    Q_OBJECT
public:
    explicit AddressWindow(BackEnd *backend, QWidget *parent = 0);

signals:
    void deviceInfoChanged(KDevice *device);

public slots:
    void updateActiveDevice(KDevice *device);
    void onParamModified();

private slots:
    void coChanged();
    void gaChanged();
    void gaAdd();
    void gaRemove();
    void gaMakePrimary(QListWidgetItem*);
    void paCheck(QString);
    void paChanged();
    void nameChanged();
    void flagsChanged();

private:
    QGridLayout *layout;

    QLabel *lbl_ga, *lbl_flags;
    QListWidget *lst_ko, *lst_ga;
    QGroupBox *pnl_flags;
    QCheckBox *ckb_flags[6];
    QPushButton *btn_add, *btn_remove, *btn_paSet, *btn_nameSet;
    QLineEdit *txf_pa;
    QLineEdit *txf_name;

    KDevice *activeDevice;
    uint8_t activeCO;

    uint8_t getCO(uint8_t *co, QListWidgetItem *item);
    BackEnd *backend;
};

#endif // ADDRESSWINDOW_H
