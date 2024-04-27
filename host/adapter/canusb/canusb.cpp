#include "canusb.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <list>

#include <iostream>

#if !defined (_WIN32)
// libusb 0.1
#include <usb.h>
#include <sys/time.h>
#endif

using namespace std;
using namespace msgsys;
using namespace logger;

#define VENDOR_ID  0x0403
#define PRODUCT_ID 0x6001

#warning "Exceptionally unstable on Windows - fix"

extern "C"
{
#if defined (_WIN32)
    static HMODULE         m_hmodule    = nullptr;
#else
    static usb_dev_handle *deviceHandle = nullptr;
#endif
}

#if defined (_WIN32)

bool canusb::loadLibrary()
{
    log(adapterlog, "Load library");
    if ((m_hmodule = LoadLibrary("ftd2xx64.dll")) == nullptr)
    {
        log(adapterlog, "Could not load ftd2xx.dll");
        return false;
    }

    return true;
}

bool canusb::unloadLibrary()
{
    log(adapterlog, "Unload library");
    if (m_hmodule != nullptr)
    {
        FreeLibrary((HMODULE)m_hmodule);
        m_hmodule = nullptr;
    }

    return true;
}

std::list<std::string> canusb::findCANUSB()
{
    list<string> adapters;
    DWORD numDevices;

    if (m_hmodule == nullptr)
    {
        return adapters;
    }

    if (FT_CreateDeviceInfoList(&numDevices) != FT_OK)
    {
        log(adapterlog, "Could not generate list of ftdi devices");
        return adapters;
    }

    FT_DEVICE_LIST_INFO_NODE *devInfo = (FT_DEVICE_LIST_INFO_NODE *)malloc(sizeof(FT_DEVICE_LIST_INFO_NODE) * numDevices);

    if (numDevices == 0)
    {
        log(adapterlog, "No ftdi devices connected to the system");
        return adapters;
    }

    if (FT_GetDeviceInfoList(devInfo, &numDevices) != FT_OK)
    {
        log(adapterlog, "Could not retrieve list of ftdi devices");
        return adapters;
    }

    for (DWORD i = 0; i < numDevices; i++)
    {
        if (toString(devInfo[i].Description) == "CANUSB")
        {
            adapters.push_back(toString(devInfo[i].SerialNumber));
        }
    }

    free(devInfo);

    return adapters;
}

#else

std::list <std::string> canusb::findCANUSB() 
{
    struct usb_device *dev;
    struct usb_bus *bus;
    char VID[257] = {0};
    char PID[257] = {0};
    char DSN[257] = {0};
    list <string> adapters;

    for (bus = usb_get_busses(); bus; bus = bus->next)
    for (dev = bus->devices;     dev; dev = dev->next)
    {
        if (dev->descriptor.idVendor  == VENDOR_ID && 
            dev->descriptor.idProduct == PRODUCT_ID )
        {
            usb_dev_handle *handle;

            if (!(handle = usb_open(dev)))
            {
                log(adapterlog, "Could not open a device that had id match");
                continue;
            }
            usb_get_string_simple(handle, dev->descriptor.iManufacturer, VID, 256);
            usb_get_string_simple(handle, dev->descriptor.iProduct     , PID, 256);
            usb_get_string_simple(handle, dev->descriptor.iSerialNumber, DSN, 256);
            if (toString(VID) == "LAWICEL" && 
                toString(PID) == "CANUSB"  && 
                toString(DSN).size() > 0    )
            {
                log(adapterlog, "Found it!");
                adapters.push_back(toString(DSN));
            }

            usb_close(handle);
        }
    }

    return adapters;
}

