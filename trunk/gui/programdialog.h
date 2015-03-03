#ifndef PROGRAMDIALOG_H
#define PROGRAMDIALOG_H

#include <stdint.h>
#include <QCheckBox>
#include <QDialog>
#include <QGridLayout>
#include <QPushButton>

#define PROGRAM_GA 1
#define PROGRAM_PA 2
#define PROGRAM_PARAMS 4

class ProgramDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ProgramDialog(QWidget *parent = 0);

signals:
    void startProgramming(uint8_t);

public slots:
    void programmingDone(bool);

private slots:
    void enabler();
    void okClicked();

private:
    QGridLayout *layout;
    QCheckBox *ckb_pa, *ckb_ga, *ckb_params;
    QPushButton *btn_ok, *btn_cancel;

};

#endif // PROGRAMDIALOG_H
