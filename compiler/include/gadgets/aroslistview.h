#ifndef GADGETS_AROSLISTVIEW_H
#define GADGETS_AROSLISTVIEW_H

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

#define AROSLISTVIEWCLASS "listview.aros"
#define AROSLISTVIEWNAME  "Gadgets/aroslistview.gadget"

#define AROSLV_TAGBASE (TAG_USER + 0x05120000)

/* Attribute IDs */

#define AROSA_Listview_DisplayHook	AROSLV_TAGBASE + 0 /* [I] */
#define AROSA_Listview_RenderHook	AROSLV_TAGBASE + 1 /* [IS] */
#define AROSA_Listview_HorSpacing	AROSLV_TAGBASE + 2 /* [ISG] */
#define AROSA_Listview_VertSpacing	AROSLV_TAGBASE + 3 /* [ISG] */
#define AROSA_Listview_List		AROSLV_TAGBASE + 4 /* [IS] */


#define AROSA_Listview_MaxColumns	AROSLV_TAGBASE + 5 /* [I] The displayhook limit */


/* Formatstring for the coulumns. If none is specified the default will be BACKGROUNDPEN,
 Leftalign and equal weight */

#define AROSA_Listview_Format 		AROSLV_TAGBASE + 6  /* [IS]	*/
#define AROSA_Listview_First 		AROSLV_TAGBASE + 7  /* [ISGUN]	*/
#define AROSA_Listview_MultiSelect	AROSLV_TAGBASE + 8  /* [IS]	*/
#define AROSA_Listview_DoubleClick	AROSLV_TAGBASE + 9  /* [G]	*/
#define AROSA_Listview_Visible		AROSLV_TAGBASE + 10 /* [N]	*/
#define AROSA_Listview_Total		AROSLV_TAGBASE + 11 /* [N]	*/
#define AROSA_Listview_FrontPen		AROSLV_TAGBASE + 12 /* [IS]	*/
#define AROSA_Listview_BackPen		AROSLV_TAGBASE + 13 /* [IS]	*/


/* These methods are merely aliases for the List ones.
 * These are needed because I must include that damn
 * GadgetInfo structure as the 2nd argument.
 */
 
#define AROSM_Listview_Insert	    AROSM_List_Insert
#define AROSM_Listview_InsertSingle AROSM_List_InsertSingle
#define AROSM_Listview_Remove	    AROSM_List_Remove

#define AROSM_Listview_DoubleClick	AROSLV_TAGBASE + 50
#define AROSM_Listview_SingleClick	AROSLV_TAGBASE + 51



struct AROSP_Listview_Insert
{
    STACKED ULONG 	MethodID;
    STACKED struct GadgetInfo *GInfo;
    STACKED APTR	*ItemArray;
    STACKED LONG	Position;
};

struct AROSP_Listview_InsertSingle
{
    STACKED ULONG	MethodID;
    STACKED struct GadgetInfo *GInfo;
    STACKED APTR	Item;
    STACKED LONG	Position;
};

struct AROSP_Listview_Remove
{
    STACKED ULONG		MethodID;
    STACKED struct GadgetInfo 	*GInfo;
    STACKED LONG		Position;
};

struct AROSP_Listview_DoubleClick
{
    STACKED ULONG		MethodID;
    STACKED struct GadgetInfo 	*GInfo;
    STACKED LONG		Position;
};

struct AROSP_Listview_SingleClick
{
    STACKED ULONG		MethodID;
    STACKED struct GadgetInfo 	*GInfo;
    STACKED LONG		Position;
};


/* Method IDs */


#endif /* GADGETS_AROSLISTVIEW_H */