void *canusb::m_open(string &identifier)
{
    struct usb_device *dev;
    struct usb_bus *bus;
    char VID[257] = {0};
    char PID[257] = {0};
    char DSN[257] = {0};

    for (bus = usb_get_busses(); bus; bus = bus->next)
    for (dev = bus->devices;     dev; dev = dev->next)
    {
        if (dev->descriptor.idVendor  == VENDOR_ID && 
            dev->descriptor.idProduct == PRODUCT_ID )
        {
            usb_dev_handle *handle;

            if (!(handle = usb_open(dev)))
                continue;

            usb_get_string_simple(handle, dev->descriptor.iManufacturer, VID, 256);
            usb_get_string_simple(handle, dev->descriptor.iProduct     , PID, 256);
            usb_get_string_simple(handle, dev->descriptor.iSerialNumber, DSN, 256);
            if (toString(VID) == "LAWICEL" && 
                toString(PID) == "CANUSB"  && 
                toString(DSN) == identifier )
            {
                log(adapterlog, "Found device of serial no");
                return handle;
            }

            usb_close(handle);
        }
    }

    return nullptr;
}

#endif

canusb::canusb()
{
#if defined (_WIN32)
    loadLibrary();
#endif
}

canusb::~canusb()
{
    log(adapterlog, "Canusb going down");
    this->close();

#if defined (_WIN32)
    unloadLibrary();
#endif
}

bool canusb::openChannel(channelData & device)
{
    char buf[16] = {0x0d, 0x0d, 0x0d, 0};
    DWORD retLen;
    uint32_t btrBits = 6;
    bool predefBtr = false;

    // Clear old commands
    FT_Purge(canusbContext, FT_PURGE_RX);
    if (FT_Write(canusbContext, buf, 3, &retLen) != FT_OK)
    {
        log(adapterlog, "Write failed");
        return false;
    }

    switch ( device.bitrate )
    {

    case btr200k:
        btrBits = (uint32_t)sja200k;
        break;

    case btr300k:
        btrBits = (uint32_t)sja300k;
        break;

    case btr400k:
        btrBits = (uint32_t)sja400k;
        break;

    case btr500k:
        predefBtr = true;
        btrBits = 6;
        break;

    case btr615k:
        btrBits = (uint32_t)sja615k;
        break;

    default:
        log(adapterlog, "Unknown baud enum");
        return false;
    }

    // Set CAN baudrate
    if ( predefBtr )
    {
        sprintf(buf, "S%u\r", btrBits);
        FT_Purge(canusbContext, FT_PURGE_RX);
        if (FT_Write(canusbContext, buf, 3, &retLen) != FT_OK)
        {
            log(adapterlog, "Write failed");
            return false;
        }
    }
    else
    {
        sprintf(buf, "s%04X\r", btrBits);
        FT_Purge(canusbContext, FT_PURGE_RX);
        if (FT_Write(canusbContext, buf, 6, &retLen) != FT_OK)
        {
            log(adapterlog, "Write failed");
            return false;
        }
    }

    sprintf(buf, "Z0\r"); // Get lost timestamps
    FT_Purge(canusbContext, FT_PURGE_RX);
    if (FT_Write(canusbContext, buf, 3, &retLen) != FT_OK)
    {
        log(adapterlog, "Write failed");
        return false;
    }

    if (!CalcAcceptanceFilters(device.canIDs))
    {
        log(adapterlog, "Couldn't set can filters");
        return false;
    }

    // Open device
    strcpy( buf, "O\r" );
    FT_Purge(canusbContext, FT_PURGE_RX);
    return  (FT_Write(canusbContext, buf, 2, &retLen) == FT_OK) ? true : false;
}

