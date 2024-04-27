
#include "acdelco.h"

#include "../../../adapter/message.h"

#include "../../../tools/tools.h"


#include <cstring>

using namespace logger;
using namespace timer;
using namespace std;
using namespace msgsys;

// 0x40004000
// 0x40002000
#define LOADER_BASE   ( 0x40004000 )

#define BAM_BLOB    "./loaderblobs/mpc5566_loader_bam.bin"
#define LOADER_BLOB "../mpc5566/out/loader.bin"


enum mpc5566Mode : uint32_t
{
    modeBAM   = 0, // Defunct. It must be compiled in this mode
    modeE39   = 1,
    modeE78   = 2,
    modeE39A  = 3,
};

e39::e39()
{
}

e39::~e39()
{
}

void e39::configProtocol()
{
    setTesterID( 0x7e0 );
    setTargetID( 0x7e8 );
}


uint64_t lazySWAP(uint64_t data)
{
    uint64_t retval = 0;
    for (int i = 0; i < 8; i++)
    {
        retval <<= 8;
        retval |= (data & 0xff);
        data >>= 8;
    }
    return retval;
}

// To be moved somewhere else. It's only here while in the thrash branch
// uint64_t bamKEY = 0x7BC10CBD55EBB2DA; // E78
// uint64_t bamKEY = 0xFEEDFACECAFEBEEF; // Public
uint64_t bamKEY = 0xFD13031FFB1D0521; // 881155AA
//                   0x________________; // Since the editor is ***...

static uint64_t rU64Mgs(message_t *msg)
{
    uint64_t retval = 0;
    for (uint32_t i = 0; i < 8; i++)
    {
        retval <<= 8;
        retval |= msg->message[7 - i];
    }
    return retval;
}

static void wU64Mgs(message_t *msg, uint64_t dat)
{
    for (uint32_t i = 0; i < 8; i++)
    {
        msg->message[i] = (uint8_t)dat;
        dat >>= 8;
    }
}

bool e39::initSessionBAM()
{
    message_t sMsg = newMessage(0x11, 8);

    uint32_t tries = 10;

    wU64Mgs(&sMsg, lazySWAP(bamKEY));
    uint64_t retVal = ~lazySWAP(bamKEY);

    setupWaitMessage(1);
    do
    {
        if (!send(&sMsg))
        {
            log(e39log, "Could not send!");
            return false;
        }

        message_t *rMsg = waitMessage(250);
        retVal = rU64Mgs(rMsg);

        // tries--;
        if (tries == 0)
        {
            log(e39log, "Could not start bam!");
            return false;
        }
    } while (retVal != lazySWAP(bamKEY));

    log(e39log, "Key accepted");

    fileManager fm;
    fileHandle *file = fm.open( BAM_BLOB );

    if (!file || file->size < (6 * 1024))
    {
        log(e39log, "No file or too small");
        return false;
    }

    uint32_t alignedSize = (file->size + 7) & ~7;
    uint8_t *tmpBuf = new uint8_t[alignedSize];
    if (tmpBuf == nullptr)
    {
        log(e39log, "Could not allocate buffer");
        return false;
    }

    memcpy(tmpBuf, file->data, file->size);

    uint64_t cmd = lazySWAP((uint64_t)LOADER_BASE << 32 | alignedSize);
    sMsg = newMessage(0x12, 8);

    wU64Mgs(&sMsg, cmd);
    setupWaitMessage(2);
    if (!send(&sMsg))
    {
        log(e39log, "Could not send!");
        return false;
    }

    // 0xFFFFF00

    message_t *rMsg = waitMessage(50);
    retVal = rU64Mgs(rMsg);

    uint32_t bufPntr = 0;
    if (retVal == cmd)
    {
        log(e39log, "Address and size accepted");

        sMsg = newMessage(0x13, 8);

        while (alignedSize > 0)
        {
            cmd = 0;
            for (int e = 0; e < 8; e++)
            {
                cmd |= (uint64_t)tmpBuf[bufPntr++] << (e * 8);
            }

            // cmd = lazySWAP(cmd);
            wU64Mgs(&sMsg, cmd);

            setupWaitMessage(3);
            sleepMS(4);
            if (!send(&sMsg))
            {
                log(e39log, "Could not send!");
                return false;
            }

            // return false;
            // response = m_canListener.waitMessage(200);
            // respData = response.getData();

            rMsg = waitMessage(500);
            retVal = rU64Mgs(rMsg);

            if (retVal != cmd)
            {
                log(e39log, "Did not receive the same data");
                // CastInfoEvent("Did not receive the same data: " + respData.ToString("X16"), ActivityType.ConvertingFile);
                return false;
            }

            alignedSize -= 8;
        }

        return true;
    }

    return false;
}

