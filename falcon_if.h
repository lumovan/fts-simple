#ifndef FALCON_IF_H
#define FALCON_IF_H

#include <QMap>
#include <QString>
#include "kdevice.h"
#if (defined (__WIN32__) || defined (__WIN64__))
#include <windows.h>
#endif
#ifdef USE_FALCON_IF
#include "falcon.h"
#include "FalconConnectionManager.h"
#else
#include <propertydatatype.h>
#endif

typedef enum {
    idle,
    connected,
    error
} if_state_t;

class FalconIF : QObject {
    Q_OBJECT
public:
    FalconIF();
    ~FalconIF();
    bool isWorking() { return working; }

    bool setConnection();
    QString connectionDesc();
    bool connectDevice(KDevice *device);
    bool disconnectDevice();
    bool readMemory(uint16_t addr, uint16_t count, uint8_t *data);
    bool writeMemory(uint16_t addr, uint16_t count, uint8_t *data, bool verify = true);
    bool readProperty(uint8_t object, uint8_t property, EibPropertyDataType datatype, uint16_t start, uint16_t nelem, uint8_t *data);
    bool writeProperty(uint8_t object, uint8_t property, EibPropertyDataType datatype, uint16_t start, uint16_t nelem, uint8_t elemsize, uint8_t *data, bool verify);
    bool restart();
    bool setPA(KDevice *device);

    static QString errTxt(HRESULT hr);

signals:
    void status(QString);

private:
    bool working;
#ifdef USE_FALCON_IF
    FalconConnection *pUserConnection;
    IConnectionManager* pConnectionManager;
    QMap<uint16_t, IConnectionCustom*> pConnections;
    IConnectionCustom *activeConnection;
    IDevice* pDevice;
    IGroupDataTransfer* pGroupData;
    IMemoryAccess2 *pMemoryAccess;
    IPropertyAccess2 *pPropertyAccess;
    if_state_t if_state;

    static long int getVariantArraySize(const VARIANT& v);
#elif defined (USE_DUMMY_IF)
    KDevice *connectedDevice;
#endif
};



#endif // FALCON_IF_H
