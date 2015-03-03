#include <QVariant>
#include "parameter.h"
#include "kdevice.h"

Parameter::Parameter(QDomElement elem) {
    if (elem.attribute("location").compare("properties") == 0) {
        handle.pos = param_pos_property;
        handle.location.prop.obj_id = KDevice::hex_or_dez2int(elem.attribute("object-id"));
        handle.location.prop.property_id = KDevice::hex_or_dez2int(elem.attribute("property-id"));
    } else if (elem.attribute("location").compare("memory") == 0) {
        handle.pos = param_pos_memory;
        handle.location.address = KDevice::hex_or_dez2int(elem.attribute("address"));
    } else {
        printf("WARNING: invalid location attribute %s for parameter %s\n",elem.attribute("location").toLatin1().data(),elem.attribute("id").toLatin1().data());
    }

    id = elem.attribute("id");
    readonly = (elem.attribute("access").compare("ReadWrite") != 0);

    if (!elem.firstChildElement("dpt").isNull()) {
        dpt = elem.firstChildElement("dpt").text();
    } else {
        dpt = "";
    }
    pdt = pdtFromString(elem.firstChildElement("pdt").text());
    if (!elem.firstChildElement("default").isNull()) {
        setData(elem.firstChildElement("default").text());
        default_data = elem.firstChildElement("default").text();
        default_disabled_data = elem.firstChildElement("default").text();
    }
    if (!elem.firstChildElement("default-disabled").isNull()) {
        default_disabled_data = elem.firstChildElement("default-disabled").text();
    }
    if (!elem.firstChildElement("value").isNull()) {
        setData(elem.firstChildElement("value").text());
    } else if (!default_data.isNull()) {
        setData(default_data);
    } else {
        setData("");
    }

    if (!elem.firstChildElement("description").isNull()) {
        description = elem.firstChildElement("description").text();
    }
}

QDomElement Parameter::toXML(QDomDocument doc) {
    QDomElement parameterElement = doc.createElement("parameter");
    parameterElement.setAttribute("id",id);
    if (handle.pos == param_pos_property) {
        parameterElement.setAttribute("location","properties");
        parameterElement.setAttribute("object-id",handle.location.prop.obj_id);
        parameterElement.setAttribute("property-id",handle.location.prop.property_id);
    } else if (handle.pos == param_pos_memory) {
        parameterElement.setAttribute("location","memory");
        parameterElement.setAttribute("address",QString("0x").append(QString::number(handle.location.address,0x10)));
    }
    parameterElement.setAttribute("access",readonly?"Read":"ReadWrite");

    QDomElement e_pdt = doc.createElement("pdt");
    e_pdt.appendChild(doc.createTextNode(pdtToString(pdt)));
    parameterElement.appendChild(e_pdt);

    if (getDescription().length() > 0) {
        QDomElement e_description = doc.createElement("description");
        e_description.appendChild(doc.createTextNode(getDescription()));
        parameterElement.appendChild(e_description);
    }

    if (!(dpt.isNull()) && (dpt.length() > 0)) {
        QDomElement e_description = doc.createElement("dpt");
        e_description.appendChild(doc.createTextNode(dpt));
        parameterElement.appendChild(e_description);
    }

    if (!default_data.isNull()) {
        QDomElement e_default = doc.createElement("default");
        e_default.appendChild(doc.createTextNode(default_data));
        parameterElement.appendChild(e_default);
    }
    if (!default_disabled_data.isNull() && (default_disabled_data.compare(default_data) != 0)) {
        QDomElement e_default_disabled = doc.createElement("default-disabled");
        e_default_disabled.appendChild(doc.createTextNode(default_disabled_data));
        parameterElement.appendChild(e_default_disabled);
    }
    if ((!data.isNull()) && (data.length() > 0) && (default_data.isNull() || (default_data.compare(data) != 0))) {
        QDomElement e_default = doc.createElement("value");
        e_default.appendChild(doc.createTextNode(data));
        parameterElement.appendChild(e_default);
    }
    return parameterElement;
}

bool Parameter::setData(QString data) {
    printf("conv: %s value #%s# DPT: %s in: %s out: ",id.toLocal8Bit().data(),data.toLocal8Bit().data(),dpt.toLatin1().data(),data.toLatin1().data()); fflush(stdout);
    if (convert(data,value)) {
        for (int n=0; n < value.size(); n++) {
            printf("%02X ",value.at(n));
        }
        printf("\n");fflush(stdout);
        this->data = data;
        return true;
    } else {
        printf("(failed)");
    }
    printf("\n");fflush(stdout);
    return false;
}

