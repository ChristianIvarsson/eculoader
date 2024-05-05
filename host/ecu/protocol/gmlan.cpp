#include "gmlan.h"

#include "../../tools/tools.h"

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <time.h>

using namespace msgsys;
using namespace logger;

static void translateError(const uint8_t *msg, uint32_t len)
{
    std::string rdRequest, rdResponse;

    if ( msg[0] != 0x7f || len < 1 )
    {
        log(gmlanlog, "translateError() - That's not how to use this function!");
        return;
    }

    if ( len < 3 )
    {
        log(gmlanlog, "Not enough data in response to translate error message");
        return;
    }

    switch ( msg[1] ) {
    case 0x04: rdRequest = "clearDiagnosticInformation"; break;
    case 0x10: rdRequest = "InitiateDiagnosticOperation"; break;
    case 0x1a: rdRequest = "ReadDataByIdentifier"; break;
    case 0x20: rdRequest = "returnToNormal"; break;
    case 0x23: rdRequest = "readMemoryByAddress"; break;
    case 0x27: rdRequest = "securityAccess"; break;
    case 0x28: rdRequest = "disableNormalCommunication"; break;
    case 0x34: rdRequest = "RequestDownload"; break;
    case 0x36: rdRequest = "transferData"; break;
    case 0x3b: rdRequest = "WriteDataByIdentifier"; break;
    case 0x3e: rdRequest = "testerPresent"; break;
    case 0xa2: rdRequest = "ReportProgrammedState"; break;
    case 0xa5: rdRequest = "programmingMode"; break;
    case 0xae: rdRequest = "DeviceControl"; break;
    default:   rdRequest = "Unknown req " + to_hex((volatile uint32_t)msg[1], 1); break;
    }

// TODO: These are copied from tcanflash. Some are known not to be correct or target specific
    switch ( msg[2] ) {
    case 0x10: rdResponse = "General reject"; break;
    case 0x11: rdResponse = "Service not supported"; break;
    case 0x12: rdResponse = "subFunction not supported - invalid format"; break;
    case 0x21: rdResponse = "Busy, repeat request"; break;
    case 0x22: rdResponse = "conditions not correct or request sequence error"; break;
    case 0x23: rdResponse = "Routine not completed or service in progress"; break;
    case 0x31: rdResponse = "Request out of range or session dropped"; break;
    case 0x33: rdResponse = "Security access denied"; break;
    case 0x35: rdResponse = "Invalid key supplied"; break;
    case 0x36: rdResponse = "Exceeded number of attempts to get security access"; break;
    case 0x37: rdResponse = "Required time delay not expired, you cannot gain security access at this moment"; break;
    case 0x40: rdResponse = "Download (PC -> ECU) not accepted"; break;
    case 0x41: rdResponse = "Improper download (PC -> ECU) type"; break;
    case 0x42: rdResponse = "Unable to download (PC -> ECU) to specified address"; break;
    case 0x43: rdResponse = "Unable to download (PC -> ECU) number of bytes requested"; break;
    case 0x50: rdResponse = "Upload (ECU -> PC) not accepted"; break;
    case 0x51: rdResponse = "Improper upload (ECU -> PC) type"; break;
    case 0x52: rdResponse = "Unable to upload (ECU -> PC) for specified address"; break;
    case 0x53: rdResponse = "Unable to upload (ECU -> PC) number of bytes requested"; break;
    case 0x71: rdResponse = "Transfer suspended"; break;
    case 0x72: rdResponse = "Transfer aborted"; break;
    case 0x74: rdResponse = "Illegal address in block transfer"; break;
    case 0x75: rdResponse = "Illegal byte count in block transfer"; break;
    case 0x76: rdResponse = "Illegal block transfer type"; break;
    case 0x77: rdResponse = "Block transfer data checksum error"; break;
    case 0x78: rdResponse = "Response pending"; break;
    case 0x79: rdResponse = "Incorrect byte count during block transfer"; break;

// Likely not to be here...
    case 0x80: rdResponse = "Service not supported in current diagnostics session"; break;
    case 0x81: rdResponse = "Scheduler full"; break;
    case 0x83: rdResponse = "Voltage out of range"; break;
    case 0x85: rdResponse = "General programming failure"; break;
    case 0x89: rdResponse = "Device type error"; break;
    case 0x99: rdResponse = "Ready for download"; break;
    case 0xE3: rdResponse = "DeviceControl Limits Exceeded"; break;
    default:   rdResponse = "Unknown error " + to_hex((volatile uint32_t)msg[2], 1); break;
    }

    log(gmlanlog, rdRequest + " threw \"" + rdResponse + "\"");
}

