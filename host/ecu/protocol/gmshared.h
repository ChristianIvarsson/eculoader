#ifndef __GMSHARED_H__
#define __GMSHARED_H__

// KWP2000 / GMLAN shared code

#include "../../tools/tools.h"
#include "../../adapter/adapter.h"

#include <cstring>

typedef struct {
    uint32_t PMA;  // Address
    uint32_t NOB;  // Size
} range_t;

typedef struct {
    uint32_t DCID;       // Data Compatibility identifier
    uint32_t NOAR;       // Number Of Address Regions
    range_t  range[16];  // Start address, size
} module_t;

#define msgLen(msg) ((uint32_t)msg[0] << 8 | msg[1])

class gmshared : public virtual adapter
{
public:

    // Used by readMemoryByAddress, requestDownload, transferData etc
    enum enGmMemSz : uint32_t
    {
        gmSize8  = 1,
        gmSize16 = 2,
        gmSize24 = 3,
        gmSize32 = 4
    };

    virtual uint8_t *sendRequest(uint8_t*, uint32_t, bool expectResponse = true) = 0;

    virtual void translateError(const uint8_t *msg, uint32_t len) = 0;

    // While it _CAN_ check lengths you're better off only using this to interpret responses that are already known to be wrong
    // If maxLen < minLen, minLen == exact len
    bool reqOk(const uint8_t *resp, uint8_t req, uint32_t minLen = 0, uint32_t maxLen = 0)
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
            logger::log(logger::gmsharedlog, "No response");
            return false;
        }

        uint32_t respLen = msgLen( resp );

        // First boundary check. This is just for translate error
        if ( respLen < 1 )
        {
            logger::log(logger::gmsharedlog, "Response too short");
            return false;
        }

        if ( resp[2] == 0x7f )
        {
            // Expect 7f, req, reason
            if ( respLen < 3 )
            {
                logger::log(logger::gmsharedlog, "Returned error message but it's too short to interpret");
                return false;
            }

            translateError( &resp[2], respLen );

            return false;
        }

        if ( resp[2] != (req | 0x40) )
        {
            logger::log(logger::gmsharedlog, "Target gave response to another request");
            return false;
        }

        if ( respLen < minLen || respLen > maxLen )
        {
            logger::log(logger::gmsharedlog, "Response length not correct");
            return false;
        }

        return true;
    }

    // Req: 1a
    // aka kwp2k readDataByLocalIdentifier
    uint8_t *ReadDataByIdentifier(uint8_t id)
    {
        uint8_t *ret, buf[ 2 ] = { 0x1a, id };

        if ((ret = sendRequest(buf, 2)) == nullptr)
            return nullptr;

        uint32_t retLen = msgLen(ret) + 2;

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

    // Req: 27
    // length is used both ways. securityAccess sets seed length and key function can optionally change it if the key is of another length
    typedef bool keyFunc_t(const uint8_t *seed, uint8_t *&key, uint32_t & length);
    bool securityAccess(uint32_t lev, const keyFunc_t & keyFunc)
    {
        uint8_t *ret, buf[ 8 ] = { 0x27, (uint8_t)lev };

        logger::log(logger::gmsharedlog, "Requesting security access");

        if ( (ret = sendRequest(buf, 2)) == nullptr )
        {
            logger::log(logger::gmsharedlog, "No response to securityAccess");
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
            logger::log(logger::gmsharedlog, "securityAccess did not respond to the correct level");

            delete[] ret;
            return false;
        }


        uint32_t retLen = msgLen(ret);

        // Unlikely scenario - Received OK response with nothing else than level
        if ( retLen == 2 )
        {
            logger::log(logger::gmsharedlog, "Received OK with no additional data. Suspect..");

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
            logger::log(logger::gmsharedlog, "Security access already granted");

            delete[] ret;
            return true;
        }

        // Convert seed to string
        std::string seedStr;
        for (uint32_t i = 0; i < retLen; i++)
            seedStr += to_hex((volatile uint32_t)ret[i + 4], 1);

        uint8_t *key = nullptr;

        // Call key calc func
        // Note: retLen is of a ref type so it could change
        if ( !keyFunc(&ret[4], key, retLen) )
        {
            logger::log(logger::gmsharedlog, "securityAccess keygen returned a fault");

            if ( key != nullptr )
                delete[] key;

            delete[] ret;
            return false;
        }
        else if ( key == nullptr )
        {
            logger::log(logger::gmsharedlog, "securityAccess keygen didn't return a key array");

            delete[] ret;
            return false;
        }
        else if ( retLen == 0 )
        {
            logger::log(logger::gmsharedlog, "securityAccess keygen returned zero-len");
            delete[] key;
            delete[] ret;
            return false;
        }

        // Convert key to string
        std::string keyStr;
        for (uint32_t i = 0; i < retLen; i++)
            keyStr += to_hex((volatile uint32_t)key[i], 1);

        logger::log(logger::gmsharedlog, "Seed " + seedStr);
        logger::log(logger::gmsharedlog, "Key  " + keyStr);

        uint8_t *req2 = new uint8_t[ retLen + 2 ];

        req2[ 0 ] = 0x27;
        req2[ 1 ] = (uint8_t)lev + 1;

        memcpy(&req2[2], key, retLen);

        // Old returned buffer and key are no longer of use
        delete[] ret;
        delete[] key;

        if ( (ret = sendRequest(req2, retLen + 2)) == nullptr )
        {
            logger::log(logger::gmsharedlog, "No response to securityAccess key");

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

        // Expect level + 1 in the response
        if ( ret[3] != (uint8_t)(lev + 1) )
        {
            logger::log(logger::gmsharedlog, "securityAccess did not respond to the correct key level");

            delete[] ret;
            return false;
        }

        delete[] ret;
        return true;
    }

#warning "Clean me!"
    // Req: 3b
    // aka kwp2k writeDataByLocalIdentifier
    bool WriteDataByIdentifier(const uint8_t *dat, uint8_t id, uint32_t len)
    {
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
        if (msgLen(ret) < 2 || ret[2] != 0x7b || ret[3] != id)
        {
            delete[] ret;
            return false;
        }

        delete[] ret;
        return true;
    }
};

#endif
