#include <iterator>
#include <stdint.h>
#include <stdio.h>
#include <QStringList>
#include "kdevice.h"
#include "parameter.h"
#include "util.h"

KDevice::KDevice(QDomElement elem) {
    access_key.clear();
    commobjs.clear();
    addrtable_size =  assoctable_size = 0;
    addrtable_address = assoctable_address = 0;

    bool valid_tag;

    if (elem.isNull()) {
        return;
    }


    valid_tag = false;
    if ((elem.tagName().compare("device-description") == 0) && elem.hasAttribute("version") && (elem.attribute("version").compare("1.0") == 0)) {
        valid_tag = true;
    }
    if ((elem.tagName().compare("device") == 0) && (elem.hasAttribute("address"))) {
        valid_tag = true;
    }
    if (!valid_tag) {
        return;
    }
    /* PA */
    if (elem.tagName().compare("device") == 0) {
        if (!elem.hasAttribute("address")) {
            return;
        }
        setPA(pa2uint(elem.attribute("address")));
    } else {
        setPA(0);
    }
    printf("!! %s\n",elem.tagName().toLatin1().data());

    /* Device header data */
    QDomElement e_name = elem.firstChildElement("name");
    if (!e_name.isNull()) {
        name = QString(e_name.text());
    } else {
        name = QString("");
    }
    QDomElement e_manuf = elem.firstChildElement("manufacturer");
    if (!e_manuf.isNull()) {
        manufacturer = QString(e_manuf.text());
    } else {
        manufacturer = QString("");
    }
    QDomElement e_manid = elem.firstChildElement("manufacturer-id");
    if (!e_manid.isNull()) {
        manufacturerID = hex_or_dez2int(e_manid.text());
    } else {
        manufacturerID = 0;
    }

    /* List of compatible devices */
    QDomElement e_devices = elem.firstChildElement("devices");
    if (!e_devices.isNull()) {
        for (QDomElement e_device = e_devices.firstChildElement("device"); !e_device.isNull(); e_device = e_device.nextSiblingElement("device")) {
            if (e_device.hasAttribute("order-info") && e_device.hasAttribute("mask-version") && e_device.hasAttribute("app-version")) {
                compatibleDevices.append(new KDeviceInfo(e_device.attribute("order-info"),e_device.attribute("mask-version"),e_device.attribute("app-version")));
            }
        }
    }

    /* Communication Objects */
    QDomElement e_co = elem.firstChildElement("commobjects");
    if (!e_co.isNull()) {
        objtable_size = 0;
        for (QDomElement e_obj = e_co.firstChildElement("commobject"); !e_obj.isNull(); e_obj = e_obj.nextSiblingElement("commobject")) {
            if (e_obj.attribute("id").isNull()) {
                continue;
            }
            if (e_obj.attribute("name").isNull()) {
                continue;
            }
            uint8_t co_id = hex_or_dez2int(e_obj.attribute("id"));
            CommObject *co = new CommObject(e_obj);
            commobjs.insert(co_id,co);
            dependencies.addDependency(new Dependency(co,e_obj));
            if (co_id >= objtable_size) {
                objtable_size = co_id + 1;
            }
        }
    }

    /* Parameter Types */
    QDomElement e_paramtypes = elem.firstChildElement("parameter-types");
    if (!e_paramtypes.isNull()) {
        for (QDomElement e_paramtype = e_paramtypes.firstChildElement("parameter-type"); !e_paramtype.isNull(); e_paramtype = e_paramtype.nextSiblingElement("parameter-type")) {
            paramtypes.add(e_paramtype);
        }
    }

    /* Parameters */
    QDomElement e_params = elem.firstChildElement("parameters");
    if (!e_params.isNull()) {
        for (QDomElement e_param = e_params.firstChildElement("parameter"); !e_param.isNull(); e_param = e_param.nextSiblingElement("parameter")) {
            Parameter *param = new Parameter(e_param);
            if (param->checkPDT_DPT()) {
                parameters.insert(e_param.attribute("id"),param);
                dependencies.addDependency(new Dependency(param,e_param));
                paramtypes.addParam(param,e_param);
            } else {
                printf("WARNING: PDT %s and DPT %i do not match for %s, parameter not handled\n",pdtToString(param->getPDT()).toLocal8Bit().data(),param->getDPT().toLocal8Bit().data(),e_param.attribute("id").data());
            }
        }
        dependencies.propagateValues(NULL);
    }

    /* Resources */
    QDomElement e_resources = elem.firstChildElement("resources");
    if (!e_resources.isNull()) {
        for (QDomElement e_resource = e_resources.firstChildElement("resource"); !e_resource.isNull(); e_resource = e_resource.nextSiblingElement("resource")) {
            if (e_resource.attribute("name").compare("GroupAddressTable") == 0) {
                if (e_resource.attribute("location").compare("memory") != 0) {
                    printf("WARNING: Unsupported location for Group Address Table (must be \"memory\"\n");
                    continue;
                } else {
                    addrtable_size = hex_or_dez2int(e_resource.attribute("size"));
                    addrtable_address = hex_or_dez2int(e_resource.attribute("address"));
                }
            } else if (e_resource.attribute("name").compare("GroupAssociationTable") == 0) {
                if (e_resource.attribute("location").compare("memory") != 0) {
                    printf("WARNING: Unsupported location for Association Table (must be \"memory\"\n");
                    continue;
                } else {
                    assoctable_size = hex_or_dez2int(e_resource.attribute("size"));
                    assoctable_address = hex_or_dez2int(e_resource.attribute("address"));
                }
            } else if (e_resource.attribute("name").compare("GroupObjectTable") == 0) {
                if (e_resource.attribute("location").compare("memory") != 0) {
                    printf("WARNING: Unsupported location for Object Table (must be \"memory\"\n");
                    continue;
                } else {
                    objtable_address = hex_or_dez2int(e_resource.attribute("address"));
                }
            } else if (e_resource.attribute("name").compare("AccessKey") == 0) {
                if ((e_resource.attribute("location").compare("value") != 0) || (e_resource.text().length() == 0)) {
                    printf("WARNING: Unsupported location for Access Key (must be \"value\"\n");
                    continue;
                } else {
                    access_key.clear();
                    QStringList kl = e_resource.text().split(" ");
                    for (QStringList::iterator iter = kl.begin(); iter != kl.end(); iter++) {
                        access_key.append(iter->toUInt(0,0x10));
                    }
                }
            }
        }
        return;
    }
}

