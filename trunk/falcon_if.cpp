#include <stddef.h>
#include <stdlib.h>
#include <qglobal.h>
#include "falcon_if.h"
#include <stdio.h>
#include <string>
#include "util.h"
#include "kdevice.h"

#ifdef USE_FALCON_IF
#include <rpc.h>
#include <comutil.h>
#include <FalconHResults.h>

FalconIF::FalconIF() {
    pConnectionManager = NULL;
    pUserConnection = NULL;
    pConnections.clear();
    activeConnection = NULL;
    pDevice = NULL;
    pGroupData = NULL;
    pMemoryAccess = NULL;
    pPropertyAccess = NULL;
    if_state = idle;

    HRESULT hr; // for failure output purposes

    hr = CoInitializeEx( NULL, COINIT_APARTMENTTHREADED );

    if(!SUCCEEDED(hr)) {
        working = false;
        return;
    }

    // declare interface pointer

    hr = CoCreateInstance( CLSID_ConnectionManager,               // CLSID of ConnectionManager
                           NULL,                                  // no aggregation
                           CLSCTX_ALL,                            // any Falcon server is welcome
                           IID_IConnectionManager,                       // IConnection reference requested for
                           reinterpret_cast<void**>(&pConnectionManager) // pointer to interface is returned here
                           );
    if (!SUCCEEDED(hr)) {
        working = false;
        return;
    }
    working = true;

    pUserConnection = (FalconConnection*)malloc(sizeof(FalconConnection));
    if (pUserConnection == NULL) {
        working = false;
        return;
    }
    hr = pConnectionManager->GetDefaultConnection(pUserConnection);

    if (!SUCCEEDED(hr)) {
        pUserConnection = NULL;
    }

    working = true;
}

FalconIF::~FalconIF() {
    for (QMap<uint16_t,IConnectionCustom*>::iterator iter = pConnections.begin(); iter != pConnections.end(); iter++) {
        Logger::log("Releasing connection to %s\n",uint2pa(iter.key()).toLocal8Bit().data());
        iter.value()->Release();
    }
    if (pMemoryAccess != NULL) {
        pMemoryAccess->Release();
    }
    if (pPropertyAccess != NULL) {
        pPropertyAccess->Release();
    }
    if (pDevice != NULL) {
        pDevice->Release();
    }
}

bool FalconIF::connectDevice(KDevice *device) {
    HRESULT hr;
    DeviceOpenError err;
    long maskver;

    if ((device == NULL) || (pUserConnection == NULL)) {
        return false;
    }
    activeConnection = NULL;

    Logger::log("Connecting for %s\n",device->getPA().toLatin1().data());
    uint16_t pa = pa2uint(device->getPA());
    if (pConnections.contains(pa)) {
        activeConnection = pConnections[pa];
        ConnectionState_t state;
        if (!SUCCEEDED(activeConnection->get_State(&state))) {
            return false;
        }
    }
    if (activeConnection == NULL) {
        Logger::log("Creating connection for device %s\n",device->getPA().toUtf8().data());

        IClassFactory2 *ptrClf2;
        hr = CoGetClassObject( __uuidof( ConnectionObject ), CLSCTX_LOCAL_SERVER, NULL, IID_IClassFactory2,
                               reinterpret_cast<void**>( &ptrClf2 ) );

        if (!SUCCEEDED(hr)) {
            return false;
        }
        // create the object using a license key
        hr = ptrClf2->CreateInstanceLic( NULL, NULL, IID_IConnectionCustom,
                                         L"Demo", reinterpret_cast<void**>( &activeConnection ) );
        if (!SUCCEEDED(hr)) {
            printf("Cannot create connection: %s\n",errTxt(hr).toLatin1().data()); fflush(stdout);
            return false;
        }

        unsigned long access_key = device->getAccessKey();
        _bstr_t _m_bstrConnectionParameter = pUserConnection->wszParameters;

        if (access_key != 0) {
            hr = activeConnection->put_AccessKey(access_key);
            if (!SUCCEEDED(hr)) {
                Logger::log("Cannot put access key: %s\n",errTxt(hr).toLatin1().data()); fflush(stdout);
                activeConnection->Release();
                return false;
            }
        }

        hr = activeConnection->put_TargetAddress(_variant_t(device->getPA().toLocal8Bit().data()));
        if (!SUCCEEDED(hr)) {
            Logger::log("Cannot put target address: %s\n",errTxt(hr).toLatin1().data()); fflush(stdout);
            activeConnection->Release();
            return false;
        }

        hr = activeConnection->put_Mode(ConnectionModeRemoteConnection);
        if (!SUCCEEDED(hr)) {
            Logger::log("Cannot put mode: %s\n",errTxt(hr).toLatin1().data()); fflush(stdout);
            activeConnection->Release();
            return false;
        }

        hr = activeConnection->Open2(pUserConnection->guidEdi,_variant_t(SysAllocString(_m_bstrConnectionParameter)),&err);
        if (!SUCCEEDED(hr)) { // && (hr != FALCON_E_DEVICEALREADYINUSE)) {
            Logger::log("Open2 Failed: %s\n",errTxt(hr).toLatin1().data()); fflush(stdout);
            activeConnection->Release();
            return false;
        }
        hr = activeConnection->get_FirmwareDescriptor(&maskver);
        pConnections.insert(pa,activeConnection);
        ConnectionState_t state;
        activeConnection->get_State(&state);
        Logger::log("Connected: %i %X\n",state); fflush(stdout);
        long fw;
        hr = activeConnection->get_FirmwareDescriptor(&fw);
        Logger::log("FW: %i %X\n",hr,fw); fflush(stdout);

    }
    return true;
}

