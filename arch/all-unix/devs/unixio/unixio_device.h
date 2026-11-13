#include <exec/devices.h>
#include <oop/oop.h>

struct UnitData
{
    struct Node    unitNode;
    IPTR           fd;
    OOP_Object    *unixio;
    ULONG          usecount;
    struct MinList readQueue;
    struct MinList writeQueue;
    ULONG          writeLength;
    void         (*errorCallback)(struct IOStdReq *req, int err);
    BOOL           stopped;
    BOOL           eofmode;
    unsigned char  termarray[8];
    char           unitName[1];
};

struct UnixDevice
{
    struct Device          dev;
    APTR                   hostlib;
    struct SignalSemaphore sigsem;
    struct MinList         units;
    OOP_Object            *unixio;
    int                  (*raise)(int sig);
    struct LibcInterface  *iface;
};