QDomElement KDevice::toXML(QDomDocument doc) {
    QDomElement deviceElement = doc.createElement("device");
    deviceElement.setAttribute("address",uint2pa(pa).toLocal8Bit().data());
    if (name.compare("")) {
        QDomElement nameElement = doc.createElement("name");
        nameElement.appendChild(doc.createTextNode(name));
        deviceElement.appendChild(nameElement);
    }

    QDomElement e_manid = doc.createElement("manufacturer-id");
    deviceElement.appendChild(e_manid);
    e_manid.appendChild(doc.createTextNode(QString("0x").append(QString::number(manufacturerID,0x10))));

    if (!manufacturer.isNull() && (manufacturer.length() > 0)) {
        QDomElement e_manuf = doc.createElement("manufacturer");
        deviceElement.appendChild(e_manuf);
        e_manuf.appendChild(doc.createTextNode(manufacturer));
    }

    if (compatibleDevices.length() > 0) {
        QDomElement e_devices = doc.createElement("devices");
        deviceElement.appendChild(e_devices);
        for (QList<KDeviceInfo*>::iterator i_dev = compatibleDevices.begin(); i_dev != compatibleDevices.end(); i_dev++) {
            e_devices.appendChild((*i_dev)->toXML(doc));
        }
    }

    QDomElement e_params = doc.createElement("parameters");
    deviceElement.appendChild(e_params);
    for (QMap<QString,Parameter*>::iterator iter = parameters.begin(); iter != parameters.end(); iter++) {
        QDomElement e_param = iter.value()->toXML(doc);
        QDomElement e_condition = dependencies.condition(doc,iter.value());
        if (!e_condition.isNull()) {
            e_param.appendChild(e_condition);
        }
        QDomElement e_paramtype = paramtypes.paramXML(doc,iter.value());
        if (!e_paramtype.isNull()) {
            e_param.appendChild(e_paramtype);
        }
        e_params.appendChild(e_param);
    }

    QDomElement e_paramtypes = paramtypes.toXML(doc);
    if (!e_paramtypes.isNull()) {
        deviceElement.appendChild(e_paramtypes);
    }

    if (!commobjs.isEmpty()) {
        QDomElement coRootElement = doc.createElement("commobjects");
        deviceElement.appendChild(coRootElement);


        for (QMap<uint8_t,CommObject*>::iterator iter = commobjs.begin(); iter != commobjs.end(); iter++) {
            QDomElement e_co = (*iter)->toXML(doc);
            QDomElement e_condition = dependencies.condition(doc,*iter);
            if (!e_condition.isNotation()) {
                e_co.appendChild(e_condition);
            }
            coRootElement.appendChild(e_co);
        }
    }

    QDomElement resourcesElement = doc.createElement("resources");
    deviceElement.appendChild(resourcesElement);

    QDomElement e_addrtable = doc.createElement("resource");
    resourcesElement.appendChild(e_addrtable);
    e_addrtable.setAttribute("access","ReadWrite");
    e_addrtable.setAttribute("location","memory");
    e_addrtable.setAttribute("name","GroupAddressTable");
    e_addrtable.setAttribute("address", QString("0x").append(QString::number(addrtable_address,0x10)));
    e_addrtable.setAttribute("count", QString::number(addrtable_size));

    QDomElement e_assoctable = doc.createElement("resource");
    resourcesElement.appendChild(e_assoctable);
    e_assoctable.setAttribute("access","ReadWrite");
    e_assoctable.setAttribute("location","memory");
    e_assoctable.setAttribute("name","GroupAssociationTable");
    e_assoctable.setAttribute("address", QString("0x").append(QString::number(assoctable_address,0x10)));
    e_assoctable.setAttribute("count", QString::number(assoctable_size));

    QDomElement e_objtable = doc.createElement("resource");
    resourcesElement.appendChild(e_objtable);
    e_objtable.setAttribute("access","ReadWrite");
    e_objtable.setAttribute("location","memory");
    e_objtable.setAttribute("name","GroupObjectTable");
    e_objtable.setAttribute("address", QString("0x").append(QString::number(objtable_address,0x10)));

    if (access_key.count() > 0) {
        QDomElement e_key = doc.createElement("resource");
        resourcesElement.appendChild(e_key);
        e_key.setAttribute("location","value");
        e_key.setAttribute("name","AccessKey");
        QString keystring = "";
        for (int n=0; n < access_key.length(); n++) {
            keystring = keystring.append(QString::number(access_key[n] & 0xff,16).toUpper()).append(" ");
        }
        e_key.appendChild(doc.createTextNode(keystring.trimmed()));
    }

    return deviceElement;
}


