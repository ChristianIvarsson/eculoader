#include <condition_variable>
#include <mutex>
#include <cstdlib>
#include <chrono>
#include <cstring>

#include "message.h"
#include "../tools/tools.h"

#include <cstdlib>
#include <chrono>

#include <pthread.h>
#include <time.h>

using namespace timer;
using namespace msgsys;
using namespace logger;
using namespace checksum;
using namespace std;
using namespace std::chrono;

namespace msgsys
{

// Must be power of two
#define qCOUNT   ( 8192 )

static mutex               msgMutex;    // Lock access while pushing or popping the queue
static volatile uint32_t   inPos   = 0; // Positions in queue. 0 to qCOUNT - 1
static volatile uint32_t   outPos  = 0;
static volatile uint32_t   inQueue = 0; // How many queued messages. 0 to qCOUNT
static message_t           messageQueue[ qCOUNT ];
static volatile uint64_t   WaitMessageID = ~0;

// TODO: Actualyl use the mutex?
pthread_mutex_t    mtxCond = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t     timedCond;
pthread_condattr_t attr;

static inline void printMessageContents(string what, message_t *msg)
{
    string strResp = what + ": ";

    strResp += to_hex((uint16_t)msg->id, 2) + " "; 
    for (uint32_t i = 0; i < msg->length; i++)
        strResp += to_hex((uint16_t)msg->message[i], 1);

    log("recd: " + strResp + "   (" + to_string_fcount(msg->timestamp, 6) + ")");
}

static message_t *popMessage()
{
    static message_t tempMessage;

    msgMutex.lock();

    if (inQueue)
    {
        memcpy(&tempMessage, &messageQueue[outPos++], sizeof(message_t));
        outPos &= (qCOUNT-1);
        inQueue--;

        msgMutex.unlock();
        return &tempMessage;
    }

    // ~no message available~
    msgMutex.unlock();

    return nullptr;
}

message_t newMessage(uint64_t id, uint32_t len)
{
    message_t msg = {0};
    msg.id = id;
    msg.length = len;

    // memset(&msg.message, 0, len);

    return msg;
}

// adapter::open is calling this just before a channel is opened
void queueInit()
{
    msgMutex.lock();

    inPos   = 0;
    outPos  = 0;
    inQueue = 0;

    msgMutex.unlock();

    pthread_condattr_init( &attr );
    pthread_condattr_setclock( &attr, CLOCK_MONOTONIC );
    pthread_cond_init( &timedCond, &attr );
}

void setupWaitMessage(uint64_t id)
{
    msgMutex.lock();

    // Clear all old messages
    inPos   = 0;
    outPos  = 0;
    inQueue = 0;

	WaitMessageID = id;

    msgMutex.unlock();
}


// This thing is a resource hog!!
// Complete notifiers...
message_t *waitMessage(uint32_t waitms)
{
    static message_t nullMessage;
	message_t *msg;
    struct timespec to;

	auto oldTime = system_clock::now();

	do {
		if ((msg = popMessage()) != nullptr && msg->id == WaitMessageID)
		    return msg;

// TODO: What happens when nsec wraps?
        clock_gettime(CLOCK_MONOTONIC, &to);
        to.tv_nsec += 1000 * 50000;
        pthread_cond_timedwait(&timedCond, &mtxCond, &to);

	} while (duration_cast<milliseconds>(system_clock::now() - oldTime).count() < waitms);

	memset(&nullMessage, 0, sizeof(message_t));
	return &nullMessage;
}

// Push received messages to the queue
void messageReceive(message_t *msg)
{
    // msg->timestamp = (uint64_t) (duration_cast< microseconds >(system_clock::now().time_since_epoch()).count() % 1000000);
    // printMessageContents("", msg);

    msgMutex.lock();

    message_t *qmsg = &messageQueue[inPos++];
	inPos &= (qCOUNT-1);
/*
    qmsg->id        = msg->id;

    // canusb is using a serial protocol, ie SLOW as all heck, so adapter timestamps can't be used..
    // ..Maybe it's better just to use local host time in that case?
    qmsg->timestamp = 0; // Implement something useful here..

    qmsg->typemask  = msg->typemask;
    qmsg->length    = msg->length;
    memcpy(qmsg->message, msg->message, msg->length);
*/

    // Enough yapping..
    memcpy( qmsg, msg, (sizeof(message_t) - sizeof(msg->message)) + msg->length );

    if ( msg->length < sizeof(msg->message) )
    {
        memset( &qmsg->message[ msg->length ], 0, sizeof(msg->message) - msg->length );
    }

    // Still room to spare
    if (inQueue < qCOUNT)
    {
        inQueue++;
    }

    // Buffer is full; The oldest message pointed to by outPos has been overwritten.
    // Update pointer to the next oldest message
    else
    {
        outPos++;
        outPos &= (qCOUNT-1);
    }

    msgMutex.unlock();

    // Notify pthread_cond_timedwait() in waitMessage()
    pthread_cond_signal( &timedCond );
}

};