bool FalconIF::disconnectDevice() {
    ConnectionState_t state;
    activeConnection->get_State(&state);
    Logger::log("Disconnecting device, state = %X\n",state);
    if (pMemoryAccess != NULL) {
        pMemoryAccess->Release();
        pMemoryAccess = NULL;
    }
    if (pPropertyAccess != NULL) {
        pPropertyAccess->Release();
        pPropertyAccess = NULL;
    }
    activeConnection->get_State(&state);
    Logger::log("Device disconnected, state now %X\n",state);
    activeConnection = NULL;
    return true;
}

bool FalconIF::readMemory(uint16_t addr, uint16_t count, uint8_t *data) {
    HRESULT hr;
    _variant_t result;

    if (activeConnection == NULL) {
        return false;
    }
    ConnectionState_t state;
    hr = activeConnection->get_State(&state);
    if (state != ConnectionOpenedOk) {
        return false;
    }

    if (pMemoryAccess == NULL) {
        hr = CoCreateInstance(__uuidof(MemoryAccess), NULL, CLSCTX_ALL, IID_IMemoryAccess2, reinterpret_cast<void**>(&pMemoryAccess));
        if (!SUCCEEDED(hr)) {
            pMemoryAccess = NULL;
            return false;
        }

        hr = pMemoryAccess->putref_Connection((IConnection*)activeConnection);
        if (!SUCCEEDED(hr)) {
            pMemoryAccess->Release();
            pMemoryAccess = NULL;
            return false;
        }
    }

    hr = pMemoryAccess->Read(count,addr,&result);
    if (!SUCCEEDED(hr)) {
        pMemoryAccess->Release();
        pMemoryAccess = NULL;
        return false;
    }
    void *p = NULL;
    SafeArrayAccessData(result.parray,&p);
    long len = getVariantArraySize(result);
    if (len != count) {
        pMemoryAccess->Release();
        pMemoryAccess = NULL;
        return false;
    }
    memcpy(data,p,len);

    return true;
}