void KDevice::setPA(uint16_t pa) {
    this->pa = pa;
}

QString KDevice::getPA() {
    return uint2pa(pa);
}

void KDevice::setName(QString name) {
    this->name = name;
}


QString KDevice::getName() {
    return name;
}

unsigned long KDevice::getAccessKey() {
    unsigned long ret = 0;
    for (QVector<uint8_t>::iterator iter = access_key.begin(); iter != access_key.end(); iter++) {
        ret <<= 8;
        ret |= *iter;
    }
    return ret;
}

bool KDevice::operator< (KDevice d2) {
    return (pa < d2.pa);
}

bool KDevice::operator == (KDevice d2) {
    return (pa == d2.pa);
}

QStringList KDevice::coList() {
    QStringList ret;

    for (QMap<uint8_t,CommObject*>::iterator co = commobjs.begin(); co != commobjs.end(); co++) {
        ret.append(QString::number(co.key()).append(" ").append((*co)->getDescription()));
    }
    return ret;
}

QList<uint8_t> KDevice::coListUint() {
    QList<uint8_t> ret;

    for (QMap<uint8_t,CommObject*>::iterator co = commobjs.begin(); co != commobjs.end(); co++) {
        ret.append(co.key());
    }
    return ret;
}


void KDevice::addAssoc(uint8_t co, uint16_t ga) {
    if (!commobjs.contains(co)) {
        return;
    }
    commobjs[co]->addAssociation(ga);
}

void KDevice::removeAssoc(uint8_t co, uint16_t ga) {
    Logger::log("Device %s: removing association %i -> %s\n",uint2pa(pa).toLatin1().data(),co,uint2ga(ga).toLatin1().data());
    if (!commobjs.contains(co)) {
        return;
    }
    commobjs[co]->removeAssociation(ga);
}

void KDevice::gaMakePrimary(uint8_t co, uint16_t ga) {
    if (!commobjs.contains(co)) {
        return;
    }
    commobjs[co]->makePrimary(ga);
}

