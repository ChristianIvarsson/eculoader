#ifndef ADAPTER_H
#define ADAPTER_H

#include "message.h"

#include <cstdio>
#include <string>
#include <list>

class adapterDesc;

// 82c200 / sja1000 ( Kvaser, Lawicel canusb )
// ( btr0 << 8 | btr1 )
enum c200btrs : uint32_t
{
    sja33k     = 0x0b2f,    // ( SP 85.00%, SJW 1, Off  0.00% )   < Trionic 8, I-bus at 33.333.. >
    sja47k     = 0x0b1a,    // ( SP 85.71%, SJW 1, Off  0.00% )   < Trionic 7, I-bus at 47.619.. >
    sja200k    = 0x012f,    // ( SP 85.00%, SJW 1, Off  0.00% )
    sja300k    = 0x4215,    // ( SP 77.78%, SJW 2, Off -1.23% )   !! WARN !!
    sja400k    = 0x002f,    // ( SP 85.00%, SJW 1, Off  0.00% )
    sja615k    = 0x0019,    // ( SP 84.62%, SJW 1, Off  0.00% )   < Trionic 5, P-bus at 615.384.. >
};

enum adaptertypes
{
    adapterCanUsb, // Lawicel CANUSB
    adapterKvaser, // Various Kvaser devices
    adapterUnknown,
};

enum adapterPorts
{
    portCAN,
    portKline
};

enum enBitrate
{
    btr200k,
    btr300k,
    btr400k,
    btr500k,
    btr615k,
};

typedef struct {
    std::string name;
    std::list<uint32_t> canIDs;
    enBitrate bitrate;
} channelData;


// Adapter class "template". Used "internally"
class adapter_t
{
public:
    // explicit adapter_t();
    virtual ~adapter_t() = 0;
    virtual std::list <std::string> adapterList() = 0;
    virtual bool open(channelData&) = 0;
    virtual bool close() = 0;
    virtual bool send(msgsys::message_t*) = 0;
};

class adapter
{
public:
    explicit adapter();
    ~adapter();
	std::list <std::string> listAdapters(adaptertypes);
	bool open(channelData&);
	bool close();
	bool send(msgsys::message_t*);
protected:

private:
	bool setAdapter(adaptertypes &);
    adapter_t *adapterContext;
};


// Unique descriptor of an adapter
class adapterDesc
{
public:
	adaptertypes type;   // 
	std::string  name;   // Adapter name. ie USBcan II, canusb, combiadapter etc
	std::string  port;   // If an adapter has several ports of the same type, this one describes which one to use
	std::string  serial; // Unique identifier
};


#endif
