#include <QtXml>
#include "paramtypes.h"

ParamType::ParamType(QDomElement elem) {
    name = elem.attribute("name");
    if (!elem.firstChildElement("enum").isNull()) {
        for (QDomElement e_pt = elem.firstChildElement("enum"); !e_pt.isNull(); e_pt = e_pt.nextSiblingElement("enum")) {
            if (e_pt.hasAttribute("text") && e_pt.hasAttribute("value")) {
                itemMap.insert(e_pt.attribute("text"),e_pt.attribute("value"));
                if (e_pt.hasAttribute("display-order")) {
                    displayOrder.insert(e_pt.attribute("text"),e_pt.attribute("display-order").toUInt());
                }
#if 0
                printf("item map %s insert: %s %s\n",
                       name.toLatin1().data(),
                       e_pt.attribute("text").toLatin1().data(),
                       e_pt.attribute("value").toLatin1().data());
#endif
            }
        }
        return;
    }
    if (!elem.firstChildElement("min-value").isNull()) {
        minValue = elem.firstChildElement("min-value").text();
    }
    if (!elem.firstChildElement("max-value").isNull()) {
        maxValue = elem.firstChildElement("max-value").text();
    }
}

QDomElement ParamType::toXML(QDomDocument doc) {
    QDomElement elem;
    elem = doc.createElement("parameter-type");
    elem.setAttribute(QString("name"),name);
    if (items().isEmpty()) {
        if (!minValue.isEmpty()) {
            QDomElement e_min = doc.createElement("min-value");
            e_min.appendChild(doc.createTextNode(minValue));
            elem.appendChild(e_min);
        }
        if (!maxValue.isEmpty()) {
            QDomElement e_max = doc.createElement("max-value");
            e_max.appendChild(doc.createTextNode(maxValue));
            elem.appendChild(e_max);
        }
        return elem;
    }

//    printf("I %i\n",itemMap.count()); fflush(stdout);
    for (QMap<QString,QString>::iterator iter = itemMap.begin(); iter != itemMap.end(); iter++) {
        QDomElement e_enum = doc.createElement("enum");
//printf("II %s\n",iter.key().toLatin1().data());
        if (displayOrder.contains(iter.key())) {
            e_enum.setAttribute("display-order",displayOrder[iter.key()]);
        }
        e_enum.setAttribute("text",iter.key());
        e_enum.setAttribute("value",*iter);
        elem.appendChild(e_enum);
    }
    return elem;
}


QString ParamType::list2value(QString listentry) {
    if (itemMap.isEmpty()) {
//        printf("ie\n");
        return QString();
    }
    if (itemMap.contains(listentry)) {
        return itemMap[listentry];
    }
    return QString();
}

QString ParamType::value2list(QString value) {
    if (itemMap.isEmpty()) {
        return QString();
    }
    for (QMap<QString,QString>::iterator iter = itemMap.begin(); iter != itemMap.end(); iter++) {
        if (iter->compare(value) == 0) {
            return iter.key();
        }
    }
    return QString();
}

QStringList ParamType::items() {
    return itemMap.keys();
}