// Calculated key e7f0 from seed a7a1

static void calculateKeyForE39(uint8_t *seed)
{
    uint32_t _seed = seed[0] << 8 | seed[1];
    uint32_t origSeed = _seed;

    if ( _seed != 0xffff )
    {
        _seed = (_seed + 0x6C50) & 0xFFFF;
        _seed = ((_seed << 8 | _seed >> 8) - 0x22DA) & 0xffff;
        _seed = ((_seed << 9 | _seed >> 7) - 0x8BAC);
    }
    else
    {
        _seed = 1;
    }

    _seed &= 0xffff;

    log(e39log, "Calculated key " + to_hex(_seed, 2) + " from seed " + to_hex(origSeed, 2));

    seed[0] = _seed >> 8;
    seed[1] = _seed;
}

bool e39::secAccE39(uint8_t lev)
{
    uint8_t buf[8] = { 0x27, lev };

    uint8_t *ret = sendRequest(buf, 2);
    if (ret == 0)
        return false;

    uint16_t retLen = (ret[0] << 8 | ret[1]) + 2;

    // 00 04 67 __ xx xx
    if (retLen < 6 || ret[2] != 0x67 || ret[3] != lev)
    {
        delete[] ret;
        return false;
    }

    if (ret[4] == 0 && ret[5] == 0)
    {
        log(e39log, "Already granted");
        delete[] ret;
        return true;
    }
    else
    {
        calculateKeyForE39( &ret[4] );
    }

    buf[0] = 0x27;
    buf[1] = lev + 1;
    buf[2] = ret[4];
    buf[3] = ret[5];
    delete[] ret;

    ret = sendRequest(buf, 4);
    if (ret == 0)
        return false;
    retLen = (ret[0] << 8 | ret[1]) + 2;

    // 00 04 67 __ xx xx
    if (retLen < 4 || ret[2] != 0x67 || ret[3] != (lev + 1))
    {
        delete[] ret;
        return false;
    }

    delete[] ret;
    return true;
}

bool e39::initSessionE39()
{
    fileManager fm;
    string ident = "";
    uint8_t *tmp;

    testerPresent();
    sleepMS(20);
    testerPresent();
    sleepMS(20);

    log(e39log, "Checking loader state..");

    if ( (tmp = ReadDataByIdentifier(0x90)) != nullptr )
    {
        uint32_t retLen = (tmp[0] << 8 | tmp[1]);
        for (uint32_t i = 0; i < retLen; i++)
            ident += tmp[i + 2];
        delete[] tmp;
    }

    if ( ident != "MPC5566-LOADER: TXSUITE.ORG" )
    {
        log(e39log, "Loader not active. Starting upload..");

        if ( !InitiateDiagnosticOperation(3) ) // 10
        {
            log(e39log, "initDiag error");
            return false;
        }

        sleepMS(10);

        disableNormalCommunication(); // 28
        sleepMS(10);

        testerPresent();

        if (!secAccE39(1))
        {
            log(e39log, "Could not gain security access");
            return false;
        }

        testerPresent();
        if (!ProgrammingMode(1))
            return false;

        // Won't give a response
        ProgrammingMode(3);

        testerPresent();

        sleepMS( 100 );

        fileHandle *file = fm.open( LOADER_BLOB );

        if (!file || file->size < (6 * 1024))
        {
            log(e39log, "No file or too small");
            return false;
        }

        uint32_t alignedSize = (file->size + 15) & ~15;
        uint8_t *tmpBuf = new uint8_t[ alignedSize ];
        if (tmpBuf == nullptr)
        {
            log(e39log, "Could not allocate buffer");
            return false;
        }

        memcpy(tmpBuf, file->data, file->size);

        tmpBuf[4] = modeE39 >> 24;
        tmpBuf[5] = modeE39 >> 16;
        tmpBuf[6] = modeE39 >> 8;
        tmpBuf[7] = modeE39;

        log(e39log, "Uploading bootloader");

        // The morons use a weird request without the format byte..
        if (!requestDownload_24( alignedSize ))
        {
            // The 39a log shows the ECU outright refusing but still accepting a transfer after. Weird fella..

            log(e39log, "Could not upload bootloader");
            delete[] tmpBuf;
            return false;
        }

        testerPresent();

        if (!transferData_32(tmpBuf, LOADER_BASE, alignedSize, 0x80, true))
        {
            log(e39log, "Could not upload bootloader");
            delete[] tmpBuf;
            return false;
        }

        log(e39log, "Bootloader uploaded");
        delete[] tmpBuf;
    }
    else
    {
        log(e39log, "Bootloader already active");
    }

    return true;
}

