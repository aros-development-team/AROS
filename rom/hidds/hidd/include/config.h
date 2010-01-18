#ifndef HIDD_CONFIG_H
#define HIDD_CONFIG_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Configuration Plugin Definitions.
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
    HIDD Configuration Plugin 
   --------------------------------------------------------------------- */
#define HIDDA_CPBase		(HIDDA_Base + 0x01000)
#define HIDDA_HasOwnGUI		(HIDDA_CPBase + 1)  /* --G-- BOOL */
#define HIDDA_ShowGUI		(HIDDA_CPBase + 2)  /* ISG-- BOOL */
#define HIDDA_UseScreen		(HIDDA_CPBase + 3)  /* ISG-- struct Screen * */
#define HIDDA_AppMsgPort	(HIDDA_CPBase + 4)  /* ISG-- struct MsgPort * */

/* Methods */
#define HIDDM_ConfigBase	(HIDDM_Base + 0x100)
#define HIDDM_QueryConfig	(HIDDM_ConfigBase + 1)	/* ULONG M (???) */
#define HIDDM_FreeConfig	(HIDDM_ConfigBase + 2)	/* ULONG M ( hmFreeData *) */
#define HIDDM_QueryModeList	(HIDDM_ConfigBase + 3)	/* ULONG M (???) */
#define HIDDM_FreeModeList	(HIDDM_ConfigBase + 4)	/* ULONG M ( hmFreeData *) */
#define HIDDM_Apply		(HIDDM_ConfigBase + 5)	/* ULONG M (???) */
#define HIDDM_ValueToString	(HIDDM_ConfigBase + 6)	/* ULONG M ( hmValueToString *) */
#define HIDDM_StringToValue	(HIDDM_ConfigBase + 7)	/* ULONG M ( hmStringToValue *) */
#define HIDDM_GetTagName	(HIDDM_ConfigBase + 8)	/* ULONG M ( hmValueToString *) */

struct hmFreeData 
{
    STACKED ULONG		Method;
    STACKED struct TagItem     *AttrList;
};

struct hmValueToString
{
    STACKED ULONG		Method;
    STACKED Tag			tag;
    STACKED IPTR		value;		/* NULL for HIDDM_GetTagName */
};

struct hmStringToValue
{	
    STACKED ULONG		Method;
    STACKED Tag			tag;
    STACKED STRPTR		string;
};

#endif /* HIDD_CONFIG_H */
