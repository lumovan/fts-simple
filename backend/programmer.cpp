#include "programdialog.h"
#include "programmer.h"
#include "falcon_if.h"
#include "util.h"

#ifdef PROGRAM_THREAD
class doProgrammingClass : public QThread {
public:
    doProgrammingClass() {}
    KDevice *device;
    uint8_t what;
    Programmer *programmer;
    void run() {
        programmer->doProgramming(device,what);
    }
};

class doReflashClass : public QThread {
public:
    doReflashClass() {}
    KDevice *device;
    Programmer *programmer;
    void run() {
        programmer->doReflash(device);
    }
};

class doRestartClass : public QThread {
public:
    doRestartClass() {}
    KDevice *device;
    Programmer *programmer;
    void run() {
        programmer->doRestart(device);
    }
};
#endif

Programmer::Programmer() {
    falcon = new FalconIF();

    active = false;
}

Programmer::~Programmer() {
    delete falcon;
}

bool Programmer::isWorking() {
    return falcon->isWorking();
}

QString Programmer::connectionDesc() {
    return falcon->connectionDesc();
}

bool Programmer::setConnection() {
    return falcon->setConnection();
}

void Programmer::cancel() {

}

bool Programmer::startProgramming(KDevice *device,uint8_t what) {
#ifdef PROGRAM_THREAD
    if (active) {
        return false;
    }
    doProgrammingClass *dp = new doProgrammingClass();
    dp->device = device;
    dp->what = what;
    dp->start();
    return true;
#else
    return doProgramming(device,what);
#endif
}


bool Programmer::doProgramming(KDevice *device,uint8_t what) {
    if ((device == 0) || active) {
        emit(programmingDone(false));
        return false;
    }
    active = true;

    if (what & PROGRAM_PA) {
        emit (statusChanged(tr("Setting physical address")));
        if (!falcon->setPA(device)) {
            emit(statusChanged(tr("Cannot set physical address, terminating")));
            active = false;
            return false;
        }
    }

    emit(statusChanged(tr("Connecting device")));
    if (!falcon->connectDevice(device)) {
        emit(statusChanged(tr("Cannot connect to device")));
        emit(programmingDone(false));
        active = false;
        return false;
    }
    emit (statusChanged(tr("Connected to device")));

    QList<uint16_t> addrtable = device->addrtable();
    QList<uint16_t> assoctable = device->assoctable();
    QList<uint8_t> objtable = device->objtable();

    if (what & PROGRAM_GA) {
        uint8_t *addrdata = (uint8_t*)malloc(addrtable.length()*2+1);
        uint8_t *assocdata = (uint8_t*)malloc(assoctable.length()*2+1);
        uint8_t *objdata;
        if (objtable.length() > 0) {
             objdata = (uint8_t*)malloc(objtable.length());
        }
        if ((addrdata == 0) || (assocdata == 0) || ((objtable.length() > 0) && (objdata == 0))) {
            emit(statusChanged(tr("Memory allocation error")));
            falcon->disconnectDevice();
            emit(programmingDone(false));
            active = false;
            return false;
        }

        addrdata[0] = (uint8_t)addrtable.length();
        assocdata[0] = (uint8_t)assoctable.length();
        for (uint8_t n=0; n < addrtable.length(); n++) {
            addrdata[2*n+1] = (uint8_t)((addrtable.at(n) >> 8) & 0xFF);
            addrdata[2*n+2] = (uint8_t)(addrtable.at(n) & 0xFF);
        }
        for (uint8_t n=0; n < assoctable.length(); n++) {
            assocdata[2*n+1] = (uint8_t)(assoctable.at(n) & 0xFF);
            assocdata[2*n+2] = (uint8_t)((assoctable.at(n) >> 8) & 0xFF);
        }


        emit(statusChanged(tr("Writing address data")));
        if (!falcon->writeMemory(device->getAddrtableAddress(),addrtable.length()*2+1,addrdata)) {
            emit(statusChanged(tr("Cannot write address data")));
            falcon->disconnectDevice();
            emit(programmingDone(false));
            active = false;
            return false;
        }
        emit(statusChanged(tr("Writing association data")));
        if (!falcon->writeMemory(device->getAssoctableAddress(),assoctable.length()*2+1,assocdata)) {
            emit(statusChanged(tr("Error writing association data")));
            falcon->disconnectDevice();
            emit(programmingDone(false));
            active = false;
            return false;
        }
        free(addrdata);
        free(assocdata);

        if (objtable.length() > 0) {
            for (uint8_t n=0; n < objtable.length(); n++) {
                objdata[n] = objtable.at(n);
            }
            emit(statusChanged(tr("Writing object data (flags)")));
            if (!falcon->writeMemory(device->getObjtableAddress(),objtable.length(),objdata)) {
                emit(statusChanged(tr("Error writing object data (flags)")));
                falcon->disconnectDevice();
                emit(programmingDone(false));
                active = false;
                return false;
            }
            free(objdata);
        } else {
            QList<uint8_t> co_list = device->coListUint();
            for (QList<uint8_t>::iterator co_iter = co_list.begin(); co_iter != co_list.end(); co_iter++) {
                uint8_t co = *co_iter;
                emit(statusChanged(tr("Writing flags for CO ").append(QString::number(co,10))));
                uint8_t flags = device->getFlags(co);
                if (!falcon->writeProperty(co,0,PT_GENERIC_01,0,1,1,&flags,true)) {
                    emit(statusChanged(tr("Error writing flags for CO ").append(QString::number(co,10))));
                    falcon->disconnectDevice();
                    emit(programmingDone(false));
                    active = false;
                    return false;
                }
            }
        }
printf("#N"); fflush(stdout);
    }

    if (what & PROGRAM_PARAMS) {
        QMap<QString,Parameter*> paramlist = device->getParameters();
        for (QMap<QString,Parameter*>::iterator p = paramlist.begin(); p != paramlist.end(); p++) {
            if ((p.value()->getValue().size() == 0) || (p.value()->isReadonly())) {
                continue;
            }
            emit(statusChanged(tr("Writing property ").append(p.value()->getID())));
            uint8_t *data = (uint8_t*)malloc(p.value()->getValue().size());
            if (data == NULL) {
                emit(statusChanged(tr("Error allocating memory")));
                falcon->disconnectDevice();
                emit(programmingDone(false));
                active = false;
                return false;
            }
            printf("Copying property data for %s, DPT %s, PDT %s\n",p.value()->getID().toLocal8Bit().data(),p.value()->getDPT().toLocal8Bit().data(),pdtToString(p.value()->getPDT()).toLatin1().data());
            for (int n=0; n < p.value()->getValue().size(); n++) {
                data[n] = p.value()->getValue().at(n);
                printf("%02X ",data[n]);
            }
            printf("\n");
            Logger::log("Programming param %s, PDT %X\n",p.value()->getID().toLatin1().data(),p.value()->getPDT());
            if (p.value()->getParamPos() == param_pos_property) {
                if (!falcon->writeProperty(p.value()->getObjectID(),p.value()->getPropertyID(),p.value()->getPDT(),1,1,p.value()->getValue().size(),data,true)) {
                    emit(statusChanged(tr("Cannot write property ").append(p.value()->getID())));
                    falcon->disconnectDevice();
                    emit(programmingDone(false));
                    free(data);
                    active = false;
                    return false;
                }
            }
#ifdef __GNUC__
#warning Memory-based data not written
#else
#pragma warning ("Memory-based data not written")
#endif
            free(data);
        }
    }

    emit(statusChanged(tr("Restarting device")));
    if (falcon->restart()) {
        emit(statusChanged(tr("Device restarted")));
    } else {
        emit(statusChanged(tr("Could not restart device")));
        emit(programmingDone(false));
        active = false;
        return false;
    }

    emit(statusChanged(tr("Disconnecting from device")));
    if (!falcon->disconnectDevice()) {
        emit(statusChanged(tr("Cannot disconnect from device")));
        emit(programmingDone(false));
        active = false;
        return false;
    }

    emit(statusChanged(tr("Disconnected from device")));
    emit(programmingDone(true));
    active = false;
    return true;
}