bool e39::initSessionE39A()
{
    fileManager fm;
    string ident = "";
    uint8_t *tmp;



    testerPresent();
    sleepMS(20);
    testerPresent();
    sleepMS(20);

    log(e39log, "Checking loader state..");

    if ( (tmp = ReadDataByIdentifier(0x90)) != nullptr )
    {
        uint32_t retLen = (tmp[0] << 8 | tmp[1]);
        for (uint32_t i = 0; i < retLen; i++)
            ident += tmp[i + 2];
        delete[] tmp;
    }

    if ( ident != "MPC5566-LOADER: TXSUITE.ORG" )
    {
        log(e39log, "Loader not active. Starting upload..");

// TODO: Investigate why it SOMETIMES wants this to be two
        if ( !InitiateDiagnosticOperation(3) ) // 10
        {
            log(e39log, "initDiag error");
            return false;
        }

        sleepMS(10);

        sleepMS(10);

        disableNormalCommunication(); // 28
        sleepMS(10);

        testerPresent();

        if (!secAccE39( 1 ))
        {
            log(e39log, "Could not gain security access");
            return false;
        }

        testerPresent();
        if (!ProgrammingMode(1))
            return false;

        // Won't give a response
        ProgrammingMode(3);

        testerPresent();

        sleepMS( 100 );

        fileHandle *file = fm.open( LOADER_BLOB );

        if (!file || file->size < (6 * 1024))
        {
            log(e39log, "No file or too small");
            return false;
        }

        uint32_t alignedSize = (file->size + 15) & ~15;
        uint8_t *tmpBuf = new uint8_t[ alignedSize ];
        if (tmpBuf == nullptr)
        {
            log(e39log, "Could not allocate buffer");
            return false;
        }

        memcpy(tmpBuf, file->data, file->size);

        // Mine couldn't care less?
        tmpBuf[4] = modeE39A >> 24;
        tmpBuf[5] = modeE39A >> 16;
        tmpBuf[6] = modeE39A >> 8;
        tmpBuf[7] = modeE39A;

        log(e39log, "Uploading bootloader");

        // The morons use a weird request without the format byte..
        if (!requestDownload_24( alignedSize ))
        {
            // The 39a log shows the ECU outright refusing but still accepting a transfer after. Weird fella..

            log(e39log, "Could not upload bootloader");
            delete[] tmpBuf;
            return false;
        }

        testerPresent();

        if (!transferData_32(tmpBuf, LOADER_BASE, alignedSize, 0x80, true))
        {
            log(e39log, "Could not upload bootloader");
            delete[] tmpBuf;
            return false;
        }

        log(e39log, "Bootloader uploaded");
        delete[] tmpBuf;
    }
    else
    {
        log(e39log, "Bootloader already active");
    }

    return true;
}

bool e39::dump(const char *name, const ECU & target)
{
    fileManager fm;
    uint8_t *buffer;
    bool retVal = false;
    static const uint32_t nBytes = 0x300000 + 1024;

    log(e39log, "Dumping e39");

    configProtocol();

    switch ( target )
    {
    case ecu_AcDelcoE39:
        retVal = initSessionE39();
        break;
    case ecu_AcDelcoE39A:
        retVal = initSessionE39A();
        break;
    case ecu_AcDelcoE39BAM:
        log(e39log, "Can not init a dump session in BAM since the key must be known");
        break;
    default: break;
    }

    if ( retVal == false )
    {
        return false;
    }

/*
    if (loader_StartRoutineById(0, 0, nBytes))
    {
        uint8_t *dat;
        if ((dat = loader_requestRoutineResult(0)) != 0)
        {
            uint32_t retLen = (uint32_t)(dat[0] << 8 | dat[1]);
            string md5 = "";
            for (uint32_t i = 0; i < retLen; i++)
                md5 += to_hex((volatile uint32_t)dat[2 + i], 1);

            log(e39log, "Success: " + md5);
            delete[] dat;
        }
    }
*/

    uint32_t InterFrame = GMLOADER_INTERFRAME;
    uint8_t byId[2] = { (uint8_t)(InterFrame >> 8), (uint8_t)InterFrame };

    if (WriteDataByIdentifier(byId, 0x91, 2))
    {
        log(e39log, "Delay set ok");
    }

    log(e39log, "Dumping");

    if ( (buffer = loader_readMemoryByAddress(0, nBytes, 245)) == nullptr )
    {
        log(e39log, "Read failed");
        returnToNormal();
        return false;
    }

    log(e39log, "Dump ok");

    fm.write( name, buffer, nBytes );

    delete[] buffer;

    returnToNormal();

    return true;
}
