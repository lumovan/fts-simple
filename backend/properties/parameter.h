#ifndef PARAMETER_H
#define PARAMETER_H

#include <QString>
#include <QVariant>
#include <QtXml>
#include <stdint.h>
#ifdef USE_FALCON_IF
#include "FalconInterfaceDefines.h"
#else
#include "propertydatatype.h"
#endif

typedef enum {
    param_pos_property,
    param_pos_memory
} param_pos_t;

class Parameter {
public:
    Parameter(QDomElement elem);

    bool setData(QString value);
    QString getData();

    QList<uint8_t> getValue() { return value; }

    void setUnit(QString unit);
    QString unit();

    bool isReadonly();

    QString getDescription();

    param_pos_t getParamPos();
    uint8_t getObjectID();
    uint8_t getPropertyID();
    uint16_t getAddress();
    QString getID();
    QString getPIDName();
    EibPropertyDataType getPDT();
    QString getDPT();
    bool checkPDT_DPT();

    QDomElement toXML(QDomDocument doc);
    static QStringList dpt1strings(uint8_t subtype);

private:
    QString id;
    QString description;

    struct {
        param_pos_t pos;
        union {
            struct {
                uint8_t obj_id;
                uint8_t property_id;
            } prop;
            uint16_t address;
        } location;
    } handle;

    EibPropertyDataType pdt;
    QString dpt;

    QString data;
    QString default_data;
    QString default_disabled_data;
    QList<uint8_t> value;
    bool readonly;

    bool convert(QList<uint8_t> in, QString &out);
    bool convert(QString in, QList<uint8_t> &out);
};

class PropertyKey {
public:
    PropertyKey(uint8_t obj, uint8_t id);

    uint8_t getObj();
    uint8_t getID();

    bool operator<(PropertyKey k2);

    bool operator==(PropertyKey k2);

private:
    uint8_t obj;
    uint8_t id;
};

class PropertyKeyCompare {
public:
    bool operator()(PropertyKey* k1, PropertyKey* k2);
};

EibPropertyDataType pdtFromString(QString pdt);
QString pdtToString(EibPropertyDataType pdt);

#endif // PARAMETER_H

