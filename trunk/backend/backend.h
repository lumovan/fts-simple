#ifndef BACKEND_H
#define BACKEND_H

#include <QSettings>
#include "programmer.h"
#include "devicelist.h"

class BackEnd {
public:
    BackEnd();

    QSettings *preferences;

    Programmer *programmer;

    DeviceList devicelist;

};

#endif // BACKEND_H
