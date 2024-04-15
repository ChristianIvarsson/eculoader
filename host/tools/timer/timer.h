#ifndef TIMER_H
#define TIMER_H

#include <cstdio>
#include <cstdint>

namespace timer
{
    void sleepMS(const uint32_t msTime);
    void sleepMicro(const uint32_t uTime);
}

#endif
