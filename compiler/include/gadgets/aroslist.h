#ifndef GADGETS_AROSLIST_H
#define GADGETS_AROSLIST_H

/*
    (C) 1995-97 AROS - The Amiga Replacement OS
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

#define AROSLISTCLASS "list.aros"

#define AROSLIST_TAGBASE 10000L

/* Attribute IDs */
#define AROSA_List_SourceArray		AROSLIST_TAGBASE + 1
#define AROSA_List_Entries		AROSLIST_TAGBASE + 2
#define AROSA_List_ConstructHook	AROSLIST_TAGBASE + 3
#define AROSA_List_DestructHook		AROSLIST_TAGBASE + 4
#define AROSA_List_PoolPuddleSize	AROSLIST_TAGBASE + 5
#define AROSA_List_PoolThreshSize	AROSLIST_TAGBASE + 6
#define AROSA_List_Pool			AROSLIST_TAGBASE + 7


/* Method IDs */
#define AROSM_List_Insert		AROSLIST_TAGBASE + 50
#define AROSM_List_InsertSingle		AROSLIST_TAGBASE + 51
#define AROSM_List_Remove		AROSLIST_TAGBASE + 52
#define AROSM_List_GetEntry		AROSLIST_TAGBASE + 53

#define AROSV_List_Insert_Top		-1L
#define AROSV_List_Insert_Bottom	-2L


struct AROSP_List_Insert
{
    STACKULONG 	MethodID;
    APTR	*ItemArray;
    STACKLONG	Position;
};

struct AROSP_List_InsertSingle
{
    STACKULONG	MethodID;
    APTR	Item;
    STACKLONG	Position;
};

struct AROSP_List_Remove
{
    STACKULONG		MethodID;
    STACKLONG		Position;
};

#define AROSV_List_Remove_First -1L
#define AROSV_List_Remove_Last  -2L

struct AROSP_List_GetEntry
{
    STACKULONG	MethodID;
    STACKLONG	Position;
    APTR	*ItemPtr;
};

#endif /* GADGETS_AROSLISTVIEW_H */
