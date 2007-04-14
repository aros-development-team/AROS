#ifndef _MUI_CLASSES_ICONLIST_H
#define _MUI_CLASSES_ICONLIST_H

/*
    Copyright  2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <stdarg.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <intuition/intuitionbase.h>
#include <intuition/classusr.h>
#include <clib/alib_protos.h>
#include <utility/utility.h>
#include <dos/dosextens.h>
#include <libraries/mui.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/muimaster.h>
#include <proto/intuition.h>

/****************************************************************************/
/*** Name *******************************************************************/
#define MUIC_IconList "IconList.mui"

/*** Identifier base ********************************************************/
#define MUIB_IconList                   (MUIB_ZUNE | 0x00004200)

/*** Methods ****************************************************************/
#define MUIM_IconList_Clear             (MUIB_IconList | 0x00000000) /* Zune: V1 */
#define MUIM_IconList_Update            (MUIB_IconList | 0x00000001) /* Zune: V1 */
#define MUIM_IconList_CreateEntry       (MUIB_IconList | 0x00000010) /* Zune: V1 returns 0 For Failure or (struct IconEntry *) */
#define MUIM_IconList_DestroyEntry      (MUIB_IconList | 0x00000011)
#define MUIM_IconList_SelectAll         (MUIB_IconList | 0x00000020) /* Zune: V1 */
#define MUIM_IconList_NextSelected      (MUIB_IconList | 0x00000021) /* Zune: V1 */
#define MUIM_IconList_UnselectAll       (MUIB_IconList | 0x00000022) /* Zune: V1 */
#define MUIM_IconList_Sort              (MUIB_IconList | 0x00000031) /* Zune: V1 */
#define MUIM_IconList_PositionIcons     (MUIB_IconList | 0x00000032) /* Zune: V1 */

struct MUIP_IconList_Clear              {ULONG MethodID;};
struct MUIP_IconList_Update             {ULONG MethodID;};
struct MUIP_IconList_CreateEntry        {ULONG MethodID; char *filename; char *label; struct FileInfoBlock *fib; struct DiskObject *icon_dob;};/* void *udata; More file attrs to add };*/
struct MUIP_IconList_DestroyEntry       {ULONG MethodID; struct IconEntry *icon;};
struct MUIP_IconList_NextSelected       {ULONG MethodID; struct IconList_Entry **entry;}; /* *entry maybe MUIV_IconList_NextSelected_Start, *entry is MUIV_IconList_NextSelected_End if no more entries are selected */
struct MUIP_IconList_Sort               {ULONG MethodID;};
struct MUIP_IconList_PositionIcons      {ULONG MethodID;};

#define MUIV_IconList_NextSelected_Start 0
#define MUIV_IconList_NextSelected_End   0

/*** Attributes *************************************************************/
#define MUIA_IconList_DoubleClick               (MUIB_IconList | 0x00000000) /* Zune: V1 ..G BOOL                      */
#define MUIA_IconList_Left                      (MUIB_IconList | 0x00000001) /* Zune: V1 ..G LONG                      */
#define MUIA_IconList_Top                       (MUIB_IconList | 0x00000002) /* Zune: V1 ..G LONG                      */
#define MUIA_IconList_Width                     (MUIB_IconList | 0x00000003) /* Zune: V1 ..G LONG                      */
#define MUIA_IconList_Height                    (MUIB_IconList | 0x00000004) /* Zune: V1 ..G LONG                      */
#define MUIA_IconList_IconsDropped              (MUIB_IconList | 0x00000005) /* Zune: V1 ..G (struct IconList_Entry *) */
#define MUIA_IconList_Clicked                   (MUIB_IconList | 0x00000006) /* Zune: V1 ..G (struct IconList_Click *) */
#define MUIA_IconList_IconsMoved                (MUIB_IconList | 0x00000007) /* Zune: V1 ..G (struct IconList_Entry *) */
#define MUIA_IconList_AppWindowDrop             (MUIB_IconList | 0x00000008) /* Zune: V1 ..G (struct IconList_Entry *) */

#define MUIA_IconList_DisplayFlags              (MUIB_IconList | 0x00000020) /* Zune: V1 ISG ULONG                     */
#define MUIA_IconList_SortFlags                 (MUIB_IconList | 0x00000021) /* Zune: V1 ISG ULONG                     */

#define MUIA_IconList_ListMode                  (MUIB_IconList | 0x00000030) /* Zune: V1 ISG UBYTE */
#define MUIA_IconList_TextMode                  (MUIB_IconList | 0x00000031) /* Zune: V1 ISG UBYTE */
#define MUIA_IconList_TextMaxLen                (MUIB_IconList | 0x00000032) /* Zune: V1 ISG ULONG */

