#ifndef __TARGET_H__
#define __TARGET_H__

#include "../ecuenum.h"

// Is it possible to make these constexpr so that it can be placed in the class?
// TODO: Clean BAM ids
static const std::list<uint32_t> e39_idListBAM = { 0x7e0, 0x7e8, 0x101, 0x011, 0x002, 0x003 };
static const std::list<uint32_t> e39_idList = { 0x7e0, 0x7e8, 0x101 };

class ecu_target
{

public:
    virtual bool dump(const char *name, const ECU & target) = 0;

    // This may seem weird but it's annoying to reimplement this for every target. Let's have one central point for it
    static bool adapterConfig(channelData & dat, const ECU & ecu)
    {
        switch( ecu )
        {
        case ecu_AcDelcoE39:
        case ecu_AcDelcoE39A:
            dat.canIDs = e39_idList;
            dat.bitrate = btr500k;
            return true;
        case ecu_AcDelcoE39BAM:
            dat.canIDs = e39_idListBAM;
            dat.bitrate = btr400k;
            return true;
        default: break;
        }
        return false;
    }
};

#endif
