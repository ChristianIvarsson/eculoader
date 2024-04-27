#ifndef __TARGET_H__
#define __TARGET_H__

#include "../ecuenum.h"

class ecu_operation
{
public:
    virtual bool dump(const char *name, const ECU & target) = 0;
};

#endif