#define MUIA_IconList_Rastport                  (MUIB_IconList | 0x000000FF) /* Zune: V1 ISG (struct RastPort *)       */

/* used by MUIM_IconList_NextSelected */
struct IconList_Entry
{
    char *filename;  /* The absolute filename of the file which the icons represents (means without the */
    char *label;     /* The label which is displayed (often FilePart(filename)) */
    LONG type;
    void *udata;     /* userdate given at MUIM_IconList_CreateEntry */
};

struct IconList_Click
{
    int shift; /* TRUE for shift click */
    struct IconList_Entry *entry; /* might be NULL */
};

struct IconList_Drop
{
    IPTR   *source_iconlistobj;              /* iconlist obj */
    IPTR   *destination_iconlistobj;         /* iconlist obj */
    unsigned char destination_string[1024];  /* destination path */
};

struct IconEntry
{
    struct Node                   ile_Node;

    struct IconList_Entry         ile_IconListEntry;

    struct DiskObject             *ile_DiskObj;                           /* The icons disk objects */
    struct FileInfoBlock          ile_FileInfoBlock;

    LONG                          ile_IconX,
					              ile_IconY;
    ULONG                         ile_IconWidth,
							      ile_IconHeight,
                                  ile_AreaWidth,
	                              ile_AreaHeight;                     /* <- includes textwidth and everything */

    ULONG                         ile_Flags;

    UBYTE   	    	          *ile_TxtBuf_DATE;
    UBYTE   	    	          *ile_TxtBuf_TIME;
    UBYTE   	    	          *ile_TxtBuf_SIZE;
    UBYTE   	    	          *ile_TxtBuf_PROT;
};

/****************************************************************************/
/* Internal Icon state flags */
#define ICONENTRY_FLAG_SELECTED      (1<<1)		/* icon selected state         */
#define ICONENTRY_FLAG_FOCUS         (1<<2)		/* icon input focus state      */
#define ICONENTRY_FLAG_VISIBLE		 (1<<3)		/* icon should be drawn        */
#define ICONENTRY_FLAG_HASICON	     (1<<4)		/* icon has an '.info' file    */

/* iconlist rendering control flags */
/* SORTFLAGS - a value of zero sets: sort by name + drawers at top */
#define ICONLIST_SORT_DRAWERS_MIXED  (1<<0)		/*mix folders and files when sorting*/
#define ICONLIST_SORT_DRAWERS_LAST   (1<<1)		/*ignored if mixed is set*/
#define ICONLIST_SORT_REVERSE		 (1<<2)		/*reverse sort direction*/
#define ICONLIST_SORT_BY_DATE	     (1<<3)		/*both date and size = sort by type*/
#define ICONLIST_SORT_BY_SIZE	     (1<<4)		/*neither = sort by name*/

/* DISPLAYFLAGS */
#define ICONLIST_DISP_SHOWHIDDEN     (1<<0)		/* show system "hidden" files */
#define ICONLIST_DISP_SHOWINFO	     (1<<1)		/* only show icon(s) which have *.info files */

#define ICONLIST_DISP_VERTICAL	     (1<<7)		/* tile icons vertically */
#define ICONLIST_DISP_NOICONS	     (1<<8)		/* name only mode*/
#define ICONLIST_DISP_DETAILS	     (1<<9)		/* name=details mode, icon=?? */



/****************************************************************************/
/*** Name *******************************************************************/
#define MUIC_IconDrawerList         "IconDrawerList.mui"

/*** Identifier base ********************************************************/
#define MUIB_IconDrawerList         (MUIB_ZUNE | 0x00004300)  

/*** Attributes *************************************************************/
#define MUIA_IconDrawerList_Drawer  (MUIB_IconDrawerList | 0x00000000) /* Zune: V1  isg LONG     */



/****************************************************************************/
/*** Name *******************************************************************/
#define MUIC_IconVolumeList         "IconVolumeList.mui"

/*** Identifier base ********************************************************/
#define MUIB_IconVolumeList         (MUIB_ZUNE | 0x00004400)  



extern const struct __MUIBuiltinClass _MUI_IconList_desc; /* PRIV */
extern const struct __MUIBuiltinClass _MUI_IconDrawerList_desc; /* PRIV */
extern const struct __MUIBuiltinClass _MUI_IconVolumeList_desc; /* PRIV */


#endif /* _MUI_CLASSES_ICONLIST_H */
