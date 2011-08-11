#include <android/log.h>
#include <stdarg.h>
#include <stdio.h>

/* These macros are defined in both UNIX and AROS headers. Get rid of warnings. */
#undef __pure
#undef __const
#undef __pure2
#undef __deprecated

#include <aros/config.h>
#include <aros/kernel.h>
#include <exec/lists.h>
#include <exec/resident.h>
#include <dos/bptr.h>

#include "../../../arch/all-unix/kernel/hostinterface.h"

#include "bootstrap.h"
#include "hostlib.h"
#include "shutdown.h"

#if AROS_MODULES_DEBUG
/* gdb hooks from which it obtains modules list */

/* This is needed in order to bring in definition of struct segment */
#include "../../../rom/debug/debug_intern.h"

struct segment *seg = NULL;
struct Resident *res = NULL;
struct MinList *Debug_ModList = NULL;
#endif

static int p = 0;

/*
 * On Android additionally to stderr we output the log to Android's
 * own buffer. This is better than nothing.
 * Since Android's debug output is line-oriented, we have to accumulate
 * the text in the buffer.
 */

static void flushLog(void)
{
    buf[p] = 0;
    __android_log_write(ANDROID_LOG_DEBUG, "AROS", buf);
    p = 0;
}    
 
static int KPutC(int chr)
{
    int ret;

    ret = fputc(chr, stderr);

    if (chr == '\n')
    {
        fflush(stderr);
        flushLog();
    }
    else
    {
        buf[p++] = chr;

    	if (p == BUFFER_SIZE - 1)
    	    flushLog();
    }

    return ret;
}

/*
 * Some helpful functions that link us to the underlying host OS.
 * Without them we would not be able to estabilish any interaction with it.
 */
static struct HostInterface _HostIFace =
{
    "linux-arm",
    HOSTINTERFACE_VERSION,

    Host_HostLib_Open,
    Host_HostLib_Close,
    Host_HostLib_GetPointer,
    KPutC,
#if AROS_MODULES_DEBUG
    &Debug_ModList,
#else
    NULL,
#endif
};

void *HostIFace = &_HostIFace;