// While it _CAN_ check lengths you're better off only using this to interpret responses that are already known to be wrong
// If maxLen < minLen, minLen == exact len
static bool reqOk(const uint8_t *resp, uint8_t req, uint32_t minLen = 0, uint32_t maxLen = 0)
{
    if ( minLen < 1 )
    {
        minLen = 1;
    }

    if ( maxLen < minLen )
    {
        maxLen = minLen;
    }

    if ( resp == nullptr )
    {
        log(gmlanlog, "No response");
        return false;
    }

    uint32_t respLen = (uint32_t)(resp[0] << 8 | resp[1]);

    // First boundary check. This is just for translate error
    if ( respLen < 1 )
    {
        log(gmlanlog, "Response too short");
        return false;
    }

    if ( resp[2] == 0x7f )
    {
        translateError( &resp[2], respLen );
        return false;
    }

    if ( resp[2] != (req | 0x40) )
    {
        log(gmlanlog, "Target gave response to another request");
        return false;
    }

    if ( respLen < minLen || respLen > maxLen )
    {
        log(gmlanlog, "Response length not correct");
        return false;
    }

    return true;
}

void gmlan::setTesterID(uint64_t ID)
{
    this->testerID = ID;
}

void gmlan::setTargetID(uint64_t ID)
{
    this->targetID = ID;
}

uint64_t gmlan::getTesterID()
{
    return this->testerID;
}

uint64_t gmlan::getTargetID()
{
    return this->targetID;
}

#warning "This is only a half-assed implementation."

void gmlan::sendAck()
{
    if ( shortAck )
    {
        message_t sMsg = newMessage(this->testerID, 3);
        sMsg.message[0] = 0x30;
        sMsg.message[1] = 0x00;
        sMsg.message[2] = 0x00;
        send(&sMsg);
    }
    else
    {
        message_t sMsg = newMessage(this->testerID, 8);
        sMsg.message[0] = 0x30;
        sMsg.message[1] = 0x00;
        sMsg.message[2] = 0x00;
        send(&sMsg);
    }
}

/* USDT PCI encoding
Byte          __________0__________ ____1____ ____2____
Bits         | 7  6  5  4    3:0   |   7:0   |   7:0   |
Single       | 0  0  0  0     DL   |   N/A   |   N/A   |   (XX, .. ..)
First consec | 0  0  0  1   DL hi  |  DL lo  |   N/A   |   (1X, XX ..)
Consecutive  | 0  0  1  0     SN   |   N/A   |   N/A   |   (21, 22, 23 .. 29, 20, 21 ..)
Flow Control | 0  0  1  1     FS   |   BS    |  STmin  |   (3X, XX, XX)
Extrapolated:

Exd single   | 0  1  0  0   addr?  | ????:dl |   N/A   |   101 [41 92    (1a 79)   00000000]

FS:
0: CTS - Continue to send
1: WT  - Wait until next flow control
2: OVF - Overflow
3+ Not specified

STmin:
00-7f - Delay in ms
f1-f9 - Delay in us * 100 (f1 == 100 us, f9 == 900 us)
*/