QStringList KDevice::assocList(uint8_t co) {
    QStringList ret;

    ret.clear();
    if (!commobjs.contains(co)) {
        return ret;
    }
    QList<uint16_t> assocs = commobjs[co]->getAssociations();
    for (QList<uint16_t>::iterator ga = assocs.begin(); ga != assocs.end(); ga++) {
        ret.append(uint2ga(*ga));
    }
    return ret;
}

QList<uint16_t> KDevice::addrtable() {
    QList<uint16_t> ret, dummy;
    QList<uint8_t> objdummy;
    genTables(ret,dummy,objdummy);
    return ret;
}

QList<uint16_t> KDevice::assoctable() {
    QList<uint16_t> ret, dummy;
    QList<uint8_t> objdummy;
    genTables(dummy,ret,objdummy);
    return ret;
}

QList<uint8_t> KDevice::objtable() {
    QList<uint16_t> dummy, assocdummy;
    QList<uint8_t> ret;
    genTables(dummy,assocdummy,ret);
    return ret;
}

void KDevice::genTables(QList<uint16_t> &addrtable, QList<uint16_t> &assoctable, QList<uint8_t> &objtable) {
    addrtable.clear();

    for (QMap<uint8_t,CommObject*>::iterator i_co = commobjs.begin(); i_co != commobjs.end(); i_co++) {
        CommObject *co = i_co.value();
        if (!dependencies.isEnabled(co)) {
            continue;
        }
        QList<uint16_t>assocs = co->getAssociations();
        for (QList<uint16_t>::iterator i_ga = assocs.begin(); i_ga != assocs.end(); i_ga++) {
            uint16_t ga = *i_ga;
            if (!addrtable.contains(ga)) {
                addrtable.append(ga);
            }
//            printf("%04X @ %i\n",ga,addrtable.indexOf(ga)+1); fflush(stdout);
            uint16_t val;
            val = co->getID();
            val |= ((uint8_t)(addrtable.indexOf(ga)+1)) << 8;
            assoctable.append(val);
        }
    }

    for (uint8_t n=0; n < objtable_size; n++) {
        if (commobjs.keys().contains(n)) {
            objtable.append(commobjs[n]->getFlags());
        } else {
            objtable.append(0xFF);
        }
    }
}

uint8_t KDevice::getFlags(uint8_t co) {
    if (!commobjs.contains(co)) {
        return 0;
    }
    return commobjs[co]->getFlags();
}

void KDevice::setFlags(uint8_t co, uint8_t flags) {
    if (!commobjs.contains(co)) {
        return;
    }
    commobjs[co]->setFlags(flags);
}

QMap<QString,Parameter*> KDevice::getParameters() {
    return parameters;
}

bool KDevice::setParameter(QString id, QString value) {
    bool ret;
    if (!parameters.contains(id)) {
        printf("Parameter %s not found\n",id.toLatin1().data());
        return false;
    }
    Parameter *p = parameters[id];
    if (paramtypes.inRange(p,value)) {
        ret = p->setData(value);
        dependencies.propagateValues(NULL);
        return ret;
    }
    return false;
}

bool KDevice::getParameterEnabled(Parameter* param) {
    return dependencies.isEnabled(param);
}

bool KDevice::getParameterEnabled(QString id) {
    if (parameters.contains(id)) {
        return dependencies.isEnabled(parameters[id]);
    } else {
        return false;
    }
}

QList<QString> KDevice::getParamKeys() {
    QList<Dependency*> deps = dependencies.flatTree(NULL);
    QList<QString> keys;

    for (QList<Dependency*>::iterator iter = deps.begin(); iter != deps.end(); iter++) {
        if (!(*iter)->getParam()) {
            continue;
        }
        keys.append((*iter)->getParam()->getID());
    }
    return keys;
}

bool KDevice::getCOEnabled(CommObject* co) {
    return dependencies.isEnabled(co);
}

bool KDevice::getCOEnabled(uint8_t co_idx) {
    if (!commobjs.contains(co_idx)) {
        return false;
    }
    CommObject *co = commobjs[co_idx];
    return dependencies.isEnabled(co);
}