QString Parameter::getData() {
//    printf("getData: %s, data is %s, default is %s\n",id.toLatin1().data(),data.toLatin1().data(),default_data.toLatin1().data());
    if (!data.isNull()) {
        return data;
    } else {
        return default_data;
    }
}

bool Parameter::isReadonly() {
    return readonly;
}

QString Parameter::getDescription() {
    return description;
}

EibPropertyDataType Parameter::getPDT() {
    return pdt;
}

QString Parameter::getDPT() {
    printf("@@@ DPT #%s#\n",dpt.toLocal8Bit().data());
    return dpt;
}

QString Parameter::getID() {
    return id;
}

param_pos_t Parameter::getParamPos() {
    return handle.pos;
}

uint8_t Parameter::getObjectID() {
    return handle.location.prop.obj_id;
}

uint8_t Parameter::getPropertyID() {
    return handle.location.prop.property_id;
}

bool Parameter::convert(QList<uint8_t> in, QString &out) {
    QString dpt;
    uint16_t dpt_num, val, val16;
    uint8_t val8;
    QStringList dpts = this->dpt.split(".");
    if (dpts.size() > 1) {
        dpt = dpts[0];
    } else {
        dpt = this->dpt;
    }
    bool ok;

    if (in.length() == 0) {
        return false;
    }

    if ((dpt.isNull()) || (dpt.length() == 0)) {
        out.clear();
        for (QList<uint8_t>::iterator iter = in.begin(); iter != in.end(); iter++) {
            out.append(" ");
            out.append(QString::number(*iter,0x10));
        }
        return true;
    }

    dpt_num = dpt.toInt(&ok);
    if (!ok) {
        return false;
    }

    switch(dpt_num) {
    case 1:
        if (in.size() != 1) {
            return false;
        }
        if (in[0] > 0) {
            out = QString("1");
        } else {
            out = QString("0");
        }
        break;
    case 2:
        if (in.size() != 1) {
            return false;
        }
        switch (in[0] & 0x03) {
        case 0x00:
            out = QString("00");
            break;
        case 0x01:
            out = QString("01");
            break;
        case 0x02:
            out = QString("10");
            break;
        case 0x03:
            out = QString("11");
            break;
        default:
            return false;
        }
        break;
    case 4: /* Character Set */
        if (in.size() != 1) {
            return false;
        }
        out = QString((const char*)(in[0]));
        break;
    case 5: /* 8-bit unsigned */
        if (in.size() != 1) {
            return false;
        }
        val8 = in[0];
        out = QString::number(val8);
        break;
    case 6: /* Signed 8-bit */
        if (in.size() != 1) {
            return false;
        }
        val8 = in[0];
        out = QString::number(*((int8_t*)(&val8)));
        break;
    case 7: /* 16-bit unsigned */
        if (in.size() != 2) {
            return false;
        }
        val16 = (in[0] << 8) + in[1];
        out = QString::number(val16);
        break;
    case 8: /* 16-bit signed */
        if (in.size() != 2) {
            return false;
        }
        val16 = (in[0] << 8) + in[1];
        out = QString::number(*((int16_t*)(&val16)));
        break;
    case 9: /* KNX Float */
        uint8_t exp;
        double result;

        if (in.size() != 2) {
            return false;
        }
        val16 = (in[0] << 8) + in[1];

        result = (val16 & 0x87FF);
        exp = (val16 >> 11) & 0x0F;
        while (exp > 0) {
            result *= 2;
            exp--;
        }
        out = QString::number(result);
        break;
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
        break;
    case 42:
        if ((in.size() < 1) || (in.size() > 2)) {
            return false;
        }
        val = 0;
        for (int n = 0; n < in.size(); n++) {
            val <<= 8;
            val |= in[n];
        }
        out = QString::number((long)val,10);
        break;
    default:
        return false;
    }
    return true;
}

