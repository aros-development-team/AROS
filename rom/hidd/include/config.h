#ifndef HIDD_CONFIG_H
#define HIDD_CONFIG_H

/*
    Copyright (C) 1997 AROS - The Amiga Replacement OS
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
#define HIDDM_QueryConfig	0x8010	/* Msg */
#define HIDDM_FreeConfig	0x8011  /* struct hmFreeData * */
#define HIDDM_QueryModeList	0x8012	/* Msg */
#define HIDDM_FreeModeList	0x8013	/* struct hmFreeData * */
#define HIDDM_Apply		0x8014	/* Msg */
#define HIDDM_ValueToString	0x8015	/* struct hmValueToString * */
#define HIDDM_StringToValue	0x8016	/* struct hmStringToValue * */
#define HIDDM_GetTagName	0x8017	/* struct hmValueToString * */

struct hmFreeData 
{
    STACKULONG		Method;
    struct TagItem     *AttrList;
};

struct hmValueToString
{
    STACKULONG		Method;
    Tag			tag;
    STACKIPTR		value;		/* NULL for HIDDM_GetTagName */
};

struct hmStringToValue
{	
    STACKULONG		Method;
    Tag			tag;
    STRPTR		string;
};

#endif /* HIDD_CONFIG_H */