bool FalconIF::writeMemory(uint16_t addr, uint16_t count, uint8_t *data, bool verify) {
    HRESULT hr;

    if (activeConnection == NULL) {
        Logger::log("writeMemory: active connection is NULL\n"); fflush(stdout);
        return false;
    }
    ConnectionState_t state;
    hr = activeConnection->get_State(&state);
    if (state == ConnectionClosed) {
        Logger::log("writeMemory: connection state is CLOSED\n"); fflush(stdout);
        return false;
    }

    VARIANT vdata;
    VariantInit(&vdata);
    vdata.vt = VT_UI1 | VT_ARRAY;

    SAFEARRAY *varray = SafeArrayCreateVector(VT_UI1,0,count);
    if (varray == 0) {
        Logger::log("writeMemory: cannot create SafeArray\n"); fflush(stdout);
        return false;
    }
    memcpy(varray->pvData,data,count);
    vdata.parray = varray;

    if (pMemoryAccess == NULL) {
        hr = CoCreateInstance(__uuidof(MemoryAccess), NULL, CLSCTX_ALL, IID_IMemoryAccess2, reinterpret_cast<void**>(&pMemoryAccess));
        if (!SUCCEEDED(hr)) {
            Logger::log("writeMemory: cannot create MemoryAccess instance: %s\n",errTxt(hr).toLatin1().data()); fflush(stdout);
            pMemoryAccess = NULL;
            return false;
        }
        hr = pMemoryAccess->putref_Connection((IConnection*)activeConnection);
        if (!SUCCEEDED(hr)) {
            Logger::log("writeMemory: cannot put connection: %s\n",errTxt(hr).toLatin1().data());
            pMemoryAccess->Release();
            pMemoryAccess = NULL;
            return false;
        }
    }

    hr = pMemoryAccess->Write(addr,vdata,verify);
    if (!SUCCEEDED(hr)) {
        Logger::log("writeMemory: cannot write memory: %s\n",errTxt(hr).toLatin1().data());
        pMemoryAccess->Release();
        pMemoryAccess = NULL;
        return false;
    }
    SafeArrayDestroy(varray);

    activeConnection->get_State(&state);
    Logger::log("writeMemory: connection state after writing is %i\n",state);
    return true;
}

bool FalconIF::readProperty(uint8_t object, uint8_t property, EibPropertyDataType datatype, uint16_t start, uint16_t nelem, uint8_t *data) {
    HRESULT hr;
    _variant_t result;

    if (activeConnection == NULL) {
        return false;
    }
    ConnectionState_t state;
    hr = activeConnection->get_State(&state);
    if (state != ConnectionOpenedOk) {
        return false;
    }

    if (pPropertyAccess == NULL) {
        hr = CoCreateInstance(__uuidof(PropertyAccess), NULL, CLSCTX_ALL, IID_IPropertyAccess, reinterpret_cast<void**>(&pPropertyAccess));
        if (!SUCCEEDED(hr)) {
            pPropertyAccess = NULL;
            return false;
        }
    }
    hr = pPropertyAccess->putref_Connection((IConnection*)activeConnection);
    if (!SUCCEEDED(hr)) {
        pPropertyAccess->Release();
        pPropertyAccess = NULL;
        return false;
    }

    hr = pPropertyAccess->Read(object,property, datatype, start, nelem, &result);
    void *p = NULL;
    SafeArrayAccessData(result.parray,&p);
    long len = getVariantArraySize(result);
    memcpy(data,p,len);

    return true;
}