// As seen above.. this thing needs some work
uint8_t *gmlan::sendRequest(uint8_t *req, uint32_t len, bool expectResponse)
{
    message_t  sMsg = newMessage(this->testerID, 8);
    message_t *rMsg;
    uint8_t    msgReq = req[0];

// TODO: Is it really 4095?
    if ( len > 4095 || len == 0 )
    {
        log(gmlanlog, "Request size out of range!");
        return nullptr;
    }

    if ( req == nullptr )
    {
        log(gmlanlog, "req[] is nullptr");
        return nullptr;
    }

    // Single frame
    if (len < 8)
    {
        sMsg.message[0] = len;

        memcpy(&sMsg.message[1], req, len);

        setupWaitMessage(this->targetID);

        if(!send(&sMsg))
        {
            log(gmlanlog, "Could not send!");
            return nullptr;
        }
    }
    else
    {
        sMsg.message[0] = (uint8_t)(0x10 | (len >> 8));
        sMsg.message[1] = (uint8_t)len;

        memcpy(&sMsg.message[2], req, 6);

        req += 6;
        len -= 6;

        setupWaitMessage(this->targetID);

        if(!send(&sMsg))
        {
            log(gmlanlog, "Could not send!");
            return nullptr;
        }


        // 0: CTS - Continue to send
        // 1: WT  - Wait until next flow control
        // 2: OVF - Overflow
        // 3+ Not specified
        rMsg = waitMessage( p2ct );

        // Wait frame
        while ( (rMsg->message[0] & 0xf3) == 0x31 )
        {
            rMsg = waitMessage( p2ct );
            // log(gmlanlog, "Transport wait..");
        }

        // Catch overflow and unknown FS flags
        if ((rMsg->message[0] & 0xf3) != 0x30)
        {
            log(gmlanlog, "Got no or corrupt ack");
            return nullptr;
        }

        // Delay between consecutive frames
        uint32_t ST = rMsg->message[2];

        if (ST < 0x80)
        {
            ST *= 1000; // Milliseconds
        }
        else if (ST > 0xf0 && ST < 0xfa)
        {
            ST = (ST&15) * 100; // microsec * 100
        }
        else
        {
            log(gmlanlog, "Received funny flow control frame");
            ST = 0;
        }

        // Consecutive sent frames before forced to wait for a new flow control
        uint32_t BS = rMsg->message[1];

        if (BS > 0)
        {
            log(gmlanlog, "BS is set. Implement!");
        }

        // Message stepper
        uint8_t stp = 0x21;

        while ( len > 0 )
        {
            size_t thisCount = (len > 7) ? 7 : len;

            memcpy(&sMsg.message[1], req, thisCount);

            req += thisCount;
            len -= thisCount;

            sMsg.message[0] = stp++;
            stp &= 0x2f;

            if ( len == 0 )
                setupWaitMessage(this->targetID);

            if(!send(&sMsg))
            {
                log(gmlanlog, "Could not send!");
                return nullptr;
            }

            if ( len > 0 )
                timer::sleepMicro( ST );
        }
    }

    if ( !expectResponse )
    {
        return nullptr;
    }

// TODO: This is another thing that'll need work to get extended addressing working

    // auto oldTime = std::chrono::system_clock::now();

busyWait:

    rMsg = waitMessage( p2ct );

    if (rMsg->id != this->targetID)
    {
        log(gmlanlog, "No response");
        // Past: This maaaay not be a showstopper. if the previous response was a 7f 78.
        // Future: Why would it not?!
        // return recdat;
        return nullptr;
    }

    // Single message
    if (rMsg->message[0] > 0 && rMsg->message[0] < 8)
    {
        // Busy, wait for a new update
        if (rMsg->message[0] >  0x02 && rMsg->message[0] < 8 &&
            rMsg->message[1] == 0x7f && 
            rMsg->message[2] == msgReq &&
            rMsg->message[3] == 0x78)
        {
#warning "CIM takes a VERY long time"
            // if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - oldTime).count() < busyWaitTimeout)
            {
                goto busyWait;
            }
        }

        uint8_t *recdat = new uint8_t[ rMsg->message[0] + 2 ];

        // First two bytes declare a u16 len
        recdat[0] = 0;

        memcpy(&recdat[ 1 ], rMsg->message, (size_t)(rMsg->message[0] + 1));

        return recdat;
    }

    if ((rMsg->message[0] & 0xf0) == 0x10)
    {
        // bytes, total to receive including the header
        size_t toRec = ((rMsg->message[0] << 8 | rMsg->message[1]) & 0xfff) + 2;

        if (toRec <= 8)
        {
            log(gmlanlog, "Target is drunk. Received undersized frame");
            return nullptr;
        }

        uint8_t *recdat = new uint8_t[ toRec ];
        uint8_t  stp = 0x21;
        uint32_t bufPtr = 0;

        memcpy(&recdat[ bufPtr ], rMsg->message, 8);

        // Remove size of header (2) and number of bytes stored (6)
        bufPtr += 8;
        toRec -= 8;

        // Strip high nible
        recdat[0] &= 0x0f;

        setupWaitMessage(this->targetID);

        sendAck();

        while ( toRec > 0 )
        {
            rMsg = waitMessage( p2ct );

            if ( rMsg->message[0] != stp ) {
                log(gmlanlog, "Received step out of sync");
                delete[] recdat;
                return nullptr;
            }

            size_t thisCount = (toRec > 7) ? 7 : toRec;

            memcpy(&recdat[ bufPtr ], &rMsg->message[1], thisCount);

            bufPtr += thisCount;
            toRec  -= thisCount;

            stp = (stp+1)&0x2f;
        }

        return recdat;
    }
    else
    {
        log(gmlanlog, "Unknown PCI");
    }

    return nullptr;
}