bool canusb::CalcAcceptanceFilters(list<uint32_t> idList)
{
    uint32_t code = ~0;
    uint32_t mask = 0;
    DWORD retLen;
    char buf[16];

    if (idList.size() == 0)
    {
        code = 0;
        mask = ~0;
    }
    else
    {
        for (uint32_t canID : idList)
        {
            if (canID == 0)
            {
                log(adapterlog, "Found illegal id");
                code = 0;
                mask = ~0;
                break;
            }
            code &= (canID & 0x7FF) << 5;
            mask |= (canID & 0x7FF) << 5;
        }
    }

    code |= code << 16;
    mask |= mask << 16;

    sprintf(buf, "M%08X\r", code);
    FT_Purge(canusbContext, FT_PURGE_RX);
    if (FT_Write(canusbContext, buf, 10, &retLen) != FT_OK)
    {
        return false;
    }

    sprintf(buf, "m%08X\r", mask);
    FT_Purge(canusbContext, FT_PURGE_RX);
    return (FT_Write(canusbContext, buf, 10, &retLen) == FT_OK) ? true : false;
}


list<string> canusb::adapterList()
{

#if !defined (_WIN32)
    // Verbosity is nice and all but this is just ANNOYING
    // usb_set_debug(255);
    usb_init();
    usb_find_busses();
    usb_find_devices();
#endif

    return findCANUSB();
}

// CAN Frame
typedef struct {
  unsigned long id;         // Message id
  unsigned long timestamp;  // timestamp in milliseconds
  unsigned char flags;      // [extended_id|1][RTR:1][reserver:6]
  unsigned char len;        // Frame size (0.8)
  unsigned char data[8];    // Databytes 0..7
} CANMsg;

bool canusb::send(message_t *msg)
{
#if defined (_WIN32)
    long unsigned int retLen;
#else
    uint32_t retLen;
#endif
    char txbuf[80];
    char hex[5];

    sprintf(txbuf, "t%03X%i", (uint32_t)msg->id, (uint32_t)msg->length);

    for (uint32_t i = 0; i < msg->length; i++)
    {
        sprintf(hex, "%02X", msg->message[i]);
        strcat(txbuf, hex);
    }

    // Add CR
    strcat(txbuf, "\r");

    // Transmit frame
    return (FT_Write(canusbContext, txbuf, strlen(txbuf), &retLen) == FT_OK) ? true : false;
}

/*
bool canusb::closeChannel()
{
    char buf[3] = { 'C', '\r' };
    uint32_t retLen;

    // Close device
    FT_Purge(canusbContext, FT_PURGE_RX | FT_PURGE_TX);

    return  (FT_Write(canusbContext, buf, 2, &retLen) == FT_OK) ? true : false;
}*/

bool canusb::closeChannel()
{
    char buf[ 3 ] = { 'C', '\r' };
    FT_STATUS status = FT_OK;
    
    // log(adapterlog, "Adapter close");
    if (canusbContext != nullptr)
    {
        // Close device
        FT_Purge(canusbContext, FT_PURGE_RX | FT_PURGE_TX);

        DWORD retLen;
        FT_Write(canusbContext, buf, 2, &retLen);

        status = FT_Close(canusbContext);
    }

    if (status == FT_OK)
    {
        return true;
    }

    log(adapterlog, "Could not close transport to adapter");
    return false;
}

