#ifndef HIDD_HIDD_H
#define HIDD_HIDD_H

/*
    Copyright (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Main HIDD Include File
    Lang: english
*/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif
#ifndef INTUITION_CLASSUSR_H
#include <intuition/classusr.h>
#endif

/* ---------------------------------------------------------------------
    Main HIDD Class Interface
   --------------------------------------------------------------------- */

/* The name of the topmost HIDD class */
#define HIDDCLASS		"hiddclass"

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
#define HIDDA_ErrorCode		(HIDDA_Base + 8)    /* --G-- ULONG */
#define HIDDA_Locking		(HIDDA_Base + 9)    /* --G-- ULONG */

/* Values for the HIDD_Type Tag */
#define HIDDV_Root		0

/* Values for the HIDDA_Locking tag */
#define HIDDV_LockShared	0
#define HIDDV_LockExclusive	1
#define HIDDV_Try		0x80	/* Flag */

/* Values for HIDDA_Status tag */
#define HIDDV_StatusUnknown	-1

/* Methods for all HIDD's */
#define HIDDM_Root_Base		0x80000
#define HIDDM_Class_Get		0x80001	/* LONG M (struct opGet *) 	*/
#define HIDDM_Class_MGet	0x80002 /* LONG M (struct op??? *) 	*/ 
#define HIDDM_BeginIO		0x80003	/* LONG M ( hmIO *) 		*/
#define HIDDM_AbortIO		0x80004	/* LONG M ( hmIO *)		*/
#define HIDDM_LoadConfigPlugin	0x80005	/* HIDDT_Config M ( hmPlugin *) */
#define HIDDM_Lock		0x80006	/* IPTR M ( hmLock *)		*/
#define HIDDM_Unlock		0x80007 /* void M ( hmLock *)		*/
#define HIDDM_AddHIDD		0x80008 /* BOOL M ( hmAdd *)		*/
#define HIDDM_RemoveHIDD	0x80009 /* void M ( hmAdd *)		*/

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
