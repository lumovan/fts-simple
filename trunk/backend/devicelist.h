#ifndef DEVICELIST_H
#define DEVICELIST_H

#include <QString>
#include <QMap>
#include <QVector>
#include "kdevice.h"

class DeviceList
{
public:
    DeviceList();

    bool readFile(QString filename);
    bool writeFile(QString filename);

    void setActiveDevice(uint16_t pa);
    KDevice* activeDevice();
    QStringList getDevices();
    bool addDeviceFromFile(QString filename);
    void removeActiveDevice();

    void rebuildList();

private:
    QMap<uint16_t,KDevice*> devices;
    KDevice* active_device;
};

#endif // DEVICELIST_H
