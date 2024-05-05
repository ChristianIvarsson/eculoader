#include <cstdio>
#include <string>

namespace logger
{
    // For future in case you want to push the log to a particular file or parent
    enum logWho {
        genericlog    ,
        lzcomplog     ,
        filemanager   ,
        e39log        ,
        gmlanlog      ,
        adapterlog    ,
    };

    class logger_t {
    public:
        virtual void log(const logWho&, const std::string&) = 0;
        virtual void progress(uint32_t) = 0;
    };

    void loggerInstall (logger_t*);
    void log           (logWho, std::string);
    void log           (std::string);
    void progress      (uint32_t);
}