bool ParamType::inRange(Parameter *param, QString data) {
    uint8_t val_i = 0;
    QString val = this->list2value(data);
    if (val.isNull()) {
        val = data;
    }

    QStringList dpts = param->getDPT().split(".");
    uint8_t type = dpts[0].toUInt();
    uint8_t subtype = 0;
    if (dpts.length() > 1) {
        subtype = dpts[1].toUInt();
    }

    bool ok;

    switch(type) {
    case 1:
        if (val.toInt(&ok) > 1) {
            return false;
        }
        if (!ok) {
            return false;
        }
        if (val.toInt() < 0) {
            return false;
        }
        if (!maxValue.isNull() && (maxValue.toUInt() < val.toUInt())) {
            return false;
        }
        if (!minValue.isNull() && (minValue.toUInt() > val.toUInt())) {
            return false;
        }
        return true;

    case 2:
        if (val.toInt(&ok) > 3) {
            return false;
        }
        if (!ok) {
            return false;
        }
        if (val.toInt() < 0) {
            return false;
        }
        if (!maxValue.isNull() && (maxValue.toUInt() < val.toUInt())) {
            return false;
        }
        if (!minValue.isNull() && (minValue.toUInt() > val.toUInt())) {
            return false;
        }
        return true;

    case 3:
        if (val.toInt(&ok) > 0x0F) {
            return false;
        }
        if (!ok) {
            return false;
        }
        if (val.toInt() < 0) {
            return false;
        }
        if (!maxValue.isNull() && (maxValue.toUInt() < val.toUInt())) {
            return false;
        }
        if (!minValue.isNull() && (minValue.toUInt() > val.toUInt())) {
            return false;
        }
        return true;
    case 4:
        if (val.length() > 1) {
            return false;
        }
        val_i = val.at(0).digitValue();
        if (!maxValue.isNull() && (maxValue.toUInt() < val_i)) {
            return false;
        }
        if (!minValue.isNull() && (minValue.toUInt() > val_i)) {
            return false;
        }
    case 5:
        if (val.toInt(&ok) > 0xFF) {
            return false;
        }
        if (!ok) {
            return false;
        }
        if (val.toInt() < 0) {
            return false;
        }
        if (!maxValue.isNull() && (maxValue.toUInt() < val.toUInt())) {
            return false;
        }
        if (!minValue.isNull() && (minValue.toUInt() > val.toUInt())) {
            return false;
        }
        return true;
    case 6:
        if (val.toInt(&ok) > 127) {
            return false;
        }
        if (!ok) {
            return false;
        }
        if (val.toInt() < -128) {
            return false;
        }
        if (!maxValue.isNull() && (maxValue.toInt() < val.toInt())) {
            return false;
        }
        if (!minValue.isNull() && (minValue.toInt() > val.toInt())) {
            return false;
        }
        return true;
    case 7:
        if (val.toULong(&ok) > 0xFFFF) {
            return false;
        }
        if (!ok) {
            return false;
        }
        if (!maxValue.isNull() && (maxValue.toULong() < val.toULong())) {
            return false;
        }
        if (!minValue.isNull() && (minValue.toULong() > val.toULong())) {
            return false;
        }
        return true;
    case 8:
        if (val.toLong(&ok) > 32767) {
            return false;
        }
        if (!ok) {
            return false;
        }
        if (val.toLong() < -32768) {
            return false;
        }
        if (!maxValue.isNull() && (maxValue.toLong() < val.toLong())) {
            return false;
        }
        if (!minValue.isNull() && (minValue.toLong() > val.toLong())) {
            return false;
        }
        return true;
    case 9:
        if (val.toDouble(&ok) > 670760.96) {
            return false;
        }
        if (!ok) {
            return false;
        }
        if (val.toDouble() < -671088.64) {
            return false;
        }
        if (!maxValue.isNull() && (maxValue.toDouble() < val.toDouble())) {
            return false;
        }
        if (!minValue.isNull() && (minValue.toDouble() > val.toDouble())) {
            return false;
        }
        return true;
    default:
        return true;
    }
}

ParamTypes::ParamTypes() {

}

bool ParamTypes::inRange(Parameter *param, QString data) {
    if (param->getDPT().isNull()) {
        return true;
    }

    if (!typeMap.contains(param)) {
        return true;
    }
    ParamType pt = typeMap.value(param);

    return pt.inRange(param,data);
}

QString ParamTypes::list2value(Parameter *param,QString listentry) {
//    printf("l2v %s %s\n",param->getID().toLocal8Bit().data(),listentry.toLatin1().data());
    if (!typeMap.contains(param)) {
        QStringList dpts = param->getDPT().split(".");
        if ((dpts.count() < 2) || (dpts[0].toUInt() != 1)) {
            return QString();
        }
        QStringList entries = dpt1strings(dpts[1].toUInt());
        if (entries.isEmpty()) {
            return QString();
        }
        if (entries[0].compare(listentry) == 0) {
            return QString("0");
        }
        if (entries[1].compare(listentry) == 0) {
            return QString("1");
        }
        return QString();
    }
//    printf("contains\n");
    ParamType pt = typeMap.value(param);
    return pt.list2value(listentry);
}

