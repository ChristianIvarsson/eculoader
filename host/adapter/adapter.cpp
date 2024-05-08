#include <cstdlib>
#include <chrono>
#include <iostream>

#include "../tools/tools.h"

#include "adapter.h"
#include "canusb/canusb.h"
#include "kvaser/kvaser.h"

#include <iostream>

using namespace std;
using namespace msgsys;
using namespace std::chrono;
using namespace logger;
using namespace timer;

#warning "Fix this. All of it!"

/*
adapter_t::adapter_t()
{

}
*/
adapter_t::~adapter_t()
{
	// cout << "Destroying adapter_t template\r\n";
}


adapter::adapter()
{
	adapterContext = nullptr;
}

adapter::~adapter()
{
	if ( adapterContext != nullptr )
	{
		adapterContext->close();
		adapterContext->~adapter_t();
		adapterContext = nullptr;
	}  
}

bool adapter::setAdapter(adaptertypes & adapter)
{
	// Close old context before opening a new one
	if ( adapterContext != nullptr )
	{
		adapterContext->close();
		adapterContext->~adapter_t();
		adapterContext = nullptr;
	}

	switch ( adapter )
	{
	case adapterCanUsb:
		adapterContext = new canusb();
		break;

	case adapterKvaser:
		adapterContext = new kvaser();
		break;

	default:
		break;
	}

	return ( adapterContext != nullptr ) ? true : false;
}

list <string> adapter::listAdapters(adaptertypes adapter)
{
	// Fallback
	static list <string> noList;

	if ( !setAdapter(adapter) )
	{
		log(adapterlog, "no set");
		return noList;
	}

	return adapterContext->adapterList();	
}

bool adapter::open(channelData & device)
{
	queueInit();

	if ( adapterContext == nullptr )
	{
		log(adapterlog, "No loaded adapter!");
		return false;
	}

	if ( adapterContext->open(device) )
	{
        sleepMilli( 1000 );
		return true;
	}

	return false;
}

bool adapter::close()
{
	bool retval = true;

	if ( adapterContext != nullptr )
	{
		retval = adapterContext->close();
		adapterContext = nullptr;
	}

	return retval;
}

static inline void printMessageContents(string what, message_t *msg)
{
    string strResp = what + ": ";

    strResp += to_hex((uint16_t)msg->id, 2) + " "; 
    for (uint32_t i = 0; i < msg->length; i++)
        strResp += to_hex((uint16_t)msg->message[i], 1);

    log(adapterlog, "Sent: " + strResp + "   (" + to_string_fcount(msg->timestamp, 6) + ")");
}

bool adapter::send(message_t *msg)
{
    msg->timestamp = (uint64_t) (duration_cast<microseconds>(system_clock::now().time_since_epoch()).count() % 1000000);
    // printMessageContents("", msg);

	if ( adapterContext != nullptr )
		return adapterContext->send(msg);

    return false;
}