// 04
bool gmlan::clearDiagnosticInformation()
{
    uint8_t *ret, buf[8] = { 0x04 };

    if ((ret = sendRequest(buf, 1)) == nullptr)
        return false;

    if ((uint32_t)(ret[0] << 8 | ret[1]) < 1 || ret[2] != 0x44)
    {
        delete[] ret;
        return false;
    }

    delete[] ret;
    return true;
}

// 10
bool gmlan::InitiateDiagnosticOperation(enDiagOp mode)
{
    uint8_t *ret, buf[ 2 ] = { 0x10, (uint8_t)mode };

    if ((ret = sendRequest(buf, 2)) == nullptr)
        return false;

    if ((uint32_t)(ret[0] << 8 | ret[1]) < 1 || ret[2] != 0x50)
    {
        delete[] ret;
        return false;
    }

    delete[] ret;
    return true;
}

// 1a
uint8_t *gmlan::ReadDataByIdentifier(uint8_t id)
{
    uint8_t *ret, buf[ 2 ] = { 0x1a, id };

    if ((ret = sendRequest(buf, 2)) == nullptr)
        return nullptr;

    uint16_t retLen = (ret[0] << 8 | ret[1]) + 2;

#warning "Clean me!"
    if (retLen < 5 ||  ret[2] != 0x5a || ret[3] != id)
    {
        delete[] ret;
        return nullptr;
    }

    retLen-=4;
    ret[0] = (retLen>>8);
    ret[1] = (retLen);
    retLen += 2;

    for (uint32_t i = 2; i < retLen; i++)
        ret[i] = ret[i + 2];

    ret[retLen] = 0;
    ret[retLen+1] = 0;

    return ret;
}

// 20
bool gmlan::returnToNormal()
{
    uint8_t *ret, buf[8] = { 0x20 };

    if ((ret = sendRequest(buf, 1)) == nullptr)
        return false;

    if ((uint32_t)(ret[0] << 8 | ret[1]) < 1 || ret[2] != 0x60)
    {
        delete[] ret;
        return false;
    }

    delete[] ret;
    return true;
}

// TODO: Implement retry

