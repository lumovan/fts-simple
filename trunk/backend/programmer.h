#ifndef PROGRAMMER_H
#define PROGRAMMER_H

#include <qthread.h>
#include <stdint.h>
#include "falcon_if.h"
#include "kdevice.h"

class Programmer : public QThread {
    Q_OBJECT

public:
    Programmer();
    ~Programmer();

    bool setConnection();
    bool startProgramming(KDevice *device,uint8_t what);
    bool doProgramming(KDevice *device,uint8_t what);
    bool reflash(KDevice *device);
    bool restart(KDevice *device);
    bool doReflash(KDevice *device);
    bool doRestart(KDevice *device);
    bool isWorking();

    QString connectionDesc();

signals:
    void statusChanged(QString);
    void isActive(bool);
    void programmingDone(bool);

public slots:
    void cancel();

private:
    FalconIF *falcon;

    bool active;
};

#endif // PROGRAMMER_H
