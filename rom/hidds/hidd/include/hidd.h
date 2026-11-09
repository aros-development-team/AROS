#ifndef HIDD_HIDD_H
#define HIDD_HIDD_H

/*
    Copyright © 1995-2026, The AROS Development Team. All rights reserved.
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
#ifndef UTILITY_HOOKS_H
#   include <utility/hooks.h>
#endif
#ifndef OOP_OOP_H
#   include <oop/oop.h>
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

#define IS_HIDD_ATTR(attr, idx) ((idx = attr - HiddAttrBase) < num_Hidd_Attrs)
#define IS_HW_ATTR(attr, idx) ((idx = attr - HWAttrBase) <  num_HW_Attrs)

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

/*
 * Object relations (moHidd_AddRelation and friends).
 *
 * A relation is declared once, by either of the two objects, and can then be
 * queried from both. The object at the other end sees the inverse type, so
 * if a driver declares vHidd_Relation_Parent to its subsystem, the subsystem
 * reports the driver as vHidd_Relation_Child.
 */
#define vHidd_RelationF_Inverse     0x80000000  /* The relation read from its other end */
#define vHidd_RelationF_Symmetric   0x40000000  /* The relation reads the same from both ends */

#define vHidd_Relation_Any          0           /* Match any type (queries only) */

#define vHidd_Relation_Child        1           /* An object I contain or manage */
#define vHidd_Relation_Parent       (vHidd_Relation_Child      | vHidd_RelationF_Inverse)
#define vHidd_Relation_Driver       2           /* A driver object operating me */
#define vHidd_Relation_Device       (vHidd_Relation_Driver     | vHidd_RelationF_Inverse)
#define vHidd_Relation_UsedBy       3           /* An object depending on me */
#define vHidd_Relation_Uses         (vHidd_Relation_UsedBy     | vHidd_RelationF_Inverse)
#define vHidd_Relation_Exposes      4           /* An object I publish (telemetry, a display, ...) */
#define vHidd_Relation_ExposedBy    (vHidd_Relation_Exposes    | vHidd_RelationF_Inverse)
#define vHidd_Relation_FirmwareOf   5           /* The hardware I am the firmware description of */
#define vHidd_Relation_Firmware     (vHidd_Relation_FirmwareOf | vHidd_RelationF_Inverse)
#define vHidd_Relation_Powers       6           /* An object I supply with power */
#define vHidd_Relation_PoweredBy    (vHidd_Relation_Powers     | vHidd_RelationF_Inverse)
#define vHidd_Relation_Monitors     7           /* The hardware my readings are about */
#define vHidd_Relation_MonitoredBy  (vHidd_Relation_Monitors   | vHidd_RelationF_Inverse)
#define vHidd_Relation_Companion    (8 | vHidd_RelationF_Symmetric) /* Another function of the same device */

#define vHidd_Relation_User         0x1000      /* First type free for private use.
                                                   The two flags above apply here too. */

/* Tags for moHidd_AddRelation */
#define tHidd_Relation_BASE         TAG_USER
#define tHidd_Relation_Path         (tHidd_Relation_BASE + 1)   /* (CONST_STRPTR) Name the target by device path */
#define tHidd_Relation_Label        (tHidd_Relation_BASE + 2)   /* (CONST_STRPTR) What the relation is, for the user */

/* One relation of an object, as passed to the moHidd_EnumRelations hook */
struct HIDD_Relation
{
    ULONG         hr_Type;      /* vHidd_Relation_#? as seen from the object queried */
    OOP_Object   *hr_Object;    /* The related object, NULL if it is not present     */
    CONST_STRPTR  hr_Path;      /* Its device path, NULL if it has none              */
    CONST_STRPTR  hr_Label;     /* Description given by the declarer, or NULL        */
};

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