bool FalconIF::writeProperty(uint8_t object, uint8_t property, EibPropertyDataType datatype, uint16_t start, uint16_t nelem, uint8_t elemsize, uint8_t *data, bool verify) {
    HRESULT hr;

    Logger::log("FalconIF::writeProperty: Object %i, property %i, datatype %i, start %i, nelem %i, elemsize %i",object,property,datatype,start,nelem,elemsize);

    if (activeConnection == NULL) {
        Logger::log("FalconIF::writeProperty: connection is NULL"); fflush(stdout);
        return false;
    }
    ConnectionState_t state;
    hr = activeConnection->get_State(&state);
    if (state != ConnectionOpenedOk) {
        Logger::log("FalconIF::writeProperty: connection not ok"); fflush(stdout);
        return false;
    }

    VARIANT vdata;
    VariantInit(&vdata);
    vdata.vt = VT_UI1 | VT_ARRAY;

    SAFEARRAY *varray = SafeArrayCreateVector(VT_UI1,0,nelem*elemsize);
    if (varray == 0) {
        return false;
    }
    memcpy(varray->pvData,data,nelem*elemsize);
    vdata.parray = varray;

    if (pPropertyAccess == NULL) {
        hr = CoCreateInstance(__uuidof(PropertyAccess), NULL, CLSCTX_ALL, IID_IPropertyAccess, reinterpret_cast<void**>(&pPropertyAccess));
        if (!SUCCEEDED(hr)) {
            pPropertyAccess->Release();
            pPropertyAccess = NULL;
            hr = CoCreateInstance(__uuidof(PropertyAccess), NULL, CLSCTX_ALL, IID_IPropertyAccess, reinterpret_cast<void**>(&pPropertyAccess));
        }
        if (!SUCCEEDED(hr)) {
            Logger::log("FalconIF::writeProperty: cannot create PropertyAccess (%s)\n",errTxt(hr).toLatin1().data()); fflush(stdout);
            pPropertyAccess->Release();
            pPropertyAccess = NULL;
            return false;
        }
        hr = pPropertyAccess->putref_Connection((IConnection*)activeConnection);
        if (!SUCCEEDED(hr)) {
            Logger::log("FalconIF::writeProperty: cannot put connection (%s)\n",errTxt(hr).toLatin1().data()); fflush(stdout);
            pPropertyAccess->Release();
            pPropertyAccess = NULL;
            return false;
        }
    }

    printf("pPropertyAccess->Write(object %x,property %x,datatype %x,start %x,nelem %x,vdata,verify);\n",object,property,datatype,start,nelem);

    hr = pPropertyAccess->Write(object,property,datatype,start,nelem,vdata,verify);
    if (!SUCCEEDED(hr)) {
        Logger::log("FalconIF::writeProperty: cannot write (%s)\n",errTxt(hr).toLatin1().data()); fflush(stdout);
        pPropertyAccess->Release();
        pPropertyAccess = NULL;
        return false;
    }
    pPropertyAccess->Release();
    pPropertyAccess = NULL;

    SafeArrayDestroy(varray);

    activeConnection->get_State(&state);
    Logger::log("FalconIF::writeProperty: connection state at end is %i\n",state);
    return true;
}

bool FalconIF::restart() {
    HRESULT hr;

    if (activeConnection == NULL) {
        return false;
    }

    if (pDevice == NULL) {
        hr = CoCreateInstance(__uuidof(Device), NULL, CLSCTX_ALL, IID_IDevice, reinterpret_cast<void**>(&pDevice));
        if (!SUCCEEDED(hr)) {
            Logger::log("Restart: Cannot create Device instance (%s)\n",errTxt(hr).toLatin1().data());
            return false;
        }
    }
    if (!SUCCEEDED(pDevice->putref_Connection((IConnection*)activeConnection))) {
        Logger::log("Restart: Cannot put connection (%s)\n",errTxt(hr).toLatin1().data());
        return false;
    }
    ConnectionState_t state;
    activeConnection->get_State(&state);
    Logger::log("Reset, state: %X\n",state); fflush(stdout);
    hr = pDevice->Reset();
    activeConnection->get_State(&state);
    Logger::log("After Reset: %X\n",state); fflush(stdout);
    return(SUCCEEDED(hr));
}

