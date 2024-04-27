#include <iostream>
#include <csignal>
#include <cstdlib>

#include "tools/tools.h"

#include "ecu/ecu.h"

using namespace logger;
using namespace std;
using namespace std::chrono;

typedef bool cmdFunc(char *[], ECU & target);

typedef struct {
    const char    *command;
    const int      nArgs;
    const cmdFunc *func;
} cmd_t;

typedef struct {
    const adaptertypes type;
    const char        *name;
} adaptArg_t;

typedef struct {
    const ECU   type;
    const char *name;
} targArg_t;

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

// Rework..
class ecuSpace : public e39
{};

ecuSpace ecu;

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
        exit(1);
    };

    signal(SIGINT, lam);
    signal(SIGABRT, lam);
    signal(SIGTERM, lam);

    loggerInstall( &parentlogger );
}

static bool fncDumpEcu(char *argv[], ECU & target);

static const adaptArg_t adaptArgs[] = {
    { adapterCanUsb    , "canusb"    },
    { adapterKvaser    , "kvaser"    },
};

static const targArg_t targetArgs[] = {
    { ecu_AcDelcoE39   , "e39"       },
};

static const cmd_t cmdArgs[] = {
    { "--dump"     , 1, fncDumpEcu      },
};

static bool populateChannel(channelData & dat, const ECU & ecu)
{
    switch ( ecu )
    {
    case ecu_AcDelcoE39:
        return e39::adapterConfig(dat, ecu);
    default:
        break;
    }

    printf("Error: Don't know how to load adapter parameters for this target!\n");
    return false;
}

static bool fncDumpEcu(char *argv[], ECU & target)
{
    bool retVal = false;

    auto timeStart = system_clock::now();

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

    uint64_t msTaken = duration_cast<milliseconds>(system_clock::now() - timeStart).count();
    uint32_t secTaken = (msTaken /  1000) % 60;
    uint32_t minTaken = (msTaken / 60000) % 60;
    msTaken %= 1000;

    log("Duration " + to_string(minTaken) + "m, " + to_string(secTaken) + "s, " + to_string(msTaken) + "ms");

    return retVal;
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

int main(int argc, char *argv[])
{
    string adapterList = "";
    list <string> adapters;
    channelData chDat;

    prepMain();

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

    if ( !populateChannel( chDat, target ) )
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




/*

    channelData chDat;
    chDat.name = adapters.front();
    chDat.bitrate = btr500k;
    chDat.canIDs = { 0x7e0, 0x7e8, 0x101, 0x011, 0x002, 0x003 };

    if(! ecu.open( chDat ) )
    {
        log("Could not open");
        return 1;
    }

    ecu.e39::dump();
*/


    return 0;
}