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

/* ---------------------------------------------------------------------
    Main HIDD Class Interface
   --------------------------------------------------------------------- */

/* The name of the topmost HIDD class */
#define CLID_Hidd "hiddclass"
#define IID_Hidd "I_Hidd"

#ifndef __typedef_HIDD
#   define __typedef_HIDD
    typedef APTR HIDD;
#endif

/*
    Attributes for the root HIDD class "hiddclass".
    See the HIDD documentation for information on their use.
 */
enum {
    HIDDAIDX_Type = 0, 		/* [..G] (UWORD) Major type of HIDD */
    HIDDAIDX_SubType,		/* [..G] (UWORD) Sub-type of HIDD */
    HIDDAIDX_Producer,		/* [..G] (ULONG) Product Developer */
    HIDDAIDX_Name, 		/* [..G] (STRPTR) Name of HIDD */
    HIDDAIDX_HardwareName, 	/* [..G] (STRPTR) Hardware description */
    HIDDAIDX_Active,		/* [ISG] (BOOL) Current active status */
    HIDDAIDX_Status,		/* [..G] (ULONG) Status change */
    HIDDAIDX_ErrorCode,		/* [..G] (ULONG) Error code */
    HIDDAIDX_Locking,		/* [..G] (UBYTE) Type of locking supported */
    
    NUM_A_HIDD
};

#define HIDDA_Type		(__HIDD_AttrBase + HIDDAIDX_Type	)
#define HIDDA_SubType		(__HIDD_AttrBase + HIDDAIDX_SubType	)
#define HIDDA_Producer		(__HIDD_AttrBase + HIDDAIDX_Producer	)
#define HIDDA_Name		(__HIDD_AttrBase + HIDDAIDX_Name	)
#define HIDDA_HardwareName	(__HIDD_AttrBase + HIDDAIDX_HardwareName)
#define HIDDA_Active		(__HIDD_AttrBase + HIDDAIDX_Active	)
#define HIDDA_Status		(__HIDD_AttrBase + HIDDAIDX_Status	)
#define HIDDA_ErrorCode		(__HIDD_AttrBase + HIDDAIDX_ErrorCode	)
#define HIDDA_Locking		(__HIDD_AttrBase + HIDDAIDX_Locking	)


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
    HIDDMIDX_Class_Get,		/* Get a value from a Class */
    HIDDMIDX_Class_MGet,		/* Get a number of values from a Class */
    HIDDMIDX_BeginIO,		/* Send a device like command */
    HIDDMIDX_AbortIO,		/* Abort a device like command */

    HIDDMIDX_LoadConfigPlugin,	/* HIDDT_Config M ( hmPlugin *) */
    HIDDMIDX_Lock, 		/* Lock a HIDD */
    HIDDMIDX_Unlock,		/* UnLock a HIDD */
    HIDDMIDX_AddHIDD,		/* Add a subclass HIDD */
    HIDDMIDX_RemoveHIDD,	/* Remove a subclass HIDD */
    HIDDMIDX_FindHIDD		/* Find a suitable HIDD */
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
    APTR 		*hma_Class;
} hmAdd;

/* Used for HIDDM_FindHIDD */
typedef struct hmFind
{
    STACKULONG		MethodID;
    STACKUWORD		hmf_Type;	/* Use HIDDV_Type_Any to match all */
    STACKUWORD		hmf_Subtype;	/* Use HIDDV_Subtype_Any to match all */
} hmFind;

#endif /* HIDD_HIDD_H */
