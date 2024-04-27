#ifndef __ACDELCO_E39_H__
#define __ACDELCO_E39_H__

#include "../../protocol/gmloader.h"
#include "../target.h"

// Is it possible to make these constexpr so that it can be placed in the class?
// TODO: Clean BAM ids
static const std::list<uint32_t> e39_idListBAM = { 0x7e0, 0x7e8, 0x101, 0x011, 0x002, 0x003 };
static const std::list<uint32_t> e39_idList = { 0x7e0, 0x7e8, 0x101 };

class e39 : public gmloader, public ecu_operation
{
    void configProtocol();
    bool secAccE39(uint8_t);

    bool initSessionE39();
    bool initSessionE39A();
    bool initSessionBAM();

public:
    explicit e39();
    ~e39();

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

    // Override of target.h / ecu_operation {}
    bool dump(const char *name, const ECU & target);
};

#endif