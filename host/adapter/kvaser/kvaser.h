#ifndef KVASER_H
#define KVASER_H

#include <thread> 
#include "../adapter.h"
#include "../../tools/tools.h"

extern "C"
{
#if defined (_WIN32)
#include "../../libs/kvaser/inc/canlib.h"
#else
#include "../../libs/kvaser/linux/include/canlib.h"
#endif
}

class kvaser : public adapter_t
{
    bool CalcAcceptanceFilters(std::list<uint64_t>&) ;
    bool m_open(channelData, int);
    CanHandle kvaserHandle = -1;

public:
    explicit kvaser();
    ~kvaser();

    bool open(channelData&);
    bool close();
    bool send(msgsys::message_t*);

    std::list <std::string> adapterList();
};

#endif
