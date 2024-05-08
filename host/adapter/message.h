#ifndef MESSAGE_H
#define MESSAGE_H

#include <cstdio>
#include <mutex>

namespace msgsys
{
	enum enMsgFlgs : uint32_t
	{
		messageRemote   = ( 1UL << 6 ),
		messageExtended = ( 1UL << 7 ),
	};

    typedef struct {
        uint64_t id;
        uint64_t timestamp;
        uint32_t typemask; // mask of "messagetypes" flags
        uint32_t length;
        // Data must be kept last due to how messages are copied
        uint8_t  message[8];
    } message_t;

    message_t  newMessage(uint64_t id, uint32_t len);
	void       setupWaitMessage(uint64_t id);
	message_t *waitMessage(uint32_t waitms);
    void       queueInit();
    void       messageReceive(message_t *msg);
}

#endif
