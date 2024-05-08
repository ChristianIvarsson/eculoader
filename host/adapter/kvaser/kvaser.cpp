#include <cstdio>
#include <string>
#include <list>
#include <iostream>

#include "kvaser.h"

using namespace std;
using namespace msgsys;
using namespace logger;

kvaser::kvaser()
{
    canInitializeLibrary();
}

kvaser::~kvaser()
{
    this->close();
}

list <string> kvaser::adapterList()
{
    list<string> adapters;
    uint32_t card_channel = 0;
    int32_t num = 0;
    char name[256];

    canStatus stat = canGetNumberOfChannels(&num);

    for (int i = 0; (i < num) && (stat == canOK); i++)
    {
        name[0] = 0;
        stat = canGetChannelData(i, canCHANNELDATA_DEVDESCR_ASCII, name, sizeof(name));
        if (stat != canOK) continue;

        stat = canGetChannelData(i, canCHANNELDATA_CHAN_NO_ON_CARD, &card_channel, sizeof(card_channel));
        if (stat != canOK) continue;

        string strname = toString(name) + " P" + to_string(card_channel);
        adapters.push_back(strname);
    }

    return adapters;
}

bool kvaser::close()
{
    if (kvaserHandle != -1)
    {
        canBusOff((CanHandle)kvaserHandle);
        canClose((CanHandle)kvaserHandle);
        kvaserHandle = -1;
    }

    return true;
}


bool kvaser::CalcAcceptanceFilters(list<uint64_t> &idList) 
{
    unsigned int code = ~0;
    unsigned int mask = 0;

    if (idList.size() > 0)
    {
        for (uint32_t canID : idList)
        {
            log(adapterlog, "Filter+ " + to_hex(canID));
            if (canID == 0)
            {
                log(adapterlog, "Found illegal id");
                code = ~0;
                mask = 0;
                goto forbidden;
            }

            code &= canID;
            mask |= canID;
        }

        mask = ~(code ^ mask);
        // mask = (~mask & 0x7FF) | code;
    }

forbidden:

    // Not implemented??
/*
    if (canSetAcceptanceFilter(kvaserHandle, code, mask, 0) != canOK)
    {
        log(adapterlog, "Could not set acceptance filters");
        return false;
    }
*/
    return true;
}

void kvasOnCall(CanHandle hnd, void* context, unsigned int evt)
{
    uint8_t msgData[128];
    message_t msg;
    unsigned int dlc, flags;
    unsigned long ts;
    long ID;

    if (evt & canNOTIFY_RX)
    {
        while (canRead(hnd, &ID, msgData, &dlc, &flags, &ts) == canOK)
        {
            if (dlc > 8) {
                log(adapterlog, "Kvaser: unsup long msg");
                dlc = 8;
            }

            for (uint32_t i = 0; i < dlc; i++)
                msg.message[i] = msgData[i];

            for (uint32_t i = dlc; i < 8; i++)
                msg.message[i] = 0;

            msg.length = dlc;
            msg.timestamp = ts;
            msg.id = ID;
            messageReceive(&msg);
        }
    }
}

bool kvaser::m_open(channelData device, int chan)
{
    int32_t btrBits = canBITRATE_500K;

    switch ( device.bitrate )
    {
    case btr200k:
        btrBits = (int32_t)sja200k;
        break;

    case btr300k:
        btrBits = (int32_t)sja300k;
        break;

    case btr400k:
        btrBits = (int32_t)sja400k;
        break;

    case btr500k:
        btrBits = canBITRATE_500K;
        break;

    case btr615k:
        btrBits = (int32_t)sja615k;
        break;

    default:
        log(adapterlog, "Unknown baud enum");
        return false;
    }

    if ( (kvaserHandle = (int32_t)canOpenChannel(chan, 0)) < 0 )
    {
        log(adapterlog, "Could not retrieve handle");
        return false;
    }

    if ( btrBits < 0 )
    {
        // Negative values are predefined
        if ( canSetBusParams(kvaserHandle, btrBits, 0, 0, 0, 0, 0) != canOK )
        {
            log(adapterlog, "Could not set bus parameters");
            return false;
        }
    }
    else
    {
        if ( canSetBusParamsC200(kvaserHandle, (unsigned char)(btrBits >> 8), (unsigned char)btrBits) != canOK )
        {
            log(adapterlog, "Could not set bus parameters");
            return false;
        }
    }

    if (canBusOn(kvaserHandle) != canOK)
    {
        log(adapterlog, "Could not open bus");
        return false;
    }

    // canNOTIFY_REMOVED is not present on Linux
#if defined (_WIN32)
    if (kvSetNotifyCallback(kvaserHandle, &kvasOnCall, 0, canNOTIFY_RX | canNOTIFY_ERROR | canNOTIFY_REMOVED) != canOK) {
#else
    if (kvSetNotifyCallback(kvaserHandle, &kvasOnCall, 0, canNOTIFY_RX | canNOTIFY_ERROR) != canOK) {
#endif
        log(adapterlog, "Could not install callback");
        return false;
    }

    return CalcAcceptanceFilters(device.idList);
}

bool kvaser::open(channelData & device)
{
    string   name = device.name;
    uint32_t card_channel = 0;
    int32_t  num = 0;
    char cname[100];

    if (name.length() == 0)
    {
        return false;
    }

    log(adapterlog, "Trying to open " + name);

    canStatus stat = canGetNumberOfChannels(&num);

    for (int i = 0; (i < num) && (stat == canOK); i++)
    {
        cname[0] = 0;
        stat = canGetChannelData(i, canCHANNELDATA_DEVDESCR_ASCII, cname, sizeof(cname));
        if (stat != canOK) continue;

        stat = canGetChannelData(i, canCHANNELDATA_CHAN_NO_ON_CARD, &card_channel, sizeof(card_channel));
        if (stat != canOK) continue;

        string strname = toString(cname) + " P" + to_string(card_channel);

        if (strname == name)
        {
            log(adapterlog, "Device located. Opening channel " + to_string(i) + "..");
            this->close();
            return this->m_open(device, i);
        }
    }

    return false;
}

bool kvaser::send(message_t *msg)
{
    if (kvaserHandle >= 0)
        return (canWrite(kvaserHandle, (long)msg->id, &msg->message, (unsigned int)msg->length, 0) == canOK);

    return false;
}