bool Programmer::reflash(KDevice *device) {
#ifdef PROGRAM_THREAD
    if (active) {
        return false;
    }
    doReflashClass *rf = new doReflashClass();
    rf->device = device;
    rf->start();
    return true;
#else
    return doReflash(device);
#endif
}

bool Programmer::doReflash(KDevice *device) {
    if (active) {
        return false;
    }
    active = true;

    emit(statusChanged(tr("Connecting device")));
    if (!falcon->connectDevice(device)) {
        emit(statusChanged(tr("Cannot connect to device")));
        active = false;
        return false;
    }
    emit(statusChanged(tr("Connected to device")));

    QVector<uint8_t> hexdata = device->getHexData();
    uint8_t *hex8 = (uint8_t*)malloc(hexdata.length());
    for (uint16_t n=0; n < hexdata.length(); n++) {
        hex8[n] = hexdata.at(n);
        printf("%02X ",hex8[n]);
    }

    uint16_t total_len = hexdata.length();
    uint16_t restlen = total_len;
    uint16_t blocklen = (uint16_t)(total_len/100);
    uint16_t index = 0;
    if (blocklen < 12) {
        blocklen = 12;
    }
    while (restlen > 0) {
        if (blocklen > restlen) {
            blocklen = restlen;
        }
        if (!falcon->writeMemory(0x5000+index,blocklen,hex8+index,false)) {
            emit(statusChanged(tr("Error writing hex data")));
            falcon->disconnectDevice();
            active = false;
            return false;
        }
        emit(statusChanged(QString::number((uint16_t)((100ul*(total_len-restlen))/total_len)).append("% written")));
        index += blocklen;
        restlen -= blocklen;
    }
    emit(statusChanged(tr("Data written")));

    falcon->disconnectDevice();
    active = false;
    return true;
}

bool Programmer::restart(KDevice *device) {
    #ifdef PROGRAM_THREAD
    if (active) {
        printf("Programmer::restart: Falcon is active\n"); fflush(stdout);
        return false;
    }
    doRestartClass *rs = new doRestartClass();
    rs->device = device;
    rs->start();
    return true;
#else
    return doRestart(device);
#endif
}

bool Programmer::doRestart(KDevice *device) {
    printf("Programmer::doRestart()\n"); fflush(stdout);
    if (device == NULL) {
        return false;
    }
    if (active) {
        return false;
    }

    emit(statusChanged(tr("Connecting device")));
    if (!falcon->connectDevice(device)) {
        emit(statusChanged(tr("Cannot connect to device")));
        active = false;
        return false;
    }
    emit(statusChanged(tr("Connected to device")));

    if (!falcon->restart()) {
        emit(statusChanged(tr("Cannot restart device")));
        falcon->disconnectDevice();
        active = false;
        return false;
    }

    emit(statusChanged(tr("Device restarted")));
    falcon->disconnectDevice();
    active = false;
    return true;
}