// 23
// Read from a variable size address and length
uint8_t *gmlan::readMemoryByAddress(
                        uint32_t  address,
                        uint32_t  length,
                        enGmMemSz addressSize,
                        enGmMemSz lengthSize,
                        uint32_t  blockSize)
{
    uint8_t *dataBuf, *ret, buf[ 16 ] = { 0x23 };
    uint32_t dataLeft = length;
    uint32_t dataPtr = 0;

    if ( length == 0 || blockSize == 0 )
    {
        log(gmlanlog, "Length or blocksize out of range");
        return nullptr;
    }

    dataBuf = new uint8_t[ length ];

    while ( dataLeft > 0 )
    {
        uint32_t thisLen = (dataLeft > blockSize) ? blockSize : dataLeft;
        uint32_t addrLen = 0;
        uint32_t sizeLen = 0;

        switch ( addressSize ) {
        case gmSize32: buf[1 + addrLen++] = (uint8_t)(address >> 24);
        case gmSize24: buf[1 + addrLen++] = (uint8_t)(address >> 16);
        case gmSize16: buf[1 + addrLen++] = (uint8_t)(address >> 8);
        case gmSize8:  buf[1 + addrLen++] = (uint8_t)address;
        }

        switch ( lengthSize ) {
        case gmSize32: buf[1 + addrLen + sizeLen++] = (uint8_t)(thisLen >> 24);
        case gmSize24: buf[1 + addrLen + sizeLen++] = (uint8_t)(thisLen >> 16);
        case gmSize16: buf[1 + addrLen + sizeLen++] = (uint8_t)(thisLen >> 8);
        case gmSize8:  buf[1 + addrLen + sizeLen++] = (uint8_t)thisLen;
        }

        if ((ret = sendRequest(buf, addrLen + sizeLen + 1)) == nullptr)
        {
            log(gmlanlog, "No returned data to request readMemoryByAddress");
            delete[] dataBuf;
            return nullptr;
        }

        if ( (uint32_t)(ret[0] << 8 | ret[1]) != (thisLen + addrLen + 1) )
        {
            log(gmlanlog, "Target did not respond with the expected length");
            delete[] ret;
            delete[] dataBuf;
            return nullptr;
        }

        if ( ret[2] != (0x23 | 0x40) )
        {
            log(gmlanlog, "Target did not respond as expected to request");
            delete[] ret;
            delete[] dataBuf;
            return nullptr;
        }

        uint32_t retAddr = 0;

        switch ( addressSize ) {
        case gmSize32: retAddr = (ret[3] << 24 | ret[4] << 16 | ret[5] << 8 | ret[6]); break;
        case gmSize24: retAddr = (               ret[3] << 16 | ret[4] << 8 | ret[5]); break;
        case gmSize16: retAddr = (                              ret[3] << 8 | ret[4]); break;
        case gmSize8:  retAddr = (                                            ret[3]); break;
        }

        if ( retAddr != address )
        {
            log(gmlanlog, "Target returned another address than requested");
            delete[] ret;
            delete[] dataBuf;
            return nullptr;
        }


        // LH/LL  - ( 2 ) Length high/low
        // 63     - ( 1 ) Request (23) OK
        // AA     - ( 1+) Response to address. Can be 1 to 4 bytes but realistically 3 to 4
        // DD     - ( 1+) Data  ( "thisLen" declares length )

        // LH LL 63 AA.. DD..
        memcpy(&dataBuf[ dataPtr ], &ret[ addrLen + 3 ], thisLen);

        delete[] ret;

        dataLeft -= thisLen;
        address  += thisLen;
        dataPtr  += thisLen;
    }

    return dataBuf;
}

