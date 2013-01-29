#ifndef HIDD_HIDD_H
#define HIDD_HIDD_H

/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Main HIDD Include File
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef UTILITY_TAGITEM_H
#   include <utility/tagitem.h>
#endif

/* ---------------------------------------------------------------------
    Main HIDD Class Interface
   --------------------------------------------------------------------- */

#define CLID_Hidd    "hiddclass" /* Topmost HIDD class                    */
#define CLID_HW      "hwclass"   /* Topmost hardware class                */
#define CLID_HW_Root "hw.root"   /* Root of subsystem of hardware classes */

/* Meta class for the HIDDs */
/* Just set it to the mimetaclass for now */
#define CLID_HiddMeta   "simetaclass"

#ifndef __typedef_HIDD
#   define __typedef_HIDD
    typedef APTR HIDD;
#endif

#include <interface/Hidd.h>
#include <interface/HW.h>

/* Values for the HIDD_Type Tag */
#define vHidd_Type_Any          -1      /* match any type */

#define vHidd_Type_Root         0       /* hiddclass */
#define vHidd_Type_Config       1       /* configuration plugins */
#define vHidd_Type_Timer        2       /* clocks and alarms */

/* Values for the aHidd_Subtype Tag */
#define vHidd_Subtype_Any       -1      /* match any subtype */
#define vHidd_Subtype_Root      0       /* main class of a type */

/* Values for the aHidd_Locking tag */
#define vHidd_LockShared        0
#define vHidd_LockExclusive     1
#define vHidd_Try               0x80    /* Flag */

/* Values for aHidd_Status tag */
#define vHidd_StatusUnknown     -1

/* Error codes defined for the HIDD */
enum {
    HIDDE_NotInList,            /* HIDD wasn't in a list */
};

enum {
    moHidd_Class_Get,           /* Get a value from a Class */
    moHidd_Class_MGet,          /* Get a number of values from a Class */
    moHidd_BeginIO,             /* Send a device like command */
    moHidd_AbortIO,             /* Abort a device like command */

    moHidd_LoadConfigPlugin,    /* HIDDT_Config M ( hmPlugin *) */
    moHidd_Lock,                /* Lock a HIDD */
    moHidd_Unlock,              /* UnLock a HIDD */
    moHidd_AddHIDD,             /* Add a subclass HIDD */
    moHidd_RemoveHIDD,  /* Remove a subclass HIDD */
    moHidd_FindHIDD             /* Find a suitable HIDD */
};


/* Used for HIDDM_BeginIO, HIDDM_AbortIO */
typedef struct hmIO
{
    STACKED ULONG               MethodID;
    STACKED struct IORequest   *hmi_ioRequest;
} hmIO;

#if 0
/* Used for HIDDM_LoadConfigPlugin */
typedef struct hmPlugin
{
    STACKED ULONG               MethodID;
    STACKED IPTR                hmp_PluginData;
} hmPlugin;
#endif

/* Combined structure for HIDDM_Lock, HIDDM_Unlock */
typedef struct hmLock
{
    STACKED ULONG               MethodID;
    STACKED ULONG               hml_LockMode;
    STACKED IPTR                hml_LockData;
} hmLock;

/* Used for HIDDM_AddHidd, HIDDM_RemoveHidd */
typedef struct hmAdd
{
    STACKED ULONG               MethodID;
    STACKED APTR                *hma_Class;
} hmAdd;

/* Used for HIDDM_FindHIDD */
typedef struct hmFind
{
    STACKED ULONG               MethodID;
    STACKED UWORD               hmf_Type;       /* Use vHidd_Type_Any to match all */
    STACKED UWORD               hmf_Subtype;    /* Use vHidd_Subtype_Any to match all */
} hmFind;

#endif /* HIDD_HIDD_H */
