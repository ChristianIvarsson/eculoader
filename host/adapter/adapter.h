#ifndef ADAPTER_H
#define ADAPTER_H

#include "message.h"

#include <cstdio>
#include <string>
#include <list>

class adapterDesc;

#warning "Disassemble some bins and determine actual SP"
// T5 is thought to be fetched from an actual bin - Verify


// 82c200 / sja1000 ( Kvaser, Lawicel canusb )
// ( btr0 << 8 | btr1 )
// Devices at 16 MHz
enum c200btrs : uint32_t
{
    sja25k     = 0x0f4d,    // ( SP 75.00%, SNW 1             )
    sja33k     = 0x0b4d,    // ( SP 75.00%, SJW 1             )   < Trionic 8, I-bus at 33.333..  >
    sja47k     = 0x074e,    // ( SP 76.19%, SJW 1             )   < Trionic 7, I-bus at 47.619..  >
    sja50k     = 0x074d,    // ( SP 75.00%, SNW 1             )
    sja75k     = 0x453c,    // ( SP 77.78%, SJW 2, Off -1.23% )   !! WARN !!
    sja100k    = 0x034d,    // ( SP 75.00%, SNW 1             )
    sja125k    = 0x033a,    // ( SP 75.00%, SNW 1             )
    sja150k    = 0x423c,    // ( SP 77.78%, SJW 2, Off -1.23% )   !! WARN !!
    sja175k    = 0x415f,    // ( SP 73.91%, SJW 2, Off -0.62% )   !! WARN !!
    sja200k    = 0x014d,    // ( SP 75.00%, SJW 1             )
    sja250k    = 0x013a,    // ( SP 75.00%, SJW 1             )
    sja300k    = 0x4215,    // ( SP 77.78%, SJW 2, Off -1.23% )   !! WARN !!
    sja400k    = 0x004d,    // ( SP 75.00%, SJW 1             )
    sja500k    = 0x003a,    // ( SP 75.00%, SJW 1             )   < Standard 500 kbit/s           >
    sja615k    = 0x4037,    // ( SP 69.23%, SJW 2             )   < Trionic 5, P-bus at 615.384.. >
    sja800k    = 0x0025,    // ( SP 70.00%, SJW 1             )
    sja1000k   = 0x0014,    // ( SP 75.00%, SJW 1             )
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
