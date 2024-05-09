#include <cstdio>
#include <iostream>
#include <csignal>
#include <cstdlib>

#include "tools/tools.h"

#include "ecu/ecu.h"

using namespace logger;
using namespace std;

typedef bool cmdFunc(char *[], ECU & target);

typedef struct {
    const char    *command;
    const int      nArgs;
    const cmdFunc *func;
} cmd_t;

typedef struct {
    const char        *name;
    const adaptertypes type;
} adaptArg_t;

typedef struct {
    const char *name;
    const ECU   type;
} targArg_t;

static bool fncDumpEcu   (char *argv[], ECU & target);
static bool fncFlashEcu  (char *argv[], ECU & target);

// List of possible adapters
static const adaptArg_t adaptArgs[] = {
    { "canusb"    , adapterCanUsb     },
    { "kvaser"    , adapterKvaser     }
};

// List of possible targets
static const targArg_t targetArgs[] = {
    {  "e39"      , ecu_AcDelcoE39    },
};

// List of possible operations
static const cmd_t cmdArgs[] = {
    { "--dump"    ,  1, fncDumpEcu    },
    { "--flash"   ,  1, fncFlashEcu   },
};

// Rework..
class ecuSpace : public e39
{};

ecuSpace ecu;

// TODO: Clean!

// --dump command
static bool fncDumpEcu(char *argv[], ECU & target)
{
    stopWatch sw;

    bool retVal = false;

    sw.capture();

    switch ( target )
    {
    case ecu_AcDelcoE39:
    case ecu_AcDelcoE39A:
    case ecu_AcDelcoE39BAM:
        retVal = ecu.e39::dump( argv[0], target );
        break;
    default:
        printf("Error: Unknown dump target\n");
        return false;
    break;
    }

    sw.capture();
    sw.print();

    return retVal;
}

// --flash command
static bool fncFlashEcu(char *argv[], ECU & target)
{
    stopWatch sw;

    bool retVal = false;

    sw.capture();

    switch ( target )
    {
    case ecu_AcDelcoE39:
    case ecu_AcDelcoE39A:
    case ecu_AcDelcoE39BAM:
        retVal = ecu.e39::flash( argv[0], target );
        break;
    default:
        printf("Error: Unknown dump target\n");
        return false;
    break;
    }

    sw.capture();
    sw.print();

    return retVal;
}

class parentlogger_t : public logger_t {
public:
    void log(const logWho & who, const string & message)
    {
        string loggedby;

        switch (who)
        {
            case filemanager:  loggedby = "Filemanager   : "; break;
            case e39log:       loggedby = "E39           : "; break;
            case gmlanlog:     loggedby = "GMLAN         : "; break;
            case adapterlog:   loggedby = "Adapter       : "; break;
            default:           loggedby = "- - -         : "; break;
        }

        cout << loggedby + message + '\n';
    }

    void progress(uint32_t prog)
    {
        static uint32_t oldProg = 100;

        if ( prog > 100 )
            prog = 100;
        
        if ( prog < oldProg || prog >= (oldProg + 5) || (oldProg < prog && prog == 100))
        {
            oldProg = prog;
            cout << to_string( prog ) << "%" "\n";
        }
    }
};

parentlogger_t parentlogger;

static void exiting()
{
    ecu.close();
    ecu.~ecuSpace();
    log("Going down");
    exit(1);
}

static void prepMain()
{
    srand(time(NULL));

    auto lam = [] (int i)
    {
        exiting();
    };

    signal(SIGINT, lam);
    signal(SIGABRT, lam);
    signal(SIGTERM, lam);

    loggerInstall( &parentlogger );
}

bool strMatch(const char *strA, const char *strB)
{
    size_t strIdx = 0;

    if ( strA == nullptr || strB == nullptr )
    {
        return false;
    }

    while ( strA[ strIdx ] == strB[ strIdx ] && strA[ strIdx ] != 0 )
    {
        strIdx++;
    }

    return ( strA[ strIdx ] == strB[ strIdx ] );
}


#define testSize  (8192 * 1024)
void lzTest()
{
    lzcomp lz;

    printf("Testing Lz..\n");

    uint8_t *tmp = new uint8_t[ testSize ];
    for (size_t i = 0; i < testSize; i++)
    {
        tmp[ i ] = (uint8_t)i; // (uint8_t)rand();
    }

    // memset(tmp, 0, testSize / 2);

    int32_t ticket = lz.push( tmp, testSize );


    if ( ticket < 0 )
    {
        printf("Lz dafuq?!\n");
        exit( 1 );
    }

    lzData_t popDat = lz.get( ticket );

    if ( popDat.data == nullptr )
    {
        printf("Null data!\n");
        exit( 1 );
    }

    // delete[] popDat.data;

    lz.flush();

    ticket = lz.push( tmp, testSize );

    if ( ticket < 0 )
    {
        printf("Lz dafuq?!\n");
        exit( 1 );
    }

    popDat = lz.get( ticket );

    if ( popDat.data == nullptr )
    {
        printf("Null data!\n");
        exit( 1 );
    }

    timer::sleepMilli( 4000 );
}

