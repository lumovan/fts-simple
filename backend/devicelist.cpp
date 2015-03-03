#include <stdio.h>
#include <algorithm>
#include <iterator>
#include <QDebug>
#include <QFile>
#include <QMap>
#include <QtXml>
#include "kdevice.h"
#include "devicelist.h"
#include "util.h"


DeviceList::DeviceList() {
    active_device = 0;
}

bool DeviceList::readFile(QString filename) {
    QDomDocument doc;

    QFile *file = new QFile(filename);
    if (!doc.setContent(file)) {
        return false;
    }

    QDomElement root = doc.documentElement();
    if (root.nodeName().compare("knx-devicelist")) {
        return false;
    }

    devices.clear();
    for (QDomElement dev = root.firstChildElement("device"); !dev.isNull(); dev = dev.nextSiblingElement("device")) {
        uint16_t pa = pa2uint(dev.attribute("address"));
        KDevice *device = new KDevice(dev);
        device->setPA(pa);
        devices.insert(pa,device);
    }

//    qSort(devices.keys().begin(),devices.keys().end(),qLess<uint16_t>());
    return true;
}

bool DeviceList::writeFile(QString filename) {
    QDomDocument doc;
    QDomElement rootElement = doc.createElement("knx-devicelist");
    rootElement.setAttribute("version","1.0");
    doc.appendChild(rootElement);

    for (QMap<uint16_t,KDevice*>::iterator iter_dev = devices.begin(); iter_dev != devices.end(); iter_dev++) {
        QDomElement deviceElement = iter_dev.value()->toXML(doc);
        deviceElement.setAttribute("address", uint2pa(iter_dev.key()));
        rootElement.appendChild(deviceElement);
    }

    QFile file (filename);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    if (file.write(doc.toByteArray(2)) < 0) {
        file.close();
        return false;
    }
    file.close();
    return true;
}

bool DeviceList::addDeviceFromFile(QString filename) {
    QDomDocument doc;

    if (devices.keys().contains(0)) {
        return false;
    }

    QFile *file = new QFile(filename);
    if (!doc.setContent(file)) {
        return false;
    }

    QDomElement root = doc.documentElement();
    if (root.nodeName().compare("device-description")) {
        return false;
    }

    devices.insert(0,new KDevice(root));
//    qSort(devices.keys().begin(),devices.keys().end(),qLess<uint16_t>());
    return true;
}

void DeviceList::removeActiveDevice() {
    if (active_device == 0) {
        return;
    }
    devices.remove(pa2uint(active_device->getPA()));
    delete active_device;
    active_device = 0;
}


QStringList DeviceList::getDevices() {
    QStringList ret;
    QList<uint16_t> addresses;

    ret.clear();
    addresses = devices.keys();
    for (int n=0; n < addresses.count(); n++) {
        uint16_t addr = addresses.at(n);
        ret.append(QString(uint2pa(addr).append(" - ").append(devices[addr]->getName())));
    }
    return ret;
}

void DeviceList::setActiveDevice(uint16_t pa) {
    if (!devices.keys().contains(pa)) {
        active_device = NULL;
        return;
    }
    Logger::log("Setting active device to %s\n",uint2pa(pa).toLatin1().data()); // fflush(stdout);
    active_device = devices[pa];
}

KDevice *DeviceList::activeDevice() {
    return active_device;
}

void DeviceList::rebuildList() {
    QMap<uint16_t, KDevice* > tempDevices;

    for (QMap<uint16_t, KDevice*>::iterator iter = devices.begin(); iter != devices.end(); iter++) {
        tempDevices.insert(pa2uint(iter.value()->getPA()),iter.value());
    }
    devices = tempDevices;
}