bool Parameter::convert(QString in, QList<uint8_t> &out) {
    QString dpt;
    uint16_t dpt_num;
    uint8_t val8;
    uint16_t val16;
    int16_t sval16;
    double val_d;
    QStringList dpts = this->dpt.split(".");
    if (dpts.size() > 1) {
        dpt = dpts[0];
    } else {
        dpt = this->dpt;
    }

    bool ok;

    if ((dpt.isNull()) || (dpt.length() == 0)) {
        ok = false;
        QStringList inl = in.split(" ");
        for (uint16_t n = 0; n < (sizeof(pdtLengths)/sizeof(PDTLen_t)); n++) {
            if (pdtLengths[n].pdt == pdt) {
                printf("PDT: %i, len: %i, input len: %i\n",pdt,pdtLengths[n].len,inl.count()); fflush(stdout);
                if (pdtLengths[n].len == inl.count()) {
                    ok = true;
                }
                break;
            }
        }
        if (!ok) {
            return false;
        }
        out.clear();
        for (QStringList::iterator iter = inl.begin(); iter != inl.end(); iter++) {
            if (iter->toLower().startsWith("0x")) {
                out.append((uint8_t)(iter->mid(2).toUInt(&ok,0x10)));
            } else {
                out.append((uint8_t)(iter->toUInt(&ok,10)));
            }
            if (!ok) {
                return false;
            }
        }
        return true;
    }

    dpt_num = dpt.toInt(&ok);
    if (!ok) {
        return false;
    }
    out.clear();

    if (in.isNull()) {
        return false;
    }

    switch(dpt_num) {
    case 1:
        out.clear();
        if (!(in.toLower().compare("true")) || (!(in.toLower().compare("1")))) {
            out.append((uint8_t)1);
        } else {
            out.append((uint8_t)0);
        }
        break;
    case 2:
        if (!in.toLower().compare("00")) {
            out.append((uint8_t)0x00);
        } else if (!in.toLower().compare("01")) {
            out.append((uint8_t)0x01);
        } else if (!in.toLower().compare("10")) {
            out.append((uint8_t)0x02);
        } else if (!in.toLower().compare("11")) {
            out.append((uint8_t)0x03);
        } else {
            return false;
        }
        break;
    case 3: /* Dimmer control, makes little sense for config values */
        return false;
    case 4: /* Character Set */
        if (in.isEmpty()) {
            return false;
        }
        out.append((in.toLocal8Bit().data())[0]);
        break;
    case 5: /* 8-bit unsigned */
        val16 = in.toUInt(&ok);
        if (!ok) {
            return false;
        }
        if (val16 > 255) {
            return false;
        }
        val8 = (uint8_t)val16;

        out.clear();
        out.append(val8);
        break;
    case 6: /* Signed 8-bit */
        sval16 = in.toInt(&ok);
        if (!ok) {
            return false;
        }
        if ((sval16 > 127) || (sval16 < -128)) {
            return false;
        }
        out.clear();
        out.append((int8_t)sval16);
        break;
    case 7:
        val16 = in.toUInt(&ok);
        if (!ok) {
            return false;
        }
        out.clear();
        out.append((uint8_t)(val16 & 0xFF));
        out.append((uint8_t)(val16 >> 8));
        break;
    case 8:
        sval16 = in.toInt(&ok);
        if (!ok) {
            return false;
        }
        out.clear();
        out.append((uint8_t)(sval16 & 0xFF));
        out.append((uint8_t)(sval16 >> 8));
        break;
    case 9:
        out.clear();
        val_d = in.toDouble(&ok);
        uint16_t outval, exp;
        if (!ok) {
            val_d = in.replace(",",".").toDouble(&ok);
        }
        if (!ok) {
            return false;
        }

        if (val_d >= 0) {
            val_d *= 100;
            exp = 0;
            while (val_d >= 2048) {
                exp++;
                val_d /= 2;
            }
            outval = ((uint16_t)val_d) & 0x07FF;
            outval |= (exp << 11);
        } else {
            val_d = -100*val_d;
            exp = 0;
            while (val_d >= 2048) {
                exp++;
                val_d /= 2;
            }
            outval = ((uint16_t)(-val_d)) & 0x07FF;
            outval |= (exp << 11) | 0x8000;
        }

        out.append((uint8_t)(outval & 0xFF));
        out.append((uint8_t)(outval >> 8));
        break;
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
        return false;
    case 42:
        uint16_t val;
        bool ok;

        val = in.toUInt(&ok);
        if (!ok) {
            return false;
        }

        out.clear();
        out.append((uint8_t)(val & 0xFF));
        out.append((uint8_t)((val >> 8) & 0xFF));
        break;
    default:
        return false;
    }
    return true;
}


