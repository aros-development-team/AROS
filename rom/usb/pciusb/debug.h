/* This file can be included multiple times with different DB_LEVEL */
#undef DB
#undef KPRINTF

#ifndef DB_LEVEL
#define DB_LEVEL 200
#endif

#ifndef DEBUG
#define DEBUG 0
#endif

//#define PCIUSB_ANSI_DEBUG

#if defined(AROS_USE_LOGRES)
#ifndef __LOG_NOLIBBASE__
#define __LOG_NOLIBBASE__
#endif
#include <proto/log.h>
#include <resources/log.h>
#else
#include <aros/debug.h>
#endif
#include <proto/debug.h>

// DEBUG 0 should equal undefined DEBUG
#ifdef DEBUG
#if DEBUG == 0
#undef DEBUG
#endif
#endif

#if defined(AROS_USE_LOGRES) && (DEBUG > 0)
#define KPRINTF(l,fmt,args...) do { \
    if (LogHandle) { \
        logAddEntry((LOGF_Flag_Type_Debug | l), LogHandle, "", __func__, 0, fmt, ##args); \
    } else if ((l) >= DB_LEVEL) { \
        KPrintF("%s/%lu: ", __func__, __LINE__); \
        KPrintF(fmt, ##args); \
    } \
} while (0)
#define pciusbDebug(sub,fmt,args...) do { \
    if (LogHandle) { \
        logAddEntry((LOGF_Flag_Type_Debug | 20), LogHandle, sub, __func__, 0, fmt, ##args); \
    } else { \
        KPrintF("%s/%lu: ", __func__, __LINE__); \
        KPrintF(fmt, ##args); \
    } \
} while (0)
#if (DEBUG > 1)
#define pciusbDebugV(sub,fmt,args...) do { \
    if (LogHandle) { \
        logAddEntry((LOGF_Flag_Type_Debug | 20), LogHandle, sub, __func__, 0, fmt, ##args); \
    } else { \
        KPrintF("%s/%lu: ", __func__, __LINE__); \
        KPrintF(fmt, ##args); \
    } \
} while (0)
#else
#define pciusbDebugV(sub,fmt,args...) ((void) 0)
#endif
#else
#ifdef DEBUG
#define KPRINTF(l,fmt,args...) do { \
    if ((l) >= DB_LEVEL) { \
        KPrintF("%s/%lu: ", __func__, __LINE__); \
        KPrintF(fmt, ##args); \
    } \
} while (0)
#define pciusbDebug(sub,fmt,args...) do { \
    KPrintF("%s/%lu: ", __func__, __LINE__); \
    KPrintF(fmt, ##args); \
} while (0)
#if (DEBUG > 1)
#define pciusbDebugV(sub,fmt,args...) do { \
    KPrintF("%s/%lu: ", __func__, __LINE__); \
    KPrintF(fmt, ##args); \
} while (0)
#else
#define pciusbDebugV(sub,fmt,args...) ((void) 0)
#endif
#define DB(x) x
void dumpmem_pciusb(void *mem, unsigned long int len);
#else /* !DEBUG */
#define KPRINTF(l,fmt,args...) ((void) 0)
#define pciusbDebug(sub,fmt,args...) ((void) 0)
#define pciusbDebugV(sub,fmt,args...) ((void) 0)
#define DB(x)
#endif /* DEBUG */
#endif
#if defined(AROS_USE_LOGRES)
#define pciusbInfo(sub, fmt,args...) do { \
    if (LogHandle){ \
       logAddEntry((LOGF_Flag_Type_Information | 20), LogHandle, sub, __func__, 0, fmt, ##args); \
    } \
} while (0)
#define pciusbWarn(sub, fmt,args...) do { \
    if (LogHandle){ \
       logAddEntry((LOGF_Flag_Type_Warn | 20), LogHandle, sub, __func__, 0, fmt, ##args); \
    } \
} while (0)
#define pciusbError(sub, fmt,args...) do { \
    if (LogHandle){ \
       logAddEntry((LOGF_Flag_Type_Error | 20), LogHandle, sub, __func__, 0, fmt, ##args); \
    } \
} while (0)
#else
#define pciusbInfo(sub, fmt,args...) do { \
    KPrintF("%s/%lu: ", __func__, __LINE__); \
    KPrintF(fmt, ##args); \
} while (0)
#define pciusbWarn(sub, fmt,args...) do { \
    KPrintF("%s/%lu: ", __func__, __LINE__); \
    KPrintF(fmt, ##args); \
} while (0)
#define pciusbError(sub, fmt,args...) do { \
    KPrintF("%s/%lu: ", __func__, __LINE__); \
    KPrintF(fmt, ##args); \
} while (0)
#endif

#if defined(DEBUG)
//#define PCIUSB_OHCI_DEBUG
//#define PCIUSB_UHCI_DEBUG
//#define PCIUSB_EHCI_DEBUG
#if !defined(AROS_USE_LOGRES) && defined(PCIUSB_ANSI_DEBUG)
#define DEBUGCOLOR_SET                          "\033[32m"
#define DEBUGWARNCOLOR_SET                      "\033[31m"
#define DEBUGFUNCCOLOR_SET                      "\033[32;1m"
#define DEBUGCOLOR_RESET                        "\033[0m"
#endif
#endif

#ifndef DEBUGCOLOR_SET
#define DEBUGCOLOR_SET
#endif
#ifndef DEBUGWARNCOLOR_SET
#define DEBUGWARNCOLOR_SET
#endif
#ifndef DEBUGFUNCCOLOR_SET
#define DEBUGFUNCCOLOR_SET
#endif
#ifndef DEBUGCOLOR_RESET
#define DEBUGCOLOR_RESET
#endif