static inline void canusbToCanMsg(char *p, message_t *msg)
{
    int val;
    uint32_t data_offset; // Offset to dlc byte
    char save;

    msg->typemask = 0;

    switch (*p)
    {
    case 'r': // Standard remote frame
        msg->typemask |= messageRemote;
    case 't': // Standard frame
        data_offset = 5;
        msg->length = p[4] - '0';
        p[4] = 0;
        break;
    case 'R': // Extended remote frame
        msg->typemask |= messageRemote;
    case 'T': // Extended frame
        msg->typemask |= messageExtended;
        data_offset = 10;
        msg->length = p[9] - '0';
        p[9] = 0;
        break;
    default:
        log(adapterlog, "canusbToCanMsg: Big oops!");
        return;
    }

#if defined (_WIN32)
    sscanf(p + 1, "%llx", &msg->id);
#else
    sscanf(p + 1, "%lx", &msg->id);
#endif

    save = *(p + data_offset + 2 * msg->length);

    // Fill in data
    if (!(msg->typemask & messageRemote))
    {
        uint32_t ln = msg->length;
        if (ln > 8) ln = 8;
        for (uint32_t i = ln; i > 0; i--)
        {
            *(p + data_offset + 2 * (i - 1) + 2) = 0;
            sscanf(p + data_offset + 2 * (i - 1), "%x", &val);
            msg->message[i - 1] = (uint8_t)val;
        }
    }

    *(p + data_offset + 2 * msg->length) = save;
    msg->timestamp = 0;

    // Not remote frame
    if (!(msg->typemask & messageRemote))
    {
        // If timestamp is active - fetch it
        if (*(p + data_offset + 2 * msg->length) != 0x0d)
        {
            p[data_offset + 2 * (msg->length) + 4] = 0;
            sscanf((p + data_offset + 2 * (msg->length)), "%x", &val);
            msg->timestamp = (uint8_t)val;
        }
    }

    // Remote frame
    else
    {
        if (*(p + data_offset) != 0x0d)
        {
            p[data_offset + 4] = 0;
            sscanf((p + data_offset), "%x", &val);
            msg->timestamp = val;
        }
    }

    messageReceive( msg );
}

#if defined (_WIN32)

void canusb::messageThread(canusb *thisInstance)
{
    char gbufferRx[4096], msgReceiveBuf[80];
    DWORD eventStatus, nRcvCnt, nRxCnt, nTxCnt;
    message_t msg;
    bool inMessage = false;
    uint32_t cntMsgRcv = 0;

    HANDLE hEvent = CreateEvent(NULL,false,false,"");

    FT_SetEventNotification(thisInstance->canusbContext, FT_EVENT_RXCHAR, hEvent);

    // Start fresh
    FT_Purge(thisInstance->canusbContext, FT_PURGE_RX);

    while (thisInstance->runThread)
    {
        WaitForSingleObject(hEvent, 500);

        uint32_t getstat = FT_GetStatus(thisInstance->canusbContext, &nRxCnt, &nTxCnt, &eventStatus);
        if (getstat != FT_OK && thisInstance->runThread)
        {
            log(adapterlog, "ft_status: " + to_hex(getstat));
        }

        if (nRxCnt)
        {
            if (nRxCnt > sizeof(gbufferRx))
            {
                log(adapterlog, "Overflow!");
                nRxCnt = sizeof(gbufferRx);
            }

            if (FT_Read(thisInstance->canusbContext, gbufferRx, nRxCnt, &nRcvCnt) == FT_OK)
            {
                for (uint32_t i = 0; i < nRcvCnt; i++)
                {
                    // Get character
                    char c = gbufferRx[i];

                    // Consecutive data
                    if (inMessage)
                    {
                        msgReceiveBuf[cntMsgRcv++] = c;

                        if (c == '\r')
                        {
                            canusbToCanMsg(msgReceiveBuf, &msg);
                            inMessage = false;
                        }

                        if (cntMsgRcv > (1 + 3 + 1 + 16 + 1))
                            log(adapterlog, "cntMsgRcv >!");
                    }

                    // Start of message
                    else
                    {
                        switch (c)
                        {
                        case '\a': // Error
                            log(adapterlog, "error response");
                            break;
                        case '\r':
                            inMessage = false;
                            break;
                        case 'R': // Remote extended frame
                        case 'r': // Remote standard frame
                        case 'T': // Extended frame
                        case 't': // Standard frame
                            inMessage = true;
                            msgReceiveBuf[0] = c;
                            cntMsgRcv = 1;
                            break;
                        case 'Z': // Response to sent extended frame
                        case 'z': // Response to sent standard frame
                            break;
                        default:
                            log(adapterlog, "Unknown char: " + to_hex((uint16_t)c, 1));
                            break;
                        }
                    }
                }
            }
            else if ( thisInstance->runThread )
            {
                log(adapterlog, "FT_Read failed");
            }
        }
    }

    log(adapterlog, "Message thread going down");
}

