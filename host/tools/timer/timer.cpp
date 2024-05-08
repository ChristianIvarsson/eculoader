#include "timer.h"

#include <condition_variable>
#include <mutex>
#include <cstdio>
#include <string>
#include <chrono>

#if defined (_WIN32)
#include <windows.h>
#else
#include <time.h>
#endif

using namespace std;
using namespace std::chrono;

namespace timer
{

void sleepMilli(const uint32_t msTime)
{
#if defined (_WIN32)

    Sleep( msTime );

#elif _POSIX_C_SOURCE >= 199309L

    struct timespec ts;
    ts.tv_sec = msTime / 1000;
    ts.tv_nsec = (msTime % 1000) * 1000000;
    nanosleep(&ts, NULL);

#else

    if (msTime >= 1000)
      sleep(msTime / 1000);
    usleep((msTime % 1000) * 1000);

#endif
}

void sleepMicro(const uint32_t uTime)
{
#if defined (_WIN32)

// Can't sleep for less than 15ms so the hogger has to stay...
    auto oldTime = system_clock::now();
    while (duration_cast<microseconds>(system_clock::now() - oldTime).count() < uTime)
        ;

    // this_thread::sleep_for(std::chrono::microseconds( uTime ));

#elif _POSIX_C_SOURCE >= 199309L

    struct timespec ts;
    ts.tv_sec = uTime / 1000000;
    ts.tv_nsec = (uTime % 1000000) * 1000;
    nanosleep(&ts, NULL);

#else

    if (uTime >= 1000000)
      sleep(uTime / 1000000);
    usleep(uTime % 1000000);

#endif
}

}
