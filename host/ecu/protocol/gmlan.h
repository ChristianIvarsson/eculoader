#ifndef GMLAN_H
#define GMLAN_H

#include "../../adapter/adapter.h"

typedef struct {
    uint32_t PMA;  // Address
    uint32_t NOB;  // Size
} range_t;

typedef struct {
    uint32_t DCID;       // Data Compatibility identifier
    uint32_t NOAR;       // Number Of Address Regions
    range_t  range[16];  // Start address, size
} module_t;

class gmlan : public virtual adapter
{
    uint64_t testerID;
    uint64_t targetID;
public:

    // Used by readMemoryByAddress, requestDownload, transferData etc
    enum enGmMemSz : uint32_t
    {
        gmSize8  = 1,
        gmSize16 = 2,
        gmSize24 = 3,
        gmSize32 = 4
    };

    void setTesterID(uint64_t ID);
    void setTargetID(uint64_t ID);
    uint64_t getTesterID();
    uint64_t getTargetID();

    uint32_t busyWaitTimeout = 6000;

/*  < From ECU perspective so useless to this code >
    - Time between tester request (N_USData.ind) and the ECU
      response as follows: The ECU has to make sure that a USDT single
      frame response message, the first frame of a multi-frame response
      message, or the first UUDT response message is completed
      (successfully transmitted onto the CAN bus) within P2CE */
    // uint32_t p2ce = 100; // (0 - 100 / 0 - 5000)

/*  < From tester perspective >
    - The timeout between the end of the tester request message (single frame or multi-frame request
      message) and the first received frame of either the USDT response message (single frame, or first frame
      of a multi-frame response), or the first UUDT message of the services $AA, $A9 (there may be subsequent
      UUDT messages transmitted by the ECU to transmit the requested data) AND 
    - The timeout between each response message from multiple nodes responding during functional
      communication. */
    uint32_t p2ct = 500; // (150 - xxxxxx  ms) 

    // Time between tester present messages
    uint32_t p3c = 3500; // (5000 nominal / 5100 max)

    // Send shortest possible ack response instead of a full-length one
    // short: [ 3 ] 30 00 00
    // long:  [ 8 ] 30 00 00 .. .. 00
    // Standard says it only needs to be 3 bytes but some targets are unable to cope with such messages
    bool shortAck = false;

    uint8_t *sendRequest(uint8_t*, uint32_t, bool expectResponse = true);

    void sendAck(); // 3x xx xx

    // 04
    bool clearDiagnosticInformation();

    // 10
    enum enDiagOp : uint32_t
    {
        disableAllDTCs           = 0x02,
        enableDTCsDuringDevCntrl = 0x03,
        wakeUpLinks              = 0x04,
    };
    bool InitiateDiagnosticOperation(enDiagOp mode);

    // 1a
    uint8_t *ReadDataByIdentifier(uint8_t id);

    // 20
    bool returnToNormal();

    // 23
    uint8_t *readMemoryByAddress(
                        uint32_t  address,
                        uint32_t  length,
                        enGmMemSz addressSize = gmSize24,
                        enGmMemSz lengthSize  = gmSize16,
                        uint32_t  blockSize   = 0x80);

    // 27
    typedef bool keyFunc_t(uint8_t *seed, uint32_t seedLen);
    bool securityAccess(uint32_t lev, keyFunc_t & keyFunc);

    // 28
    bool disableNormalCommunication(int exdAddr = -1);
    bool disableNormalCommunicationNoResp(int exdAddr = -1);

    // 34
    bool RequestDownload(
                    uint32_t  size,
                    enGmMemSz lengthSize  = gmSize16,
                    uint32_t  fmt         = 0);

    // 36
    bool transferData(
                    const uint8_t *data,
                    uint32_t       address,
                    uint32_t       length,
                    bool           execute = false,
                    enGmMemSz      addressSize = gmSize24,
                    uint32_t       blockSize = 0x80);

    // 3b
    bool WriteDataByIdentifier(const uint8_t *dat, uint8_t id, uint32_t len);

    // 3e
    // Note: Setting extended address makes it broadcast the message with id 101
    bool testerPresent(int exdAddr = -1);

    // a2
    bool ReportProgrammedState();

    // a5
    enum enProgOp : uint32_t
    {
        requestProgrammingMode           = 0x01,
        requestProgrammingMode_HighSpeed = 0x02,
        enableProgrammingMode            = 0x03, // ( Expected NOT to respond )
    };
    bool programmingMode(enProgOp, bool expectResponse = true);

    // ae
    bool DeviceControl(const uint8_t*,uint8_t,uint8_t);
};

#endif