QString Parameter::unit() {
    bool ok;
    QStringList dpts = dpt.split(".");
    if (dpts.count() < 2) {
        return QString("");
    }

    QString type_s = dpts.takeFirst();
    QString subtype_s = dpts.takeFirst();
    uint16_t type = type_s.toUInt(&ok);
    if (!ok) {
        return QString("");
    }
    uint16_t subtype = subtype_s.toUInt(&ok);
    if (!ok) {
        return QString("");
    }
    switch(1000*type+subtype) {
    case 5001:
    case 5004:
    case 8010:
    case 9007:
        return QString("%");
    case 5003:
        return QString("°");
    case 5010:
        return QString("Pulses");
    case 6001:
        return QString("%");
    case 6010:
        return QString("");
    case 9001:
        return QString("°C");
    case 9002:
        return QString("K");
    case 9003:
        return QString("K/h");
    case 9004:
        return QString("Lux");
    case 9005:
        return QString("m/s");
    case 9006:
        return QString("Pa");
    case 9008:
        return QString("ppm");
    case 9010:
        return QString("s");
    case 9011:
        return QString("ms");
    case 9020:
        return QString("mV");
    case 9021:
        return QString("mA");
    case 9022:
        return QString("W/m²");
    case 9023:
        return QString("K/%");
    case 9024:
        return QString("kW");
    case 9025:
        return QString("l/h");
    case 14007: return QString("°");
    case 14017: return QString("kg/m³");
    case 14019: return QString("A");
    case 14027: return QString("V");
    case 14028: return QString("V");
    case 14031: return QString("J");
    case 14032: return QString("N");
    case 14033: return QString("Hz");
    case 14036: return QString("W");
    case 14037: return QString("J");
    case 14038: return QString("Ω");
    case 14039: return QString("m");
    case 14051: return QString("kg");
    case 14056: return QString("W");
    case 14065: return QString("m");
    case 14066: return QString("Pa");
    case 14067: return QString("N/m");
    case 14068: return QString("°C");
    case 14069: return QString("K");
    case 14070: return QString("K");
    case 14078: return QString("N");
    case 14079: return QString("J");
    }
    return QString("");
}

PropertyKey::PropertyKey(uint8_t obj, uint8_t id) {
    this->obj = obj;
    this->id = id;
}

uint8_t PropertyKey::getObj() {
    return obj;
}

uint8_t PropertyKey::getID() {
    return id;
}

bool PropertyKey::operator <(PropertyKey k2) {
    if (obj < k2.obj) {
        return true;
    }
    if (obj > k2.obj) {
        return false;
    }
    if (id < k2.id) {
        return true;
    }
    return false;
}

bool PropertyKey::operator==(PropertyKey k2) {
    return ((obj == k2.obj) && (id == k2.id));
}

bool PropertyKeyCompare::operator()(PropertyKey* k1, PropertyKey* k2) {
    if (k1->getObj() < k2->getObj()) {
        return true;
    }
    if (k1->getObj() > k2->getObj()) {
        return false;
    }
    if (k1->getID() < k2->getID()) {
        return true;
    }
    return false;
}