bool FalconIF::setPA(KDevice* device) {

    if (device == NULL) {
        Logger::log("setPA: Device is NULL\n"); fflush(stdout);
        return false;
    }
    Logger::log("Trying to connect device\n");
    emit(status("Trying to connect device"));
    if (connectDevice(device)) {
        emit(status("Device found, PA already in use"));
        Logger::log("setPA: Can connect to device, this should not happen - PA is already in use\n"); fflush(stdout);
        disconnectDevice();
        return false;
    }

    HRESULT hr;
    IConnectionCustom *pConnection;

    IClassFactory2 *ptrClf2;
    hr = CoGetClassObject( __uuidof( ConnectionObject ), CLSCTX_LOCAL_SERVER, NULL, IID_IClassFactory2,
                           reinterpret_cast<void**>( &ptrClf2 ) );

    if (!SUCCEEDED(hr)) {
        return false;
    }
    // create the object using a license key
    hr = ptrClf2->CreateInstanceLic( NULL, NULL, IID_IConnectionCustom,
                                     L"Demo", reinterpret_cast<void**>( &pConnection ) );
    if (!SUCCEEDED(hr)) {
        Logger::log("setPA: cannot create connection: %s\n",errTxt(hr).toLatin1().data());
        return false;
    }
    pConnection->put_Mode( ConnectionModeRemoteConnectionless );
    DeviceOpenError err;
    _bstr_t _m_bstrConnectionParameter = pUserConnection->wszParameters;
    hr = pConnection->Open2(pUserConnection->guidEdi,_variant_t(SysAllocString(_m_bstrConnectionParameter)),&err);
    if (!SUCCEEDED(hr)) {
        Logger::log("setPA: cannot open connection: %s\n",errTxt(hr).toLatin1().data());
        return false;
    }

    INetworkManagement *pNetworkManagement;
    hr = CoCreateInstance(__uuidof(NetworkManagement), NULL, CLSCTX_ALL, IID_INetworkManagement, reinterpret_cast<void**>(&pNetworkManagement));
    if (!SUCCEEDED(hr)) {
        pConnection->Release();
        Logger::log("setPA: cannot create network management instance: %s\n",errTxt(hr).toLatin1().data());
        return false;

    }
    hr = pNetworkManagement->putref_Connection((IConnection*)pConnection);
    if (!SUCCEEDED(hr)) {
        pConnection->Release();
        pNetworkManagement->Release();
        Logger::log("setPA: cannot put connection: %s\n",errTxt(hr).toLatin1().data());
        return false;
    }

    emit(status("Checking for devices in programming mode"));
    VARIANT addrList;
    hr = pNetworkManagement->IndividualAddressRead(&addrList);
    if (!SUCCEEDED(hr)) {
        pConnection->Release();
        pNetworkManagement->Release();
        Logger::log("setPA: cannot read PAs: %s\n",errTxt(hr).toLatin1().data());
        return false;
    }

    if (SafeArrayGetDim(addrList.parray) == 1) {
        long lbound,ubound;
        SafeArrayGetLBound(addrList.parray, 1, &lbound);
        SafeArrayGetUBound(addrList.parray, 1, &ubound);
        if (ubound < lbound) {
            Logger::log("No device in programming mode found\n");
            pConnection->Release();
            pNetworkManagement->Release();
            return false;
        } else if (ubound - lbound > 0) {
            Logger::log("More than one device in programming mode found\n"); fflush(stdout);
            pConnection->Release();
            pNetworkManagement->Release();
            return false;
        }
        emit(status("Writing PA"));
        hr = pNetworkManagement->IndividualAddressWrite(_variant_t(device->getPA().toLocal8Bit().data()));
        if (!SUCCEEDED(hr)) {
            Logger::log("Could not write PA: %s\n",errTxt(hr).toLatin1().data());
            pNetworkManagement->Release();
            pConnection->Release();
            return false;
        }
    }

    pNetworkManagement->Release();
    pConnection->Release();
    return true;
}


bool FalconIF::setConnection() {
    if (!working) {
        return false;
    }
    FalconConnection newConnection;
    HRESULT hr = pConnectionManager->GetConnection("",VARIANT_TRUE,&newConnection);
    if (!SUCCEEDED(hr)) {
        return false;
    }
    if (pUserConnection != NULL) {
        if ((!wcscmp(newConnection.wszName,pUserConnection->wszName)) &&
                (!wcscmp(newConnection.wszParameters,pUserConnection->wszParameters)) &&
                (newConnection.guidEdi == pUserConnection->guidEdi)) {
            Logger::log("Connection has not changed\n"); fflush(stdout);
            return false;
        }
    } else {
        pUserConnection = (FalconConnection*)malloc(sizeof(FalconConnection));
        if (pUserConnection == NULL) {
            return false;
        }
    }
    memcpy(pUserConnection,&newConnection,sizeof(FalconConnection));

    for (QMap<uint16_t, IConnectionCustom*>::iterator c = pConnections.begin(); c != pConnections.end(); c++) {
        c.value()->Release();
    }
    pConnections.clear();
    return true;
}

