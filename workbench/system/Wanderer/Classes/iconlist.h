#ifndef _MUI_CLASSES_ICONLIST_H
#define _MUI_CLASSES_ICONLIST_H

/*
	Copyright  2002-2007, The AROS Development Team. All rights reserved.
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

#include "iconlist_attributes.h"

/****************************************************************************/
/*** Name *******************************************************************/
#define MUIC_IconList "IconList.mui"

/*** Methods ****************************************************************/
#define MUIM_IconList_Clear             (MUIB_IconList | 0x00000000) /* Zune: V1 */
#define MUIM_IconList_Update            (MUIB_IconList | 0x00000001) /* Zune: V1 */
#define MUIM_IconList_RethinkDimensions (MUIB_IconList | 0x00000002) /* Zune: V1 */
#define MUIM_IconList_CreateEntry       (MUIB_IconList | 0x00000010) /* Zune: V1 returns 0 For Failure or (struct IconEntry *) */
#define MUIM_IconList_DestroyEntry      (MUIB_IconList | 0x00000011) /* Zune: V1 */
#define MUIM_IconList_DrawEntry         (MUIB_IconList | 0x00000012) /* Zune: V1 */
#define MUIM_IconList_DrawEntryLabel    (MUIB_IconList | 0x00000013) /* Zune: V1 */
#define MUIM_IconList_SelectAll         (MUIB_IconList | 0x00000020) /* Zune: V1 */
#define MUIM_IconList_UnselectAll       (MUIB_IconList | 0x00000021) /* Zune: V1 */
#define MUIM_IconList_NextSelected      (MUIB_IconList | 0x00000025) /* Zune: V1 */
#define MUIM_IconList_NextVisible       (MUIB_IconList | 0x00000026) /* Zune: V1 */
#define MUIM_IconList_Sort              (MUIB_IconList | 0x00000031) /* Zune: V1 */
#define MUIM_IconList_CoordsSort        (MUIB_IconList | 0x00000032) /* Zune: V1 */
#define MUIM_IconList_PositionIcons     (MUIB_IconList | 0x00000033) /* Zune: V1 */
#define MUIM_IconList_ViewIcon          (MUIB_IconList | 0x00000034) /* Zune: V1 */

struct MUIP_IconList_Clear              {STACKED ULONG MethodID;};
struct MUIP_IconList_Update             {STACKED ULONG MethodID;};
struct MUIP_IconList_RethinkDimensions  {STACKED ULONG MethodID; STACKED struct IconEntry *singleicon;};
struct MUIP_IconList_CreateEntry        {STACKED ULONG MethodID; STACKED char *filename; STACKED char *label; STACKED struct FileInfoBlock *fib; STACKED struct DiskObject *icon_dob;};/* void *udata; More file attrs to add };*/
struct MUIP_IconList_DestroyEntry       {STACKED ULONG MethodID; STACKED struct IconEntry *icon;};
struct MUIP_IconList_DrawEntry          {STACKED ULONG MethodID; STACKED struct IconEntry *icon; STACKED IPTR drawmode;};
struct MUIP_IconList_DrawEntryLabel     {STACKED ULONG MethodID; STACKED struct IconEntry *icon; STACKED IPTR drawmode;};
struct MUIP_IconList_NextSelected       {STACKED ULONG MethodID; STACKED struct IconList_Entry **entry;}; /* *entry maybe MUIV_IconList_NextSelected_Start, *entry is MUIV_IconList_NextSelected_End if no more entries are selected */
struct MUIP_IconList_Sort               {STACKED ULONG MethodID;};
struct MUIP_IconList_PositionIcons      {STACKED ULONG MethodID;};
struct MUIP_IconList_ViewIcon           {STACKED ULONG MethodID; STACKED struct IconEntry *icon;};

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
	struct Node                   ile_IconNode;
	struct Node                   ile_SelectionNode;

	struct IconList_Entry         ile_IconListEntry;

	struct DiskObject             *ile_DiskObj;                       /* The icons disk objects */
	struct FileInfoBlock          ile_FileInfoBlock;

	LONG                          ile_IconX,                          /* Top Left Co-ords of Icons "AREA" */
								  ile_IconY;

	ULONG                         ile_IconWidth,					  /* Width/Height of Icon "Image" */
								  ile_IconHeight,
								  ile_AreaWidth,                      /* Width/Height of Icon "AREA" ..    */
								  ile_AreaHeight;                     /* if the icons Label Width is larger than
																		 ile_IconWidth, AreaWidth = the icons label Width
																		 else it will be the same as ile_IconWidth */

	ULONG                         ile_Flags;

	UBYTE                         *ile_TxtBuf_DisplayedLabel;
	ULONG                         ile_SplitParts;
	ULONG						  ile_TxtBuf_DisplayedLabelWidth;
	UBYTE   	    	          *ile_TxtBuf_DATE;
	ULONG   	    	          ile_TxtBuf_DATEWidth;
	UBYTE   	    	          *ile_TxtBuf_TIME;
	ULONG   	    	          ile_TxtBuf_TIMEWidth;
	UBYTE   	    	          *ile_TxtBuf_SIZE;
	ULONG   	    	          ile_TxtBuf_SIZEWidth;
	UBYTE   	    	          *ile_TxtBuf_PROT;
};

/****************************************************************************/
/*** Name *******************************************************************/
#define MUIC_IconDrawerList         "IconDrawerList.mui"

/****************************************************************************/
/*** Name *******************************************************************/
#define MUIC_IconVolumeList         "IconVolumeList.mui"

extern const struct __MUIBuiltinClass _MUI_IconList_desc; /* PRIV */
extern const struct __MUIBuiltinClass _MUI_IconDrawerList_desc; /* PRIV */
extern const struct __MUIBuiltinClass _MUI_IconVolumeList_desc; /* PRIV */

#ifdef __AROS__
#define IconListObject       MUIOBJMACRO_START(MUIC_IconList)
#define IconVolumeListObject MUIOBJMACRO_START(MUIC_IconVolumeList)
#define IconDrawerListObject MUIOBJMACRO_START(MUIC_IconDrawerList)
#else
#define IconDrawerListObject   NewObject(IconDrawerList_Class->mcc_Class, NULL
#define IconVolumeListObject   NewObject(IconVolumeList_Class->mcc_Class, NULL
#define IconListObject         NewObject(IconList_Class->mcc_Class, NULL
#endif

#endif /* _MUI_CLASSES_ICONLIST_H */
