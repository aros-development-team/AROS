#include <exec/libraries.h>
#include <oop/oop.h>

extern OOP_Object *unixio;
extern int audio_fd;

struct OSS_Base
{
    struct Library lib;
    OOP_AttrBase UnixIOAttrBase;
};

#undef HiddUnixIOAttrBase
#define HiddUnixIOAttrBase LIBBASE->UnixIOAttrBase
