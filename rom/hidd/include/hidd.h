#ifndef HIDD_HIDD_H
#define HIDD_HIDD_H

/*
    Copyright (C) 1997 AROS - The Amiga Replacement OS
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
#define HIDDCLASS		"hiddclass"

#ifndef __typedef_HIDD
#   define __typedef_HIDD
    typedef APTR HIDD;
#endif

/*
    Tags for the root HIDD class "hiddclass".
    See the HIDD documentation for information on their use.
 */

#define HIDDA_Base		(TAG_USER + 0x800000)
#define HIDDA_Type		(HIDDA_Base + 1)    /* --G-- ULONG */
#define HIDDA_SubType		(HIDDA_Base + 2)    /* --G-- ULONG */
#define HIDDA_Producer		(HIDDA_Base + 3)    /* --G-- ULONG */
#define HIDDA_Name		(HIDDA_Base + 4)    /* --G-- STRPTR */
#define HIDDA_HardwareName	(HIDDA_Base + 5)    /* --G-- STRPTR */
#define HIDDA_Active		(HIDDA_Base + 6)    /* ISGNU BOOL */
#define HIDDA_Status		(HIDDA_Base + 7)    /* --GNU ULONG */
#define HIDDA_ErrorCode 	(HIDDA_Base + 8)    /* --G-- ULONG */
#define HIDDA_Locking		(HIDDA_Base + 9)    /* --G-- ULONG */

/*
    This flag is set on private attributes which external code should just
    ignore. To define such tags, you can offset from HIDDA_PrivateBase.
    Note that these tags are HIDD-specific; you should not try to apply
    them on another HIDD than the one from which you got them.
*/
#define HIDDV_PrivateAttribute	0x40000000
#define HIDDA_PrivateBase	(HIDDA_Base | HIDDV_PrivateAttribute)

/* Values for the HIDD_Type Tag */
#define HIDDV_Root		0

/* Values for the HIDDA_Locking tag */
#define HIDDV_LockShared	0
#define HIDDV_LockExclusive	1
#define HIDDV_Try		0x80	/* Flag */

/* Values for HIDDA_Status tag */
#define HIDDV_StatusUnknown	-1

/* Methods for all HIDD's */
#define HIDDM_Base		0x80000
#define HIDDM_Class_Get 	(HIDDM_Base + 1) /* LONG M (struct opGet *)      */
#define HIDDM_Class_MGet	(HIDDM_Base + 2) /* LONG M (struct op??? *)      */
#define HIDDM_BeginIO		(HIDDM_Base + 3) /* LONG M ( hmIO *)             */
#define HIDDM_AbortIO		(HIDDM_Base + 4) /* LONG M ( hmIO *)             */
#define HIDDM_LoadConfigPlugin	(HIDDM_Base + 5) /* HIDDT_Config M ( hmPlugin *) */
#define HIDDM_Lock		(HIDDM_Base + 6) /* IPTR M ( hmLock *)           */
#define HIDDM_Unlock		(HIDDM_Base + 7) /* void M ( hmLock *)           */
#define HIDDM_AddHIDD		(HIDDM_Base + 8) /* BOOL M ( hmAdd *)            */
#define HIDDM_RemoveHIDD	(HIDDM_Base + 9) /* void M ( hmAdd *)            */

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

/* Used for HIDDM_LoadConfigPlugin */
typedef struct hmPlugin
{
    STACKULONG		MethodID;
    STACKIPTR		hmp_PluginData;
} hmPlugin;

/* Combined structure for HIDDM_Lock, HIDDM_Unlock */
typedef struct hmLock
{
    STACKULONG		MethodID;
    STACKULONG		hml_LockMode;
    STACKIPTR		hml_LockData;
} hmLock;

typedef struct hmAdd
{
    STACKULONG		MethodID;
    Class	       *hma_Class;
} hmAdd;

#endif /* HIDD_HIDD_H */
