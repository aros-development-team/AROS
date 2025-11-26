/* This file can be included multiple times with different DB_LEVEL */
#undef DB
#undef KPRINTF

#ifndef DB_LEVEL
#define DB_LEVEL 200
#endif

#ifndef DEBUG
#define DEBUG 0
#endif

#if defined(AROS_USE_LOGRES)
#ifndef __LOG_NOLIBBASE__
#define __LOG_NOLIBBASE__
#endif
#include <proto/log.h>
#include <resources/log.h>
#else
#include <aros/debug.h>
#include <proto/debug.h>
#endif

// DEBUG 0 should equal undefined DEBUG
#ifdef DEBUG
#if DEBUG == 0
#undef DEBUG
#endif
#endif

#if defined(AROS_USE_LOGRES) && (DEBUG > 0)
#define KPRINTF(l,fmt,args...)          if (LogHandle){ logAddEntry((LOGF_Flag_Type_Debug | l), LogHandle, "", __func__, 0, fmt, ##args); }
#define pciusbDebug(sub,fmt,args...)    if (LogHandle){ logAddEntry((LOGF_Flag_Type_Debug | 20), LogHandle, sub, __func__, 0, fmt, ##args); }
#else
#ifdef DEBUG
#define KPRINTF(l,fmt,args...) do { if ((l) >= DB_LEVEL) \
     { KPrintF("%s/%lu: ", __func__, __LINE__); KPrintF(fmt, ##args);} } while (0)
#define pciusbDebug(sub,fmt,args...) \
     { KPrintF("%s/%lu: ", __func__, __LINE__); KPrintF(fmt, ##args);}
#define DB(x) x
void dumpmem_pciusb(void *mem, unsigned long int len);
#else /* !DEBUG */
#define KPRINTF(l,fmt,args...) ((void) 0)
#define pciusbDebug(fmt,args...)
#define DB(x)
#endif
#endif /* DEBUG */
#if defined(AROS_USE_LOGRES)
#define pciusbInfo(sub, fmt,args...)    if (LogHandle){ logAddEntry((LOGF_Flag_Type_Information | 20), LogHandle, sub, __func__, 0, fmt, ##args); }
#define pciusbWarn(sub, fmt,args...)    if (LogHandle){ logAddEntry((LOGF_Flag_Type_Warn | 20), LogHandle, sub, __func__, 0, fmt, ##args); }
#define pciusbError(sub, fmt,args...)   if (LogHandle){ logAddEntry((LOGF_Flag_Type_Error | 20), LogHandle, sub, __func__, 0, fmt, ##args); }
#else
#define pciusbInfo(sub, fmt,args...) \
     { KPrintF("%s/%lu: ", __func__, __LINE__); KPrintF(fmt, ##args);}
#define pciusbWarn(sub, fmt,args...) \
     { KPrintF("%s/%lu: ", __func__, __LINE__); KPrintF(fmt, ##args);}
#define pciusbError(sub, fmt,args...) \
     { KPrintF("%s/%lu: ", __func__, __LINE__); KPrintF(fmt, ##args);}
#endif

#if defined(DEBUG)
//#define PCIUSB_OHCI_DEBUG
//#define PCIUSB_UHCI_DEBUG
//#define PCIUSB_EHCI_DEBUG
#if defined(PCIUSB_ENABLEXHCI)
//#define PCIUSB_XHCI_DEBUG
//#define XHCI_LONGDEBUGNAK
#endif
#endif