QString FalconIF::connectionDesc() {
    if (pUserConnection == NULL) {
        return QString("(not connected)");
    }
    return QString::fromWCharArray(pUserConnection->wszName);
}

long int FalconIF::getVariantArraySize(const VARIANT& var) {

    if ( !(var.vt & VT_ARRAY) ) {
        return -1;
    }
    long size = 0;
    long dims = SafeArrayGetDim(var.parray);
    long elemSize = SafeArrayGetElemsize(var.parray);
    long elems = 1;

    for ( long i=1; i <= dims; i++ ) {
        long lbound, ubound;

        SafeArrayGetLBound(var.parray, i, &lbound);
        SafeArrayGetUBound(var.parray, i, &ubound);
        elems *= (ubound - lbound + 1);
    }

    size = elems*elemSize;

    return size;
}

QString FalconIF::errTxt(HRESULT hr) {
    switch(hr) {
    case FALCON_E_PING:
        return QString("Ping failed");
    case FALCON_E_PHYSADDRREAD:
        return QString("Physical address read failed");
    case FALCON_E_PHYSADDRWRITE:
        return QString("Physical address write failed");
    case FALCON_E_MULTIDEVPRGMODE:
        return QString("Physical/Domain address write: more than one device in programming mode");
    case FALCON_E_ADDRESSUSED:
        return QString("Physical/Domain address write: address already in use by another device");
    case FALCON_E_DOMAINADDRREAD:
        return QString("Domain address read failed");
    case FALCON_E_DOMAINADDRWRITE:
        return QString("Domain address write failed");
    case FALCON_E_DOMAINADDRSELREAD:
        return QString("Domain address selective read failed");
    case FALCON_E_DEVICE_NOT_EXIST:
        return QString("Device does not exist");
    case FALCON_E_DEV_AUTHENTICATION:
        return QString("Authentication failed");
    case FALCON_E_DEV_MASKREAD:
        return QString("Mask version read failed");
    case FALCON_E_DEV_MEM_ACC_DENIED:
        return QString("Memory access denied");
    case FALCON_E_DEV_CONF_TIMEOUT:
        return QString("Management timeout");
    case FALCON_E_NOCONNECTION:
        return QString("No connection available");
    case FALCON_E_CON_CLOSED:
        return QString("Connection explicitly closed");
    case FALCON_E_CON_MODE_INVALID:
        return QString("Connection: invalid mode");
    case FALCON_E_GROUP_TIMEOUT:
        return QString("Timeout in wait of synchron call");
    case FALCON_E_GROUP_WAITFAILED:
        return QString("Fail in wait of synchron call");
    case FALCON_E_EDI_WRITEERROR:
        return QString("EDI write error");
    case FALCON_E_ADVISE_FAILED:
        return QString("Advise within Falcon component failed");
    case FALCON_E_DEV_INDADDR:
        return QString("Reading individual address from device failed");
    case FALCON_E_DEV_DOMADDR:
        return QString("Reading domain address from device failed");
    case FALCON_E_VERIFY:
        return QString("Verifying the data after beeing written has failed, write should be regarded as failed");
    case FALCON_E_INVALID_SAP_TABLE:
        return QString("Falcon invalid SAP table");
    case FALCON_E_DEVICEALREADYINUSE:
        return QString("A connection to the device is already established");
    case FALCON_E_DEMOMODE:
        return QString("Falcon running in demo mode. Demo mode running time has expired");
    case FALCON_E_SENDFAILED:
        return QString("Sending a message to the bus failed");
    case FALCON_E_NORESPONSE:
        return QString("No response received");
    case FALCON_E_USERMESSAGEINDEX:
        return QString("Invalid usermessage index specified");
    case FALCON_E_ADCREADRESPONSE:
        return QString("Invalid channel number specified or ADC value overflow");
    case FALCON_E_ADC_CHANNEL:
        return QString("Specified channel number is out of range");
    case FALCON_E_ADC_READCOUNT:
        return QString("Specified read count is out of range");
    case FALCON_E_ASYNCSCANRUNNING:
        return QString("An asynchron individual address scan is already running");
    case FALCON_E_OPENDRIVER:
        return QString("Open/Initialize of the driver failed");
    case FALCON_E_EDIINIT:
        return QString("Open of EDI failed");
    case FALCON_E_INVALIDOPENPARAMS:
        return QString("The specified open parameter is invalid");
    case FALCON_E_BADINDEX:
        return QString("EIB object or property doesn't exist or access is denied");
    case FALCON_E_NODEVPRGMODE:
        return QString("Physical/Domain address write: no device in programming mode");
    case FALCON_E_LOCALGATEWAYINIT:
        return QString("Error while initializing local gateway");
    case FALCON_E_CANCELED:
        return QString("The operation has been canceled by the client");
    case FALCON_E_LICKEY_ALREADYINUSE:
        return QString("A connection with the same licence key is already open to another gateway, please use other licence key");
    case FALCON_E_RECONNECT_FAILED:
        return QString("The reconnect to a remote gateway (e.g. EIBlib/IP) failed or is currently in progress but has not succeeded yet");
    case FALCON_E_INVALIDROUTINGCOUNTER:
        return QString("The routing counter was out of range (0 to 7)");
    case FALCON_E_INVALIDPRIORITY:
        return QString("The value of priority (enumeration) is not valid");
    case FALCON_E_DOMAINADDRWRITE_VERIFY:
        return QString("Verifying the domain address after beeing written has failed, write should be regarded as failed");
    case FALCON_E_PHYSADDRWRITE_VERIFY:
        return QString("Verifying the individual address after beeing written has failed, write should be regarded as failed");
    case FALCON_E_INVALID_PROPDATATYPE:
        return QString("The property datatype was invalid");
    case FALCON_E_DEVICE_RESET:
        return QString("Error during device reset");
    case FALCON_E_UNIQUENAME_VIOLATION:
        return QString("The connection name is not unique");
    case FALCON_E_OBJECT_ALREADY_IN_USE:
        return QString("The object is already in use and the method must not be called twice on the system");
    case FALCON_E_ADDRESSUSED_BY_LOCAL_GATEWAY:
        return QString("Physical/Domain address write: address already in use by local device");
    case FALCON_E_CONNECT:
        return QString("The connect to the remote gateway failed");
    case FALCON_E_DEVICERESET_VERIFY:
        return QString("The reset of the remote gateway failed");
    case FALCON_E_PROPDATA_BYTECOUNT:
        return QString("The number of bytes of the property response is not equal to that of the requested property data type");
    case FALCON_E_OPEN_IN_DIFFERENT_LAYER:
        return QString("The local gateway is already in use with an incompatible mode");
    case FALCON_E_ONLYDRIVERMODECON:
        return QString("The property or method cannot be called with only opened drivermode connection");
    case FALCON_E_READLOCALINDADDR:
        return QString("Error reading the local individual address");
    case FALCON_E_INTERNALCONNECTIONALREADYOPEN:
        return QString("The internal connection is already opened");
    case FALCON_E_DRIVERNOTOPEN:
        return QString("The driver is not open");
    case FALCON_E_TLALREADYOPEN:
        return QString("TransportLayer Connection already opened");
    case FALCON_E_USB_DEVICE_NOT_FOUND:
        return QString("The USB device could not be found");
    case FALCON_E_DEVICE_DISCONNECTED:
        return QString("The remote device has disconnected");
    case FALCON_E_INVALIDGROUPADDRESS:
        return QString("Invalid GroupAddress value");
    case FALCON_E_SERIAL_EDI_MORE_THAN_ONE:
        return QString("The serial EDIs do not allow opening more than one bus connection via different serial ports, e.g. COM1 and COM2");
    case FALCON_E_BUSMONMODE_NOT_SUPPORTED_BY_EDI:
        return QString("The EDIs does not support busmon mode connection");
    case FALCON_E_HOST_NOT_FOUND:
        return QString("The host could not be found, e.g. DNS name could not be resolved");
    case FALCON_E_OPEN_USB_DRIVER:
        return QString("The USB driver could not be opened, e.g. there is no USB device connected to the computer");
    case FALCON_E_VERIFY_MODE:
        return QString("The management server did not send a memory response APCI after the memory write request");
    case FALCON_E_MAX_CONNECTIONS_REACHED:
        return QString("The tunneling server is available in general but does not accept more connections at this point of time");
    case FALCON_E_REQUESTEDLAYER_NOT_SUPPORTED_BY_EDI:
        return QString("The EDI does not support the requested layer");
    case FALCON_E_MORE_THAN_ONE_DEVICE_WITH_SAME_IA:
        return QString("More than one device exists with the same individual address");
    case FALCON_E_DEVICE_HAS_SAME_IA:
        return QString("The device has already the individual address");
    case FALCON_E_DEVICE_CURRENT_IA_NOT_FOUND:
        return QString("The the current individual address could not be found");
    default:
        return QString("Falcon unknown error");

    }
}
#elif defined (USE_DUMMY_IF)