EibPropertyDataType pdtFromString(QString pdt) {
    if (!pdt.toUpper().compare("CONTROL")) {
        return PT_CONTROL;
    }
    if (!pdt.toUpper().compare("CHAR")) {
        return PT_CHAR;
    }
    if (!pdt.toUpper().compare("UNSIGNED_CHAR")) {
        return PT_UNSIGNED_CHAR;
    }
    if (!pdt.toUpper().compare("INT")) {
        return PT_INT;
    }
    if (!pdt.toUpper().compare("UNSIGNED_INT")) {
        return PT_UNSIGNED_INT;
    }
    if (!pdt.toUpper().compare("EIB_FLOAT")) {
        return PT_EIB_FLOAT;
    }
    if (!pdt.toUpper().compare("DATE")) {
        return PT_DATE;
    }
    if (!pdt.toUpper().compare("TIME")) {
        return PT_TIME;
    }
    if (!pdt.toUpper().compare("LONG")) {
        return PT_LONG;
    }
    if (!pdt.toUpper().compare("UNSIGNED_LONG")) {
        return PT_UNSIGNED_LONG;
    }
    if (!pdt.toUpper().compare("FLOAT")) {
        return PT_FLOAT;
    }
    if (!pdt.toUpper().compare("DOUBLE")) {
        return PT_DOUBLE;
    }
    if (!pdt.toUpper().compare("CHAR_BLOCK")) {
        return PT_CHAR_BLOCK;
    }
    if (!pdt.toUpper().compare("SHORT_CHAR_BLOCK")) {
        return PT_SHORT_CHAR_BLOCK;
    }
    if (!pdt.toUpper().compare("GENERIC01")) {
        return PT_GENERIC_01;
    }
    if (!pdt.toUpper().compare("GENERIC02")) {
        return PT_GENERIC_02;
    }
    if (!pdt.toUpper().compare("GENERIC03")) {
        return PT_GENERIC_03;
    }
    if (!pdt.toUpper().compare("GENERIC04")) {
        return PT_GENERIC_04;
    }
    if (!pdt.toUpper().compare("GENERIC05")) {
        return PT_GENERIC_05;
    }
    if (!pdt.toUpper().compare("GENERIC06")) {
        return PT_GENERIC_06;
    }
    if (!pdt.toUpper().compare("GENERIC07")) {
        return PT_GENERIC_07;
    }
    if (!pdt.toUpper().compare("GENERIC08")) {
        return PT_GENERIC_08;
    }
    if (!pdt.toUpper().compare("GENERIC09")) {
        return PT_GENERIC_09;
    }
    if (!pdt.toUpper().compare("GENERIC10")) {
        return PT_GENERIC_10;
    }
    if (!pdt.toUpper().compare("GENERIC11")) {
        return PT_GENERIC_11;
    }
    if (!pdt.toUpper().compare("GENERIC12")) {
        return PT_GENERIC_12;
    }
    if (!pdt.toUpper().compare("GENERIC13")) {
        return PT_GENERIC_13;
    }
    if (!pdt.toUpper().compare("GENERIC14")) {
        return PT_GENERIC_14;
    }
    if (!pdt.toUpper().compare("GENERIC15")) {
        return PT_GENERIC_15;
    }
    if (!pdt.toUpper().compare("GENERIC16")) {
        return PT_GENERIC_16;
    }
    if (!pdt.toUpper().compare("GENERIC17")) {
        return PT_GENERIC_17;
    }
    if (!pdt.toUpper().compare("GENERIC18")) {
        return PT_GENERIC_18;
    }
    if (!pdt.toUpper().compare("GENERIC19")) {
        return PT_GENERIC_19;
    }
    if (!pdt.toUpper().compare("GENERIC20")) {
        return PT_GENERIC_20;
    }
    return PT_UNDEFINED;
}

QString pdtToString(EibPropertyDataType pdt) {
    switch(pdt) {
    case PT_CONTROL:
        return QString("CONTROL");

    case PT_CHAR:
        return QString("CHAR");

    case PT_UNSIGNED_CHAR:
        return QString("UNSIGNED_CHAR");

    case PT_INT:
        return QString("INT");

    case PT_UNSIGNED_INT:
        return QString("UNSIGNED_INT");

    case PT_EIB_FLOAT:
        return QString("EIB_FLOAT");

    case PT_DATE:
        return QString("DATE");

    case PT_TIME:
        return QString("TIME");

    case PT_LONG:
        return QString("LONG");

    case PT_UNSIGNED_LONG:
        return QString("UNSIGNED_LONG");

    case PT_FLOAT:
        return QString("FLOAT");

    case PT_DOUBLE:
        return QString("DOUBLE");

    case PT_CHAR_BLOCK:
        return QString("CHAR_BLOCK");

    case PT_SHORT_CHAR_BLOCK:
        return QString("SHORT_CHAR_BLOCK");

    case PT_GENERIC_01:
        return QString("GENERIC01");

    case PT_GENERIC_02:
        return QString("GENERIC02");

    case PT_GENERIC_03:
        return QString("GENERIC03");

    case PT_GENERIC_04:
        return QString("GENERIC04");

    case PT_GENERIC_05:
        return QString("GENERIC05");

    case PT_GENERIC_06:
        return QString("GENERIC06");

    case PT_GENERIC_07:
        return QString("GENERIC07");

    case PT_GENERIC_08:
        return QString("GENERIC08");

    case PT_GENERIC_09:
        return QString("GENERIC09");

    case PT_GENERIC_10:
        return QString("GENERIC10");

    case PT_GENERIC_11:
        return QString("GENERIC11");

    case PT_GENERIC_12:
        return QString("GENERIC12");

    case PT_GENERIC_13:
        return QString("GENERIC13");

    case PT_GENERIC_14:
        return QString("GENERIC14");

    case PT_GENERIC_15:
        return QString("GENERIC15");

    case PT_GENERIC_16:
        return QString("GENERIC16");

    case PT_GENERIC_17:
        return QString("GENERIC17");

    case PT_GENERIC_18:
        return QString("GENERIC18");

    case PT_GENERIC_19:
        return QString("GENERIC19");

    case PT_GENERIC_20:
        return QString("GENERIC20");

    default:
        return QString();
    }
}

