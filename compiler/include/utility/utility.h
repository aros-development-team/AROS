#ifndef UTILITY_UTILITY_H
#define UTILITY_UTILITY_H
#include <exec/types.h>
#include <exec/libraries.h>
#include <dos/dos.h>

#define UTILITYNAME	"utility.library"

struct UtilityBase
{
    struct Library ub_LibNode;
    UBYTE ub_Language;
    UBYTE ub_Reserved;
    struct ExecBase *ub_SysBase;
    BPTR ub_SegList;
};

#endif