bool canusb::open(channelData & device)
{
    string identifier = device.name;

    this->close();
    DWORD retLen;

    log(adapterlog, "OpenEx");

    canusbContext = nullptr;

    if (FT_OpenEx((void*)identifier.c_str(), FT_OPEN_BY_SERIAL_NUMBER, &canusbContext) == FT_OK)
    {
        if (canusbContext == nullptr)
        {
            log(adapterlog, "FT_OpenEx nullptr");
            return false;
        }

        char buf[3] = {'C', '\r', 0};
        FT_Purge(canusbContext, FT_PURGE_RX | FT_PURGE_TX);
        FT_Write(canusbContext, buf, 2, &retLen);

        if (FT_ResetDevice(canusbContext) != FT_OK)
        {
            log(adapterlog, "Could not reset device");
            return false;
        }

        // Set latency to 2 ms.
        if (FT_SetLatencyTimer(canusbContext, 2) != FT_OK)
        {
            log(adapterlog, "Could not set latency to two ms");
            return false;
        }

        if (FT_SetBaudRate(canusbContext, FT_BAUD_921600) != FT_OK)
        {
            log(adapterlog, "Could not baud");
        }

        if (!openChannel(device))
        {
            log(adapterlog, "Open channel fail");
            this->close();
            return FALSE;
        }

        log(adapterlog, "Channel open");

        runThread = true;

        messageThreadPtr = new thread(messageThread, this);

        return true;
    }
    else
    {
        log(adapterlog, ".. failed");

        canusbContext = nullptr;

        this->close();
    }

    return false;
}

bool canusb::close()
{
    runThread = false;

    if (!closeChannel())
    {
        log(adapterlog, "Could not close channel");
    }

    if ( messageThreadPtr != nullptr )
    {
        messageThreadPtr->join();
        messageThreadPtr = nullptr;
    }

    canusbContext = nullptr;
    return true;
}

#else