// 27
bool gmlan::securityAccess(uint32_t lev, keyFunc_t & keyFunc)
{
    uint8_t *ret, buf[ 8 ] = { 0x27, (uint8_t)lev };

    log(gmlanlog, "Requesting security access");

    if ( (ret = sendRequest(buf, 2)) == nullptr )
    {
        log(gmlanlog, "No response to securityAccess");
        return false;
    }

    // Check if responding to our request, status and length
    if ( !reqOk(ret, 0x27, 2, 4095) )
    {
        delete[] ret;
        return false;
    }

    if ( ret[3] != (uint8_t)lev )
    {
        log(gmlanlog, "securityAccess did not respond to the correct level");

        delete[] ret;
        return false;
    }


    uint32_t retLen = (uint32_t)(ret[0] << 8 | ret[1]);

    // Unlikely scenario - Received OK response with nothing else than level
    if ( retLen == 2 )
    {
        log(gmlanlog, "Received OK with no additional data. Suspect..");

        delete[] ret;
        return true;
    }

    // Translate to seed length
    retLen -= 2;
    bool respZero = true;

    for (uint32_t i = 0; i < retLen; i++)
    {
        if ( ret[4 + i] != 0 )
        {
            respZero = false;
            break;
        }
    }

    if ( respZero )
    {
        log(gmlanlog, "Security access already granted");

        delete[] ret;
        return true;
    }

    // Convert seed to string
    std::string seedStr;
    for (uint32_t i = 0; i < retLen; i++)
        seedStr += to_hex((volatile uint32_t)ret[i + 4], 1);

    // Call key calc func
    if ( !keyFunc(&ret[4], retLen) )
    {
        log(gmlanlog, "securityAccess keygen returned a fault");

        delete[] ret;
        return false;
    }

    // Convert key to string
    std::string keyStr;
    for (uint32_t i = 0; i < retLen; i++)
        keyStr += to_hex((volatile uint32_t)ret[i + 4], 1);

    log(gmlanlog, "Seed " + seedStr);
    log(gmlanlog, "Key  " + keyStr);

    uint8_t *req2 = new uint8_t[ retLen + 2 ];

    req2[ 0 ] = 0x27;
    req2[ 1 ] = (uint8_t)lev + 1;

    memcpy(&req2[2], &ret[4], retLen);

    // Old returned buffer is no longer of any use
    delete[] ret;






    if ( (ret = sendRequest(req2, retLen + 2)) == nullptr )
    {
        log(gmlanlog, "No response to securityAccess key");

        delete[] req2;
        return false;
    }

    delete[] req2;


    // Check if responding to our request, status and length
    if ( !reqOk(ret, 0x27, 2, 4095) )
    {
        delete[] ret;
        return false;
    }

    // Expect 27 | 40 (OK)
    if ( ret[2] != 0x67 )
    {
        // We already know it'll fail so ignore response of this.
        reqOk(ret, 0x27);

        delete[] ret;
        return false;
    }

    // Expect level + 1 in the response
    if ( ret[3] != (uint8_t)(lev + 1) )
    {
        log(gmlanlog, "securityAccess did not respond to the correct key level");

        delete[] ret;
        return false;
    }


    delete[] ret;
    return true;
}


// Weird responses will not be interpreted as they should
#warning "This is not compliant"

// 28
bool gmlan::disableNormalCommunication(int exdAddr)
{
    message_t sMsg = newMessage(this->testerID, 8);

    if (exdAddr >= 0 && exdAddr <= 0xff)
    {
        sMsg.id = 0x101;
        sMsg.message[0] = (uint8_t)exdAddr;
        sMsg.message[1] = 0x01;
        sMsg.message[2] = 0x28;
    }
    else
    {
        sMsg.message[0] = 0x01;
        sMsg.message[1] = 0x28;
    }

    setupWaitMessage(this->targetID);

    if(!send(&sMsg))
    {
        log(gmlanlog, "Could not send!");
        return false;
    }

    message_t *rMsg = waitMessage( p2ct );
    return (
        rMsg->message[0] < 8 &&
        rMsg->message[0] > 0 &&
        rMsg->message[1] == 0x68) ? true : false;
}

// 28
bool gmlan::disableNormalCommunicationNoResp(int exdAddr)
{
    message_t sMsg = newMessage(this->testerID, 8);

    if (exdAddr >= 0 && exdAddr <= 0xff)
    {
        sMsg.id = 0x101;
        sMsg.message[0] = (uint8_t)exdAddr;
        sMsg.message[1] = 0x01;
        sMsg.message[2] = 0x28;
    }
    else
    {
        sMsg.message[0] = 0x01;
        sMsg.message[1] = 0x28;
    }

    if(!send(&sMsg))
    {
        log(gmlanlog, "Could not send!");
        return false;
    }

    return true;
}

// 34
bool gmlan::RequestDownload(
                    uint32_t  size,
                    enGmMemSz lengthSize,
                    uint32_t  fmt
                    )
{
    uint8_t *ret, buf[ 8 ] = { 0x34, (uint8_t)fmt };
    uint32_t sizeLen = 0;

    switch ( lengthSize ) {
    case gmSize32: buf[2 + sizeLen++] = (uint8_t)(size >> 24);
    case gmSize24: buf[2 + sizeLen++] = (uint8_t)(size >> 16);
    case gmSize16: buf[2 + sizeLen++] = (uint8_t)(size >> 8);
    case gmSize8:  buf[2 + sizeLen++] = (uint8_t)size;
    }

    if ((ret = sendRequest(buf, sizeLen + 2)) == nullptr)
        return false;

    if ((uint32_t)(ret[0] << 8 | ret[1]) < 1 || ret[2] != 0x74)
    {
        delete[] ret;
        return false;
    }

    delete[] ret;
    return true;
}

