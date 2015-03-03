#ifndef COMMOBJECT_H
#define COMMOBJECT_H

#include <QtXml>
#include "propertydatatype.h"

class CommObject {
public:
    CommObject(QDomElement elem);
    QDomElement toXML(QDomDocument doc);

    uint8_t getID();
    QString getName();
    QString getDescription();
    QString getDPT();
    EibPropertyDataType_t getPDT();
    QList<uint16_t> getAssociations();
    bool addAssociation(uint16_t);
    bool removeAssociation(uint16_t);
    void makePrimary(uint16_t ga);

    void setFlags(uint8_t flags);
    uint8_t getFlags();

private:
    uint8_t id;
    QString name;
    QString description;
    QString dpt;
    EibPropertyDataType_t pdt;
    QList<uint16_t> associations;
    uint8_t flags;
};

#endif // COMMOBJECT_H
