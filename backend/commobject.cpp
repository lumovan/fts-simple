#include "commobject.h"
#include "parameter.h"
#include "util.h"

CommObject::CommObject(QDomElement elem) {
    if (!elem.hasAttribute("id") || (!elem.hasAttribute("name"))) {
        return;
    }
    if (elem.firstChildElement("pdt").isNull()) {
        return;
    }

    id = elem.attribute("id").toUInt();
    name = elem.attribute("name");
    pdt = pdtFromString(elem.firstChildElement("pdt").text());

    if (!elem.firstChildElement("description").isNull()) {
        description = elem.firstChildElement("description").text();
    }
    if (!elem.firstChildElement("dpt").isNull()) {
        dpt = elem.firstChildElement("dpt").text();
    }
    if (!elem.firstChildElement("flags").isNull()) {
        flags = elem.firstChildElement("flags").text().toUInt(0,0x10);
    }

    QDomElement e_assoc = elem.firstChildElement("associations");
    if (e_assoc.isNull()) {
        return;
    }

    for (QDomElement e_ga = e_assoc.firstChildElement("association"); !e_ga.isNull(); e_ga = e_ga.nextSiblingElement("association")) {
        associations.append(ga2uint(e_ga.text()));
    }
}

QDomElement CommObject::toXML(QDomDocument doc) {
    QDomElement elem = doc.createElement("commobject");
    elem.setAttribute("id",id);
    elem.setAttribute("name",name);

    QDomElement e_pdt = doc.createElement("pdt");
    elem.appendChild(e_pdt);
    e_pdt.appendChild(doc.createTextNode(pdtToString(pdt)));

    QDomElement e_flags = doc.createElement("flags");
    elem.appendChild(e_flags);
    e_flags.appendChild(doc.createTextNode(QString::number(flags,0x10)));

    if (!description.isNull()) {
        QDomElement e_desc = doc.createElement("description");
        elem.appendChild(e_desc);
        e_desc.appendChild(doc.createTextNode(description));
    }
    if (!dpt.isNull()) {
        QDomElement e_dpt = doc.createElement("dpt");
        elem.appendChild(e_dpt);
        e_dpt.appendChild(doc.createTextNode(dpt));
    }

    if (associations.count() == 0) {
        return elem;
    }

    QDomElement e_assoc = doc.createElement("associations");
    elem.appendChild(e_assoc);
    for (QList<uint16_t>::iterator ga = associations.begin(); ga != associations.end(); ga++) {
        QDomElement e_ga = doc.createElement("association");
        e_assoc.appendChild(e_ga);
        e_ga.appendChild(doc.createTextNode(uint2ga(*ga)));
    }

    return elem;

}

uint8_t CommObject::getID() {
    return id;
}

QString CommObject::getName() {
    return name;
}

QString CommObject::getDescription() {
    return description;
}

QString CommObject::getDPT() {
    return dpt;
}

EibPropertyDataType_t CommObject::getPDT() {
    return pdt;
}

QList<uint16_t> CommObject::getAssociations() {
    return associations;
}

bool CommObject::addAssociation(uint16_t ga) {
    if (associations.contains(ga)) {
        return false;
    }
    associations.append(ga);
    return true;
}

bool CommObject::removeAssociation(uint16_t ga) {
    if (associations.contains(ga)) {
        return associations.removeOne(ga);
    }
    return false;
}

void CommObject::makePrimary(uint16_t ga) {
    if (!associations.contains(ga)) {
        return;
    }
    associations.move(associations.indexOf(ga),0);
}


void CommObject::setFlags(uint8_t flags) {
    this->flags = flags;
}

uint8_t CommObject::getFlags() {
    return flags;
}
