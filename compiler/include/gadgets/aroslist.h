#ifndef GADGETS_AROSLIST_H
#define GADGETS_AROSLIST_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: MethodIDs and AttrIDs for the AROS listview class.
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef UTILITY_TAGITEM_H
#   include <utility/tagitem.h>
#endif

#define AROSLISTCLASS	"list.aros"
#define AROSLISTNAME	"Gadgets/aroslist.class"

#define AROSLIST_TAGBASE (TAG_USER + 0x05110000)

/* Attribute IDs */
#define AROSA_List_SourceArray		AROSLIST_TAGBASE + 1
#define AROSA_List_Entries		AROSLIST_TAGBASE + 2
#define AROSA_List_ConstructHook	AROSLIST_TAGBASE + 3
#define AROSA_List_DestructHook		AROSLIST_TAGBASE + 4
#define AROSA_List_PoolPuddleSize	AROSLIST_TAGBASE + 5
#define AROSA_List_PoolThreshSize	AROSLIST_TAGBASE + 6
#define AROSA_List_Pool			AROSLIST_TAGBASE + 7
#define AROSA_List_Active		AROSLIST_TAGBASE + 8

/* Method IDs */
#define AROSM_List_Insert		AROSLIST_TAGBASE + 50
#define AROSM_List_InsertSingle		AROSLIST_TAGBASE + 51
#define AROSM_List_Remove		AROSLIST_TAGBASE + 52
#define AROSM_List_GetEntry		AROSLIST_TAGBASE + 53
#define AROSM_List_Clear		AROSLIST_TAGBASE + 54
#define AROSM_List_Sort			AROSLIST_TAGBASE + 55
#define AROSM_List_Select		AROSLIST_TAGBASE + 56
#define AROSM_List_NextSelected		AROSLIST_TAGBASE + 57

/* Special values */
#define AROSV_List_Insert_Top		-1L
#define AROSV_List_Insert_Bottom	-2L

#define AROSV_List_Active_None		-1L

/* pos */
#define AROSV_List_Select_All		-1L

/* seltype */
#define AROSV_List_Select_Off		-1L
#define AROSV_List_Select_On		-2L
#define AROSV_List_Select_Toggle	-3L
#define AROSV_List_Select_Ask		-4L

#define  AROSV_List_NextSelected_Start  -1L
#define  AROSV_List_NextSelected_End	-2L


/* Msg structs */

struct AROSP_List_Insert
{
    STACKED ULONG 	MethodID;
    STACKED APTR	*ItemArray;
    STACKED LONG	Position;
};

struct AROSP_List_InsertSingle
{
    STACKED ULONG	MethodID;
    STACKED APTR	Item;
    STACKED LONG	Position;
};

struct AROSP_List_Remove
{
    STACKED ULONG		MethodID;
    STACKED LONG		Position;
};

#define AROSV_List_Remove_First -1L
#define AROSV_List_Remove_Last  -2L

struct AROSP_List_GetEntry
{
    STACKED ULONG	MethodID;
    STACKED LONG	Position;
    STACKED APTR	*ItemPtr;
};



struct AROSP_List_Select
{
    STACKED ULONG	MethodID;
    STACKED LONG	Position;
    STACKED LONG	SelType;
    STACKED LONG	*State;
};

struct AROSP_List_NextSelected
{
    STACKED ULONG	MethodID;
    STACKED LONG	*Position;
};

#endif /* GADGETS_AROSLISTVIEW_H */