void canusb::messageThread(canusb *thisInstance)
{
    char         gbufferRx[4096], msgReceiveBuf[80];
    uint32_t     nRcvCnt, nRxCnt, nTxCnt;
    uint32_t     EventMask, eventStatus;
    EVENT_HANDLE eh;
    int          rc;
    message_t    msg;
    bool         inMessage = false;
    uint32_t     cntMsgRcv = 0;
    uint32_t     getstat;

    pthread_mutex_init(&eh.eMutex  , NULL);
    pthread_cond_init (&eh.eCondVar, NULL);

    EventMask = FT_EVENT_RXCHAR; //  | FT_EVENT_MODEM_STATUS;
    FT_SetEventNotification(thisInstance->canusbContext,
                            EventMask,
                            (PVOID)&eh);

    timeval  timep;
    timespec time = {0};

    // Start fresh
    FT_Purge(thisInstance->canusbContext, FT_PURGE_RX);

    while(thisInstance->runThread)
    {
        eventStatus = 0;
        nRxCnt = 0;
        rc     = 0;

        pthread_mutex_lock(&eh.eMutex);

        while ((getstat = FT_GetStatus(thisInstance->canusbContext, &nRxCnt, &nTxCnt, &eventStatus)) == FT_OK &&
                eventStatus       == 0 && 
                nRxCnt            == 0 &&
                (rc & ~ETIMEDOUT) == 0 &&
                thisInstance->runThread)
        {
            // Hackjob to prevent deadlock while shutting down
            gettimeofday(&timep, NULL);
            time.tv_sec  = timep.tv_sec + 1;
            time.tv_nsec = timep.tv_usec * 1000;
            rc = pthread_cond_timedwait(&eh.eCondVar, &eh.eMutex, &time);
            // rc = pthread_cond_wait(&eh.eCondVar, &eh.eMutex);
        }

        pthread_mutex_unlock(&eh.eMutex);

        if (!thisInstance->runThread) break;
        if ((rc & ~ETIMEDOUT))        log(adapterlog, "thread cond: " + to_hex(rc));
        if (getstat != FT_OK)         log(adapterlog, "ft_status: " + to_hex(getstat));
        if (eventStatus & ~EventMask) log(adapterlog, "evt_status: " + to_hex(eventStatus));

        if (nRxCnt)
        {
            if (nRxCnt > sizeof(gbufferRx))
            {
                log(adapterlog, "Overflow!");
                nRxCnt = sizeof(gbufferRx);
            }

            if ((rc = FT_Read(thisInstance->canusbContext, gbufferRx, nRxCnt, &nRcvCnt)) == FT_OK)
            {
                for (uint32_t i = 0; i < nRcvCnt; i++)
                {
                    // Get character
                    char c = gbufferRx[i];

                    // Consecutive data
                    if (inMessage)
                    {
                        msgReceiveBuf[cntMsgRcv++] = c;

                        if (c == '\r')
                        {
                            canusbToCanMsg(msgReceiveBuf, &msg);
                            inMessage = false;
                        }

                        if (cntMsgRcv > (1 + 3 + 1 + 16 + 1))
                            log(adapterlog, "cntMsgRcv >!");
                    }

                    // Start of message
                    else
                    {
                        switch (c)
                        {
                        case 0x07: // Error
                            log(adapterlog, "error response");
                            break;
                        case 0x0d: // \n
                            inMessage = false;
                            break;
                        case 'R': // Remote extended frame
                        case 'r': // Remote standard frame
                        case 'T': // Extended frame
                        case 't': // Standard frame
                            inMessage = true;
                            msgReceiveBuf[0] = c;
                            cntMsgRcv = 1;
                            break;
                        case 'Z': // Response to sent extended frame
                        case 'z': // Response to sent standard frame
                            break;
                        default:
                            log(adapterlog, "Unknown char: " + to_hex((uint16_t) c, 1));
                            break;
                        }
                    }
                }
            }
            else
            {
                log(adapterlog, "rc is not 0 (" + to_string(rc) + ")");
            }
        }
    }

    log(adapterlog, "Message thread going down");
}

bool canusb::open(channelData & device)
{
    char dname[32] = {0};

    this->close();

    usb_init();
    usb_find_busses();
    usb_find_devices();

    if ((deviceHandle = (usb_dev_handle*)this->m_open(device.name)))
    {
        // Is it attached to a driver?
        if (usb_get_driver_np((usb_dev_handle*)deviceHandle, 0, dname, 31) == 0)
        {
            // Yup. This is mine, thank you!
            if (usb_detach_kernel_driver_np((usb_dev_handle*)deviceHandle, 0) != 0)
            {
                // Could not take ownership of the interface for one reason or another.
                this->close();
                return false;
            }
        }

        if (FT_OpenEx((void*)device.name.c_str(), FT_OPEN_BY_SERIAL_NUMBER, &canusbContext) == 0)
        {
            // Set latency to 2 ms.
            FT_SetLatencyTimer(canusbContext, 2);

            if (!openChannel(device))
            {
                this->close();
                return FALSE;
            }

            runThread = true;
            messageThreadPtr = new thread(messageThread, this);
            return true;
        }
        else
        {
            canusbContext = nullptr;
            this->close();
        }
    }

    return false;
}

bool canusb::close()
{
    runThread = false;

    if (canusbContext && deviceHandle)
    {
        if (!closeChannel())
        {
            // printf("Failed to close channel\n");
            // return FALSE;
        }
    }

    canusbContext = 0;

    if (messageThreadPtr)
    {
        messageThreadPtr->join();
        messageThreadPtr = 0;
    }

    if ( deviceHandle )
    {
        usb_close((usb_dev_handle*)deviceHandle);
        deviceHandle = 0;
    }

    return true;
}

#endif