// 36
bool gmlan::transferData(
                    const uint8_t *data,
                    uint32_t       address,
                    uint32_t       length,
                    bool           execute,
                    enGmMemSz      addressSize,
                    uint32_t       blockSize)
{
    uint8_t *ret, buf[ 256 + 8 ] = { 0x36 };
    uint32_t remain = length;
    uint32_t baseAd = address;

#warning "needs work"

    // Prevent overflow of message buffer
    if ( blockSize > 256 )
    {
        log(gmlanlog, "Nudging block size to prevent overflow");
        blockSize = 256;
    }

    progress( 0 );

    if ( length == 0 || blockSize == 0 )
    {
        log(gmlanlog, "Length or blocksize out of range");
        return false;
    }

    while ( remain > 0 )
    {
        uint32_t thisLen = (remain > blockSize) ? blockSize : remain;
        uint32_t addrLen = 0;

        switch ( addressSize ) {
        case gmSize32: buf[2 + addrLen++] = (uint8_t)(address >> 24);
        case gmSize24: buf[2 + addrLen++] = (uint8_t)(address >> 16);
        case gmSize16: buf[2 + addrLen++] = (uint8_t)(address >> 8);
        case gmSize8:  buf[2 + addrLen++] = (uint8_t)address;
        }

        memcpy(&buf[ addrLen + 2 ], data, thisLen);

        if ((ret = sendRequest(buf, thisLen + addrLen + 2)) == nullptr)
        {
            log(gmlanlog, "Transfer failed (no answer)");
            return false;
        }

        // 00 xx 76
        if ((uint32_t)(ret[0] << 8 | ret[1])  < 1 || ret[2] != 0x76)
        {
            log(gmlanlog, "Received fail");
            delete[] ret;
            return false;
        }
        delete[] ret;

        data    += thisLen;
        remain  -= thisLen;
        address += thisLen;

        progress( (uint32_t)(float) ( 100.0 * (length-remain)) / length );
    }

    if ( execute == true )
    {
        uint32_t addrLen = 0;

        switch ( addressSize ) {
        case gmSize32: buf[2 + addrLen++] = (uint8_t)(baseAd >> 24);
        case gmSize24: buf[2 + addrLen++] = (uint8_t)(baseAd >> 16);
        case gmSize16: buf[2 + addrLen++] = (uint8_t)(baseAd >> 8);
        case gmSize8:  buf[2 + addrLen++] = (uint8_t)baseAd;
        }

        // Set execute flag
        buf[1] = 0x80;

        // Special case. It's expected NOT to respond when requesting start of code
        if ((ret = sendRequest(buf, addrLen + 2)) == nullptr)
        {
            return true;
        }

        // 00 xx 76
        if ((uint32_t)(ret[0] << 8 | ret[1])  < 1 || ret[2] != 0x76)
        {
            log(gmlanlog, "Received fail");
            delete[] ret;
            return false;
        }

        delete[] ret;
    }

    return true;
}

// 3b
bool gmlan::WriteDataByIdentifier(const uint8_t *dat, uint8_t id, uint32_t len)
{
#warning "Clean me!"
    uint8_t *ret, buf[256 + 2] = { 0x3b, id };

    if (len > 256)
        return false;

    if (dat)
        memcpy(&buf[2], dat, len);
    else
        memset(&buf[2], 0  , len);

    if ((ret = sendRequest(buf, len + 2)) == nullptr)
        return false;

    // 20202020202020202020
    // 00 02 7b __ xx xx
    if ((uint32_t)(ret[0] << 8 | ret[1]) < 2 || ret[2] != 0x7b || ret[3] != id)
    {
        delete[] ret;
        return false;
    }

    // log(gmlanlog, "Succ??");

    delete[] ret;
    return true;
}


#warning "Mend"