bool KDevice::loadHexFile(QString filename) {
    QList<uint8_t> hexdata;
    QFile *file = new QFile(filename);

    if (!file->open(QIODevice::ReadOnly)) {
        return false;
    }

    char c;
    bool ok;
    while (true) {
        QString s;
        if (!(file->getChar(&c))) {
            break;
        }
        s.append(c);
        if (!(file->getChar(&c))) {
            file->close();
            return false;
        }
        s.append(c);
        uint8_t h = (uint8_t)(s.toUInt(&ok,16));
        if (!ok) {
            file->close();
            return false;
        }
        hexdata.append(h);

        if (!file->getChar(&c)) {
            break;
        }
        if (c != ' ') {
            file->close();
            return false;
        }
    }

    this->hexdata.clear();
    for (uint16_t n=0; n < hexdata.length(); n++) {
        this->hexdata.append(hexdata.at(n));
    }
    printf("--- %i\n",this->hexdata.length());
    return true;
}

QVector<uint8_t> KDevice::getHexData() {
    return hexdata;
}

uint16_t KDevice::hex_or_dez2int(QString hex) {
    QString hex2 = hex.trimmed();
    if (hex2.toLower().startsWith("0x")) {
        hex2 = hex2.right(hex2.length()-2);
        return hex2.toInt(0,16);
    } else {
        return hex2.toInt(0,10);
    }
}

KDeviceInfo::KDeviceInfo(QString order_info, QString mask_version, QString app_version) {
    uint8_t n;

    if (order_info.toLower().startsWith("0x")) {
        order_info = order_info.mid(2);
    }
    if ((order_info.length() % 2) == 1) {
        order_info = order_info.prepend('0');
    }
    while (order_info.length() > 0) {
        this->order_info.append(order_info.left(2).toUInt(NULL,0x10));
        order_info = order_info.mid(2);
    }
    while ((this->order_info.at(0) == 0) && (this->order_info.length() > 1)) {
        this->order_info.removeAt(0);
    }

    if (mask_version.toLower().startsWith("0x")) {
        mask_version = mask_version.mid(2);
    }
    this->mask_version = mask_version.toUInt(NULL,0x10);

    if (app_version.toLower().startsWith("0x")) {
        app_version = app_version.mid(2);
    }
    if ((app_version.length() % 2) == 1) {
        app_version = app_version.prepend('0');
    }
    while (app_version.length() > 0) {
        this->app_version.append(app_version.left(2).toUInt(NULL,0x10));
        app_version = app_version.mid(2);
    }
    while ((this->app_version.at(0) == 0) && (this->app_version.length() > 1)) {
        this->app_version.removeAt(0);
    }
}

bool KDeviceInfo::matches(QList<uint8_t> order_info, uint8_t mask_version, QList<uint8_t> app_version) {
    uint8_t n;
    if ((this->order_info.length() == 0) && (order_info.length() > 0)) {
        return false;
    }
    if ((this->app_version.length() == 0) && (app_version.length() > 0)) {
        return false;
    }
    if (mask_version != this->mask_version) {
        return false;
    }
    if (order_info.length() > 0) {
        while ((order_info.at(0) == 0) && (order_info.length() > 1)) {
            order_info.removeAt(0);
        }
        if (order_info.length() != this->order_info.length()) {
            return false;
        }
        for (n=0; n < order_info.length(); n++) {
            if (order_info.at(n) != this->order_info.at(n)) {
                return false;
            }
        }
    }
    if (app_version.length() > 0) {
        while ((app_version.at(0) == 0) && (app_version.length() > 1)) {
            app_version.removeAt(0);
        }
        if (app_version.length() != this->app_version.length()) {
            return false;
        }
        for (n=0; n < app_version.length(); n++) {
            if (app_version.at(n) != this->app_version.at(n)) {
                return false;
            }
        }
    }
    return true;
}

QDomElement KDeviceInfo::toXML(QDomDocument doc) {
    QDomElement ret = doc.createElement("device");
    ret.setAttribute("mask-version",QString("0x").append(QString::number(mask_version,0x10)));

    QString appver = QString("0x");
    for (QList<uint8_t>::iterator iter = app_version.begin(); iter != app_version.end(); iter++) {
        QString part = QString::number(*iter,0x10);
        while (part.length() < 2) {
            part = QString("0").append(part);
        }
        appver.append(part);
    }
    ret.setAttribute("app-version",appver);

    QString orderinfo = QString("0x");
    for (QList<uint8_t>::iterator iter = order_info.begin(); iter != order_info.end(); iter++) {
        QString part = QString::number(*iter,0x10);
        while (part.length() < 2) {
            part = QString("0").append(part);
        }
        orderinfo.append(part);
    }
    ret.setAttribute("order-info",orderinfo);

    return ret;
}
