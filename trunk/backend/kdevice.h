#ifndef KDEVICE_H
#define KDEVICE_H

#include <QtXml>
#include <QString>
#include <QMap>
#include <QMultiMap>
#include <QVector>
#include <stdint.h>
#include "dependencies.h"
#include "parameter.h"
#include "commobject.h"
#include "paramtypes.h"

class KDeviceInfo {
public:
    KDeviceInfo(QString order_info, QString mask_version, QString app_version);
    bool matches(QList<uint8_t> order_info, uint8_t mask_version, QList<uint8_t> app_version);
    QDomElement toXML(QDomDocument doc);
private:
    QList<uint8_t> order_info;
    uint16_t mask_version;
    QList<uint8_t> app_version;
};

class KDevice {
public:
    KDevice(QDomElement elem);
    static uint16_t hex_or_dez2int(QString data);
    void setPA(uint16_t pa);
    QString getPA();
    void setName(QString name);
    QString getName();
    QDomElement toXML(QDomDocument doc);
    QStringList coList();
    QList<uint8_t> coListUint();
    QStringList assocList(uint8_t co);
    void addAssoc(uint8_t co, uint16_t ga);
    void removeAssoc(uint8_t co, uint16_t ga);
    void gaMakePrimary(uint8_t co, uint16_t ga);
    unsigned long getAccessKey();

    bool operator< (KDevice d2);
    bool operator == (KDevice d2);

    QList<uint16_t> addrtable();
    QList<uint16_t> assoctable();
    QList<uint8_t> objtable();
    uint16_t getAddrtableAddress() { return addrtable_address; }
    uint16_t getAssoctableAddress() { return assoctable_address; }
    uint16_t getObjtableAddress() { return objtable_address; }
    uint8_t getFlags(uint8_t co);
    void setFlags(uint8_t co, uint8_t flags);
    QMap<QString,Parameter*> getParameters();
    bool setParameter(QString id, QString value);
    bool getParameterEnabled(Parameter*);
    bool getParameterEnabled(QString id);
    QList<QString> getParamKeys();
    ParamTypes getParamTypes() { return paramtypes; }
    bool getCOEnabled(CommObject*);
    bool getCOEnabled(uint8_t);

    bool loadHexFile(QString filename);
    QVector<uint8_t> getHexData();

private:
    QString manufacturer;
    uint16_t manufacturerID;
    QList<KDeviceInfo*> compatibleDevices;

    QString name;
    QMap<QString,Parameter*> parameters;
    Dependencies dependencies;
    ParamTypes paramtypes;
    QMap<uint8_t,CommObject*> commobjs;
    QMap<CommObject*,uint8_t> primaryGA;
    uint16_t pa;
    QVector<uint8_t> access_key;
    uint8_t addrtable_size, assoctable_size, objtable_size;
    uint16_t addrtable_address, assoctable_address, objtable_address;

    void genTables(QList<uint16_t> &addrtable, QList<uint16_t> &assoctable, QList<uint8_t> &objtable);

    QVector<uint8_t> hexdata;
};


#endif // KDEVICE_H
