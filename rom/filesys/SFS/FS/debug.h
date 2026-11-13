#ifndef _DEBUG_H
#define _DEBUG_H

#include <exec/types.h>
#include <proto/debug.h>
#include <proto/dos.h>

#include "debug_protos.h"
#include "fs.h"
#include "globals.h"

#ifdef __AROS__
#include <aros/debug.h>
#else

#ifdef DEBUG
#define D(x) x
#else
#define D(x)
#endif

#define bug _DEBUG

#endif

#ifdef DEBUGCODE
#if defined(AROS_USE_LOGRES)
#define _DEBUG(fmt,args...)                                                                             \
    if (globals->logHandle) {                                                                           \
        logAddEntry((LOGF_Flag_Type_Debug | 50), globals->logHandle, "", __func__, 0, fmt, ##args);     \
    }
#define _TDEBUG _DEBUG
#define _XDEBUG(type,fmt,args...)                                                                       \
do {                                                                                                    \
    ULONG debug=globals->mask_debug;                                                                    \
    ULONG debugdetailed=globals->mask_debug & ~(DEBUG_CACHEBUFFER|DEBUG_NODES|DEBUG_LOCK|DEBUG_BITMAP); \
    if((debugdetailed & type)!=0 || ((type & 1)==0 && (debug & type)!=0))                               \
    {                                                                                                   \
        if (globals->logHandle) {                                                                       \
            logAddEntry((LOGF_Flag_Type_Debug | 50), globals->logHandle, "", __func__, 0, fmt, ##args); \
        }                                                                                               \
    }                                                                                                   \
} while (0)
#else
#define _TDEBUG(fmt,args...)                                                                            \
do {                                                                                                    \
    struct DateStamp ds;                                                                                \
    DateStamp(&ds);                                                                                     \
    KPrintF("%4ld.%4ld ", ds.ds_Minute, ds.ds_Tick*2);                                                  \
    KPrintF(fmt,##args);                                                                                \
} while (0)
#define _DEBUG(fmt,args...) KPrintF("[SFS] "); KPrintF(fmt,##args)
#define _XDEBUG(type,fmt,args...)                                                                       \
do {                                                                                                    \
    ULONG debug=globals->mask_debug;                                                                    \
    ULONG debugdetailed=globals->mask_debug & ~(DEBUG_CACHEBUFFER|DEBUG_NODES|DEBUG_LOCK|DEBUG_BITMAP); \
    if((debugdetailed & type)!=0 || ((type & 1)==0 && (debug & type)!=0))                               \
    {                                                                                                   \
        KPrintF(fmt,##args);                                                                            \
    }                                                                                                   \
} while (0)
#endif
#else
#define _TDEBUG(fmt,args...)
#define _DEBUG(fmt,args...)
#define _XDEBUG(type,fmt,args...)
#endif

#define DEBUG_DETAILED (1)

#define DEBUG_CACHEBUFFER (2)
#define DEBUG_NODES       (4)
#define DEBUG_IO          (8)
#define DEBUG_SEEK        (16)
#define DEBUG_BITMAP      (32)
#define DEBUG_LOCK        (64)
#define DEBUG_OBJECTS     (128)
#define DEBUG_TRANSACTION (256)

#define DDEBUG_CACHEBUFFER (DEBUG_DETAILED + DEBUG_CACHEBUFFER)
#define DDEBUG_NODES       (DEBUG_DETAILED + DEBUG_NODES)
#define DDEBUG_IO          (DEBUG_DETAILED + DEBUG_IO)
#define DDEBUG_SEEK        (DEBUG_DETAILED + DEBUG_SEEK)
#define DDEBUG_BITMAP      (DEBUG_DETAILED + DEBUG_BITMAP)
#define DDEBUG_LOCK        (DEBUG_DETAILED + DEBUG_LOCK)
#define DDEBUG_OBJECTS     (DEBUG_DETAILED + DEBUG_OBJECTS)
#define DDEBUG_TRANSACTION (DEBUG_DETAILED + DEBUG_TRANSACTION)

#endif // _DEBUG_H
