#ifndef HIDD_HIDD_H
#define HIDD_HIDD_H

/*
    Copyright (C) 1997-1998 AROS - The Amiga Replacement OS
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
#ifndef INTUITION_CLASSES_H
#   include <intuition/classes.h>
#endif
#ifndef INTUITION_CLASSUSR_H
#   include <intuition/classusr.h>
#endif

/* ---------------------------------------------------------------------
    Main HIDD Class Interface
   --------------------------------------------------------------------- */

/* The name of the topmost HIDD class */
#define HIDDCLASS "hiddclass"

#ifndef __typedef_HIDD
#   define __typedef_HIDD
    typedef APTR HIDD;
#endif

/*
    Attributes for the root HIDD class "hiddclass".
    See the HIDD documentation for information on their use.
 */
enum {
    HIDDA_Base = (TAG_USER + 0x800000),
    HIDDA_Type, 		/* [..G] (UWORD) Major type of HIDD */
    HIDDA_SubType,		/* [..G] (UWORD) Sub-type of HIDD */
    HIDDA_Producer,		/* [..G] (ULONG) Product Developer */
    HIDDA_Name, 		/* [..G] (STRPTR) Name of HIDD */
    HIDDA_HardwareName, 	/* [..G] (STRPTR) Hardware description */
    HIDDA_Active,		/* [ISG] (BOOL) Current active status */
    HIDDA_Status,		/* [..G] (ULONG) Status change */
    HIDDA_ErrorCode,		/* [..G] (ULONG) Error code */
    HIDDA_Locking		/* [..G] (UBYTE) Type of locking supported */
};

/*
    This flag is set on private attributes which external code should just
    ignore. To define such tags, you can offset from HIDDA_PrivateBase.
    Note that these tags are HIDD-specific; you should not try to apply
    them on another HIDD than the one from which you got them.
*/
#define HIDDV_PrivateAttribute	0x40000000
#define HIDDA_PrivateBase	(HIDDA_Base | HIDDV_PrivateAttribute)

/* Values for the HIDD_Type Tag */
#define HIDDV_Type_Any		-1	/* match any type */

#define HIDDV_Type_Root 	0	/* hiddclass */
#define HIDDV_Type_Config	1	/* configuration plugins */
#define HIDDV_Type_Timer	2	/* clocks and alarms */

/* Values for the HIDDA_Subtype Tag */
#define HIDDV_Subtype_Any	-1	/* match any subtype */
#define HIDDV_Subtype_Root	0	/* main class of a type */

/* Values for the HIDDA_Locking tag */
#define HIDDV_LockShared	0
#define HIDDV_LockExclusive	1
#define HIDDV_Try		0x80	/* Flag */

/* Values for HIDDA_Status tag */
#define HIDDV_StatusUnknown	-1

/* Error codes defined for the HIDD */
enum {
    HIDDE_NotInList,		/* HIDD wasn't in a list */
};

enum {
    HIDDM_Base = 0x80000,
    HIDDM_Class_Get,		/* Get a value from a Class */
    HIDDM_Class_MGet,		/* Get a number of values from a Class */
    HIDDM_BeginIO,		/* Send a device like command */
    HIDDM_AbortIO,		/* Abort a device like command */

    HIDDM_LoadConfigPlugin,	/* HIDDT_Config M ( hmPlugin *) */
    HIDDM_Lock, 		/* Lock a HIDD */
    HIDDM_Unlock,		/* UnLock a HIDD */
    HIDDM_AddHIDD,		/* Add a subclass HIDD */
    HIDDM_RemoveHIDD,		/* Remove a subclass HIDD */
    HIDDM_FindHIDD		/* Find a suitable HIDD */
};

/*
    This flag is set on uncommon methods. Uncommon methods are methods
    which are really uncommon, ie. which are used by a specific HIDD.
    Any method which is shared by a set of HIDDs should not have this
    flag set (unless the method is really uncommon). Examples might be
    methods which access a unique feature of the hardware which are not
    available anywhere else (eg. CopperLists on the Amiga). Something
    like 3D methods (ie. methods which are introduced by new hardware
    but which seem to be common in the future) shouldn't have this
    flag set.
*/
#define HIDDV_UncommonMethod	    0x80000000
#define HIDDM_UncommonMethodBase    (HIDDM_Base | HIDDV_UncommonMethod)


/* Used for HIDDM_BeginIO, HIDDM_AbortIO */
typedef struct hmIO
{
    STACKULONG		MethodID;
    struct IORequest   *hmi_ioRequest;
} hmIO;

#if 0
/* Used for HIDDM_LoadConfigPlugin */
typedef struct hmPlugin
{
    STACKULONG		MethodID;
    STACKIPTR		hmp_PluginData;
} hmPlugin;
#endif

/* Combined structure for HIDDM_Lock, HIDDM_Unlock */
typedef struct hmLock
{
    STACKULONG		MethodID;
    STACKULONG		hml_LockMode;
    STACKIPTR		hml_LockData;
} hmLock;

/* Used for HIDDM_AddHidd, HIDDM_RemoveHidd */
typedef struct hmAdd
{
    STACKULONG		MethodID;
    Class		*hma_Class;
} hmAdd;

/* Used for HIDDM_FindHIDD */
typedef struct hmFind
{
    STACKULONG		MethodID;
    STACKUWORD		hmf_Type;	/* Use HIDDV_Type_Any to match all */
    STACKUWORD		hmf_Subtype;	/* Use HIDDV_Subtype_Any to match all */
} hmFind;

#endif /* HIDD_HIDD_H */