bool Parameter::checkPDT_DPT() {
    QString dpt;
    if (this->dpt.isNull() || this->dpt.isEmpty()) {
        return true;
    }
    uint16_t dpt_num;
    printf("== DPT: %s, PDT: %i (%s)\n",this->dpt.toLocal8Bit().data(),pdt,pdtToString(pdt).toLocal8Bit().data());
    QStringList dpts = this->dpt.split(".");
    if (dpts.size() > 1) {
        dpt = dpts[0];
    } else {
        dpt = this->dpt;
    }
    bool ok;
    dpt_num = dpt.toInt(&ok);
    if (!ok) {
        return false;
    }
    printf("=== DPT: %s, numeric: %i, PDT: %i (%s)\n",dpt.toLocal8Bit().data(),dpt_num,pdt,pdtToString(pdt).toLocal8Bit().data());

    switch(dpt_num) {
    case 1:
        if ((pdt == PT_UNSIGNED_CHAR) || (pdt == PT_GENERIC_01)) {
            return true;
        }
        break;
    case 2:
        if (pdt == PT_GENERIC_01) {
            return true;
        }
        break;
    case 3:
        if (pdt == PT_GENERIC_01) {
            return true;
        }
        break;
    case 4:
        if ((pdt == PT_UNSIGNED_CHAR) || (pdt == PT_GENERIC_01)) {
            return true;
        }
        break;
    case 5:
        if ((pdt == PT_UNSIGNED_CHAR) || (pdt == PT_SCALING)) {
            return true;
        }
        break;
    case 6:
        if (pdt == PT_CHAR) {
            return true;
        }
        break;
    case 7:
        if (pdt == PT_UNSIGNED_INT) {
            return true;
        }
        break;
    case 8:
        if (pdt == PT_INT) {
            return true;
        }
        break;
    case 9:
        if (pdt == PT_EIB_FLOAT) {
            return true;
        }
        break;
    case 10:
        if (pdt == PT_TIME) {
            return true;
        }
        break;
    case 11:
        if (pdt == PT_DATE) {
            return true;
        }
        break;
    case 12:
        if (pdt == PT_UNSIGNED_LONG) {
            return true;
        }
        break;
    case 13:
        if (pdt == PT_LONG) {
            return true;
        }
        break;
    case 14:
        if (pdt == PT_FLOAT) {
            return true;
        }
        break;
    case 15:
        if (pdt == PT_GENERIC_04) {
            return true;
        }
        break;
    case 16:
        if (pdt == PT_GENERIC_14) {
            return true;
        }
        break;
    case 17:
    case 18:
        if (pdt == PT_GENERIC_01) {
            return true;
        }
        break;
    case 19:
        if (pdt == PT_DATE_TIME) {
            return true;
        }
        break;
    case 20:
        if ((pdt == PT_ENUM8) || (pdt == PT_UNSIGNED_CHAR)) {
            return true;
        }
        break;
    case 21:
        if ((pdt == PT_BITSET8) || (pdt == PT_GENERIC_01)) {
            return true;
        }
        break;
    case 22:
    case 23:
        if ((pdt == PT_ENUM8) || (pdt == PT_UNSIGNED_CHAR)) {
            return true;
        }
        break;
    default:
        printf("Warning: DPT %s not checked, implement in checkPDT_DPT()\n",dpt.toLocal8Bit().data());
        return true;
    }
    return false;
}