// 3e
bool gmlan::testerPresent(int exdAddr)
{
    message_t sMsg = newMessage(this->testerID, 8);

    if (exdAddr >= 0 && exdAddr <= 0xff)
    {
        sMsg.id = 0x101;

        sMsg.message[0] = (uint8_t)exdAddr;
        sMsg.message[1] = 0x01;
        sMsg.message[2] = 0x3e;

        if(!send(&sMsg))
        {
            log(gmlanlog, "Could not send!");
            return false;
        }

        return true;
    }
    else
    {
        sMsg.message[0] = 0x01;
        sMsg.message[1] = 0x3e;

        setupWaitMessage(this->targetID);

        if(!send(&sMsg))
        {
            log(gmlanlog, "Could not send!");
            return false;
        }
    }

    message_t *rMsg = waitMessage( p2ct );
    return (rMsg->message[0] < 8 && rMsg->message[1] == 0x7e) ? true : false;
}

/* a2
00: fully programmed
01: no op s/w or cal data
02: op s/w present, cal data missing
50: General Memory Fault
51: RAM Memory Fault
52: NVRAM Memory Fault
53: Boot Memory Failure
54: Flash Memory Failure
55: EEPROM Memory Failure */
bool gmlan::ReportProgrammedState()
{
    uint8_t *ret, buf[8] = { 0xa2 };

    if ((ret = sendRequest(buf, 1)) == nullptr)
    {
        log(gmlanlog, "Could not retrieve programmed state");
        return false;
    }

    // 00 02 e2 <st>
    if ((uint32_t)(ret[0] << 8 | ret[1])  < 2 || ret[2] != 0xe2)
    {
        log(gmlanlog, "Could not retrieve programmed state");
        delete[] ret;
        return false;
    }

    std::string state;
    switch (ret[3]) {
    case 0x00: state = "Fully programmed"; break;
    case 0x01: state = "No op s/w or cal data"; break;
    case 0x02: state = "Op s/w present, cal data missing"; break;
    case 0x50: state = "General Memory Fault"; break;
    case 0x51: state = "RAM Memory Fault"; break;
    case 0x52: state = "NVRAM Memory Fault"; break;
    case 0x53: state = "Boot Memory Failure"; break;
    case 0x54: state = "Flash Memory Failure"; break;
    case 0x55: state = "EEPROM Memory Failure"; break;
    default:
        state = "Unknown response: 0x" + to_hex((volatile uint32_t)ret[3], 1);
        break;
    }

    log(gmlanlog, "Target state: " + state);

    delete[] ret;
    return true;
}



// a5
bool gmlan::programmingMode(enProgOp lev, bool expectResponse)
{
    uint8_t *ret, buf[ 4 ] = { 0xa5, (uint8_t)lev };

    // Only 3 is expected not to respond according to spec
    if ( !expectResponse )
    {
        if ((ret = sendRequest(buf, 2, false)) == nullptr)
        {
            return true;
        }

        // 00 01 e5 ..?
        if ((uint32_t)(ret[0] << 8 | ret[1]) < 1 || ret[2] != 0xE5)
        {
            log(gmlanlog, "Retrieved unexpected response to programmingMode");

            delete[] ret;
            return false;
        }

        log(gmlanlog, "Target gave a response where one wasn't expected");

        delete[] ret;
        return true;
    }
    else
    {
        if ((ret = sendRequest(buf, 2)) == nullptr)
        {
            return true;
        }

        // 00 01 e5 ..?
        if ((uint32_t)(ret[0] << 8 | ret[1]) < 1 || ret[2] != 0xE5)
        {
            log(gmlanlog, "Retrieved unexpected response to programmingMode");
            delete[] ret;
            return false;
        }

        delete[] ret;
        return true;
    }
}

// ae
bool gmlan::DeviceControl(const uint8_t *dat, uint8_t id, uint8_t len)
{
    uint8_t *ret, buf[256 + 2] = { 0xae, id };

    if (len > 256)
        return false;

    if (dat)
        memcpy(&buf[2], dat, len);
    else
        memset(&buf[2], 0  , len);
    
    if ((ret = sendRequest(buf, len + 2)) == nullptr)
        return false;

    if ((uint32_t)(ret[0] << 8 | ret[1]) < 1 || ret[2] != 0xee)
    {
        delete[] ret;
        return false;
    }

    delete[] ret;
    return true;
}