QString ParamTypes::value2list(Parameter *param,QString value) {
    if (!typeMap.contains(param)) {
        QStringList dpts = param->getDPT().split(".");
        if ((dpts.count() < 2) || (dpts[0].toUInt() != 1)) {
            return QString();
        }
        QStringList entries = dpt1strings(dpts[1].toUInt());
        if (entries.isEmpty()) {
            return QString();
        }
        if (value.toUInt() == 0) {
            return entries[0];
        }
        if (value.toUInt() == 1) {
            return entries[1];
        }
        return QString();
    }
    ParamType pt = typeMap.value(param);
    return pt.value2list(value);
}

QStringList ParamTypes::items(Parameter *param) {
//    printf("PTI: %s %i\n",param->getID().toLatin1().data(),typeMap.contains(param)); fflush(stdout);
    if (!typeMap.contains(param)) {
        QStringList dpts = param->getDPT().split(".");
        if ((dpts.count() < 2) || (dpts[0].toUInt() != 1)) {
            return QStringList();
        }
//        printf("$1.%s",dpts[1].toLatin1().data());
        return dpt1strings(dpts[1].toUInt());
    }
    return typeMap[param].items();
}

QDomElement ParamTypes::toXML(QDomDocument doc) {
    if (types.isEmpty()) {
        return QDomElement();
    }

    QDomElement elem;
    elem = doc.createElement("parameter-types");

    for (QList<ParamType>::iterator iter = types.begin(); iter != types.end(); iter++) {
        elem.appendChild(iter->toXML(doc));
    }
    return elem;
}

void ParamTypes::add(QDomElement elem) {
    if ((elem.nodeName().compare(QString("parameter-type")) != 0) || (!elem.hasAttribute("name"))) {
        return;
    }
    types.append(ParamType(elem));
}

void ParamTypes::addParam(Parameter *param, QDomElement e_param) {
    if (e_param.firstChildElement("parameter-type").isNull()) {
        return;
    }
    for (QList<ParamType>::iterator iter = types.begin(); iter != types.end(); iter++) {
        if (e_param.firstChildElement("parameter-type").text().compare(iter->getName()) == 0) {
            typeMap.insert(param,*iter);
            break;
        }
    }
}

QDomElement ParamTypes::paramXML(QDomDocument doc,Parameter *param) {
//    printf("pXML: %s\n",param->getID().toLatin1().data()); fflush(stdout);
    if (!typeMap.contains(param)) {
        return QDomElement();
    }
//    printf("YES!\n"); fflush(stdout);
    QDomElement ret = doc.createElement("parameter-type");
    ret.appendChild(doc.createTextNode(typeMap[param].getName()));
    return ret;
}


QStringList ParamTypes::dpt1strings(uint8_t subtype) {
    QStringList ret = QStringList();
    switch(subtype) {
    case 1:
        ret = QStringList() << QString("Off") << QString("On");
        break;
    case 2:
        ret = QStringList() << QString("False") << QString("True");
        break;
    case 3:
        ret = QStringList() << QString("Disable") << QString("Enable");
        break;
    case 4:
        ret = QStringList() << QString("No ramp") << QString("Ramp");
        break;
    case 5:
        ret = QStringList() << QString("No alarm") << QString("Alarm");
        break;
    case 6:
        ret = QStringList() << QString("Low") << QString("High");
        break;
    case 7:
        ret = QStringList() << QString("Decrease") << QString("Increase");
        break;
    case 8:
        ret = QStringList() << QString("Up") << QString("Down");
        break;
    case 9:
        ret = QStringList() << QString("Open") << QString("Close");
        break;
    case 10:
        ret = QStringList() << QString("Stop") << QString("Start");
        break;
    case 11:
        ret = QStringList() << QString("Inactive") << QString("Active");
        break;
    case 12:
        ret = QStringList() << QString("Not inverted") << QString("Inverted");
        break;
    case 15:
        ret = QStringList() << QString("No Action") << QString("Reset Command");
        break;
    case 16:
        ret = QStringList() << QString("No Action") << QString("Acknowledge");
        break;
    case 17:
        ret = QStringList() << QString("Trigger") << QString("Trigger");
        break;
    case 18:
        ret = QStringList() << QString("Not occupied") << QString("Occupied");
        break;
    case 19:
        ret = QStringList() << QString("Closed") << QString("Open");
        break;
    case 21:
        ret = QStringList() << QString("Logical OR") << QString("Logical AND");
        break;
    case 22:
        ret = QStringList() << QString("Scene A") << QString("Scene B");
        break;
    }
    return ret;
}