int main(int argc, char *argv[])
{
    string adapterList = "";
    list <string> adapters;
    channelData chDat;

    prepMain();

    // lzTest();

    if ( argc < 2 )
    {
        printf(
            "Available commands:\n"
            "--dump < file >\n"
            "--adapter < kvaser, canusb .. >\n"
            "--target < e39, e39bam .. >\n"
        );

        return 1;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Find specified adapter or the first available one
    adaptertypes adptr = adapterUnknown;

    for ( int i = 1; i < argc; i++ )
    {
        if ( strMatch("--adapter", argv[ i ]) )
        {
            if ( i < (argc - 1) )
            {
                for ( size_t a = 0; a < (sizeof(adaptArgs) / sizeof(adaptArgs[0])); a++ )
                {
                    if ( strMatch(adaptArgs[ a ].name, argv[ i + 1 ]) )
                    {
                        adptr = adaptArgs[ a ].type;
                        printf("Info: Selected adapter \"%s\"\n", adaptArgs[ a ].name);
                        goto adaptKnown;
                    }
                }

                printf("Error: Unknown adapter specified\n");
                return 1;
            }

            printf("Error: Specify adapter after \"--adapter\"\n");
            return 1;
        }
    }

adaptKnown:

    if ( adptr == adapterUnknown )
    {
        // .. try all combinations
        printf("No adapter specified - Trying first available one..\n");
        
        for ( size_t a = 0; a < (sizeof(adaptArgs) / sizeof(adaptArgs[0])); a++ )
        {
            printf("Info: Trying adapters of type \"%s\"\n", adaptArgs[ a ].name);
            adapters = ecu.listAdapters( adaptArgs[ a ].type );

            if ( adapters.size() != 0 )
            {
                printf("Info: Found adapters\n");

                int idx = 0;

                for (string adapt : adapters)
                {
                    adapterList += adapt + " ";
                    printf("Info: < %d > %s\n", idx++, adapt.c_str());
                }

                break;
            }
        }
    }
    else
    {
        adapters = ecu.listAdapters( adptr );

        if ( adapters.size() != 0 )
        {
            int idx = 0;
            for (string adapt : adapters)
            {
                adapterList += adapt + " ";
                printf("Info: < %d > %s\n", idx++, adapt.c_str());
            }
        }
    }

    if ( adapters.size() == 0 )
    {
        printf("Error: No adapters\n");
        return 1;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Find selected ECU
    ECU target = ecu_Unknown;

    for ( int i = 1; i < argc; i++ )
    {
        if ( strMatch("--target", argv[ i ]) )
        {
            if ( i < (argc - 1) )
            {
                for ( size_t a = 0; a < (sizeof(targetArgs) / sizeof(targetArgs[0])); a++ )
                {
                    if ( strMatch(targetArgs[ a ].name, argv[ i + 1 ]) )
                    {
                        target = targetArgs[ a ].type;
                        printf("Info: Selected target \"%s\"\n", targetArgs[ a ].name);
                        goto targetKnown;
                    }
                }

                printf("Error: Unknown target specified\n");
                return 1;
            }

            printf("Error: Specify target after \"--target\"\n");
            return 1;
        }
    }

targetKnown:

    if ( target == ecu_Unknown )
    {
        printf("Error: Specify target with --target\n");
        return 1;
    }

    // Ugly. Implement index and detection based on constraints
    chDat.name = adapters.front();

    if ( !ecu_target::adapterConfig( chDat, target ) )
    {
        printf("Error: Could not load channel data\n");
        return 1;
    }

    if(! ecu.open( chDat ) )
    {
        printf("Error: Could not open comms\n");
        return 1;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Now to operations
    bool opStatus = false;

    // Iterate over commands
    for ( int i = 1; i < argc; i++ )
    {
        for ( size_t a = 0; a < (sizeof(cmdArgs) / sizeof(cmdArgs[0])); a++ )
        {
            if ( strMatch(cmdArgs[ a ].command, argv[ i ]) )
            {
                printf("Command match\n");

                if ( cmdArgs[ a ].func == nullptr )
                {
                    printf("Error: Argument \"%s\" doesn't have a function pointer\n", cmdArgs[ a ].command);
                    return 1;
                }

                if ( (i + cmdArgs[ a ].nArgs) >= argc )
                {
                    printf("Error: Not enough args for command \"%s\"\n", cmdArgs[ a ].command);
                    return 1;
                }

                if ( (opStatus = cmdArgs[ a ].func( &argv[ i + 1 ], target )) == false )
                {
                    printf("Error: Command failed\n");
                    return 1;
                }
            }
        }
    }

    if ( opStatus == false )
    {
        printf("Error: Operation failed\n");
        return 1;
    }

    return 0;
}