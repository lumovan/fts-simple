#ifndef PARAMTYPES_H
#define PARAMTYPES_H

#include <QtXml>
#include "parameter.h"

class ParamType {
public:
    ParamType() {};
    ParamType(QDomElement elem);
    QString getName() { return name; }
    QString list2value(QString);
    QString value2list(QString);
    bool inRange(Parameter *param, QString data);

    QStringList items();
    QDomElement toXML(QDomDocument doc);

private:
    QMap<QString,QString> itemMap;
    QMap<QString,uint8_t> displayOrder;
    QString minValue, maxValue;
    QString name;
};

class ParamTypes {
public:
    ParamTypes();
    void add(QDomElement elem);
    void addParam(Parameter *param, QDomElement e_param);
    bool inRange(Parameter *param, QString data);
    QString list2value(Parameter *param,QString listentry);
    QString value2list(Parameter *param,QString value);
    QStringList items(Parameter *param);
    QDomElement toXML(QDomDocument doc);
    QDomElement paramXML(QDomDocument doc, Parameter *param);

private:
    QList<ParamType> types;
    QMap<Parameter*,ParamType> typeMap;
    static QStringList dpt1strings(uint8_t subtype);
};

#endif // PARAMTYPES_H
