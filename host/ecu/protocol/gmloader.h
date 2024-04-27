#ifndef __GMLOADER_H__
#define __GMLOADER_H__

#include "gmlan.h"

// Default safe interframe delay
#define GMLOADER_INTERFRAME ( 1200 )

class gmloader : public gmlan
{

public:
    uint8_t *loader_readMemoryByAddress(uint32_t address, uint32_t len, uint32_t blockSize = 0x80);
    bool loader_StartRoutineById(uint8_t service, uint32_t param1, uint32_t param2);
    uint8_t *loader_requestRoutineResult(uint8_t service);
};

#endif
