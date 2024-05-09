#ifndef __ACDELCO_E39_H__
#define __ACDELCO_E39_H__

#include "../../protocol/gmloader.h"
#include "../target.h"

class e39 : public gmloader, public ecu_target
{
    void configProtocol();
    bool secAccE39(uint8_t);

    bool initSessionE39();
    bool initSessionE39A();
    bool initSessionBAM();

public:
    explicit e39();
    ~e39();

    // Overrides of target.h / ecu_target {}
    bool dump  (const char *name, const ECU & target);
    bool flash (const char *name, const ECU & target);
};

#endif