FalconIF::FalconIF() {
    connectedDevice = NULL;
    working = true;
}

FalconIF::~FalconIF() {

}

bool FalconIF::setConnection() {
    return true;
}

QString FalconIF::connectionDesc() {
    return QString("Dummy");
}

bool FalconIF::connectDevice(KDevice *device) {
    connectedDevice = device;
    return true;
}

bool FalconIF::disconnectDevice() {
    if (connectedDevice == NULL) {
        return false;
    }
    connectedDevice = NULL;
    return true;
}

bool FalconIF::readMemory(uint16_t addr, uint16_t count, uint8_t *data) {
    printf("Dummy interface: Memory read from %04X, count %i\n",addr,count); fflush(stdout);
    for (uint16_t n = 0; n < count; n++) {
        data[n] = 0;
    }
    return true;
}

bool FalconIF::writeMemory(uint16_t addr, uint16_t count, uint8_t *data, bool verify) {
    printf("Dummy interface: Memory write to %04X, count %i, data",addr,count);
    for (uint16_t n = 0; n < count; n++) {
        printf(" %02X",data[n]);
    }
    printf("\n"); fflush(stdout);
    return true;
}

bool FalconIF::readProperty(uint8_t object, uint8_t property, EibPropertyDataType datatype, uint16_t start, uint16_t nelem, uint8_t *data) {
    printf("Dummy interface: Property read from %02X/%02X, start %i\n",object,property,start); fflush(stdout);
    return true;
}

bool FalconIF::writeProperty(uint8_t object, uint8_t property, EibPropertyDataType datatype, uint16_t start, uint16_t nelem, uint8_t elemsize, uint8_t *data, bool verify = false) {
    printf("Dummy interface: Property write to %02X/%02X, start %i, data",object,property,start);
    for (int idx = 0; idx < nelem; idx++) {
        for (int n = 0; n < elemsize; n++) {
            printf(" %02X",data[idx*elemsize+n]);
        }
        printf("  ");
    }
    printf("\n"); fflush(stdout);
    return true;
}

bool FalconIF::restart() {
    if (connectedDevice == NULL) {
        return false;
    }
    printf("Dummy interface: Reset %s\n",connectedDevice->getPA().toLocal8Bit().data()); fflush(stdout);
    return true;
}

bool FalconIF::setPA(KDevice *device) {
    if (connectedDevice == NULL) {
        return false;
    }
    printf("Dummy interface: Set PA %s\n",connectedDevice->getPA().toLocal8Bit().data()); fflush(stdout);
    return true;
}

#else
#error No valid interface defined
#endif
