#ifndef _MUI_CLASSES_ICONLIST_H
#define _MUI_CLASSES_ICONLIST_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/****************************************************************************/
/*** Name *******************************************************************/
#define MUIC_IconList "IconList.mui"

/*** Identifier base ********************************************************/
#define MUIB_IconList                   (MUIB_ZUNE | 0x00004200)

/*** Methods ****************************************************************/
#define MUIM_IconList_Clear             (MUIB_IconList | 0x00000000) /* Zune: V1 */
#define MUIM_IconList_Update            (MUIB_IconList | 0x00000001) /* Zune: V1 */
#define MUIM_IconList_Add               (MUIB_IconList | 0x00000002) /* Zune: V1 returns BOOL */
#define MUIM_IconList_NextSelected      (MUIB_IconList | 0x00000003) /* Zune: V1 */
#define MUIM_IconList_UnselectAll       (MUIB_IconList | 0x00000004) /* Zune: V1 */
#define MUIM_IconList_Sort              (MUIB_IconList | 0x00000005) /* Zune: V1 */
#define MUIM_IconList_GetSortBits       (MUIB_IconList | 0x00000006) /* Zune: V1 */
#define MUIM_IconList_SetSortBits       (MUIB_IconList | 0x00000007) /* Zune: V1 */
#define MUIM_IconList_PositionIcons     (MUIB_IconList | 0x00000008) /* Zune: V1 */

struct MUIP_IconList_Clear              {ULONG MethodID;};
struct MUIP_IconList_Update             {ULONG MethodID;};
struct MUIP_IconList_Add                {ULONG MethodID; char *filename; char *label; struct FileInfoBlock *fib;};/* void *udata; More file attrs to add };*/
struct MUIP_IconList_NextSelected       {ULONG MethodID; struct IconList_Entry **entry;}; /* *entry maybe MUIV_IconList_NextSelected_Start, *entry is MUIV_IconList_NextSelected_End if no more entries are selected */
struct MUIP_IconList_Sort               {ULONG MethodID;};
struct MUIP_IconList_SetSortBits        {ULONG MethodID; ULONG sort_bits; };
struct MUIP_IconList_GetSortBits        {ULONG MethodID;};
struct MUIP_IconList_PositionIcons      {ULONG MethodID;};

#define MUIV_IconList_NextSelected_Start 0
#define MUIV_IconList_NextSelected_End   0

/*** Attributes *************************************************************/
#define MUIA_IconList_DoubleClick       (MUIB_IconList | 0x00000000) /* Zune: V1 ..G BOOL */
#define MUIA_IconList_Left              (MUIB_IconList | 0x00000001) /* Zune: V1 ..G LONG */
#define MUIA_IconList_Top               (MUIB_IconList | 0x00000002) /* Zune: V1 ..G LONG */
#define MUIA_IconList_Width             (MUIB_IconList | 0x00000003) /* Zune: V1 ..G LONG */
#define MUIA_IconList_Height            (MUIB_IconList | 0x00000004) /* Zune: V1 ..G LONG */
#define MUIA_IconList_IconsDropped      (MUIB_IconList | 0x00000005) /* Zune: V1 ..G struct IconList_Entry * */
#define MUIA_IconList_Clicked           (MUIB_IconList | 0x00000006) /* Zune: V1 ..G struct IconList_Click * */
#define MUIA_IconList_IconsMoved        (MUIB_IconList | 0x00000007) /* Zune: V1 ..G struct IconList_Entry * */

/* used by MUIM_IconList_NextSelected */
struct IconList_Entry
{
    char *filename;  /* The absolute filename of the file which the icons represents (means without the */
    char *label;     /* The label which is displayed (often FilePart(filename)) */
    LONG type;
    void *udata;     /* userdate given at MUIM_IconList_Add */
};

struct IconList_Click
{
    int shift; /* TRUE for shift click */
    struct IconList_Entry *entry; /* might be NULL */
};

struct IconList_Drop
{
    IPTR   *source_iconlistobj;          /* iconlist obj */
    IPTR   *destination_iconlistobj;     /* iconlist obj */
};

/****************************************************************************/
/* iconlist sort bits - a value of zero sets: sort by name + drawers at top */

#define ICONLIST_SORT_DRAWERS_MIXED  (1<<0)		/*mix folders and files when sorting*/
#define ICONLIST_SORT_DRAWERS_LAST   (1<<1)		/*ignored if mixed is set*/
#define ICONLIST_SORT_REVERSE		 (1<<2)		/*reverse sort direction*/
#define ICONLIST_SORT_BY_DATE	     (1<<3)		/*both date and size = sort by type*/
#define ICONLIST_SORT_BY_SIZE	     (1<<4)		/*neither = sort by name*/

#define ICONLIST_SHOW_HIDDEN	     (1<<5)		/*needs to be globally overridable*/
#define ICONLIST_SHOW_INFO		     (1<<6)		/*show icon *.info files*/

#define ICONLIST_DISP_VERTICAL	     (1<<7)		/*tile icons vertically*/
#define ICONLIST_DISP_NOICONS	     (1<<8)		/*name only mode*/
#define ICONLIST_DISP_DETAILS	     (1<<9)		/*name=details mode, icon=??*/



/****************************************************************************/
/*** Name *******************************************************************/
#define MUIC_IconDrawerList         "IconDrawerList.mui"

/*** Identifier base ********************************************************/
#define MUIB_IconDrawerList         (MUIB_ZUNE | 0x00004300)  

/*** Attributes *************************************************************/
#define MUIA_IconDrawerList_Drawer  (MUIB_IconDrawerList | 0x00000000) /* Zune: V1  isg LONG     */



/****************************************************************************/
/*** Name *******************************************************************/
#define MUIC_IconVolumeList         "IconVolumneList.mui"

/*** Identifier base ********************************************************/
#define MUIB_IconVolumeList         (MUIB_ZUNE | 0x00004400)  



extern const struct __MUIBuiltinClass _MUI_IconList_desc; /* PRIV */
extern const struct __MUIBuiltinClass _MUI_IconDrawerList_desc; /* PRIV */
extern const struct __MUIBuiltinClass _MUI_IconVolumeList_desc; /* PRIV */


#endif /* _MUI_CLASSES_ICONLIST_H */
