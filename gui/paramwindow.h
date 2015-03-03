#ifndef PARAMWINDOW_H
#define PARAMWINDOW_H

#include <QDialog>
#include <QLabel>
#include <QGridLayout>
#include "kdevice.h"

class ParamWindow : public QDialog
{
    Q_OBJECT
public:
    explicit ParamWindow(QWidget *parent = 0);
    void resizeEvent (QResizeEvent *);
signals:
    void paramModified();

public slots:
    void updateActiveDevice(KDevice *device);
    void valueModified(QString value);

private:
    QWidget *canvas;
    QGridLayout *layout, *canvasLayout;
    KDevice *activeDevice;

    QList<QWidget*> editors;
    QList<QLabel*> labels;
    QList<QLabel*> units;
};

#endif // PARAMWINDOW_H
