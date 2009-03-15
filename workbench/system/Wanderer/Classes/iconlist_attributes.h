#ifndef _MUI_CLASSES_ICONLIST_ATTRIBUTES_H
#define _MUI_CLASSES_ICONLIST_ATTRIBUTES_H

/*
	Copyright  2002-2009, The AROS Development Team. All rights reserved.
	$Id$
*/

/*** Identifier base ********************************************************/
#define MUIB_IconList                      (MUIB_ZUNE | 0x00042000)

#define MUIV_IconList_NextIcon_Start      0
#define MUIV_IconList_NextIcon_End        1

#define MUIV_IconList_NextIcon_Selected   2
#define MUIV_IconList_NextIcon_Visible    3

/*** Attributes *************************************************************/
#define MUIA_IconList_Changed                               (MUIB_IconList | 0x00000001) /* Zune: V1 .SG BOOL */
#define MUIA_IconList_Clicked                               (MUIB_IconList | 0x00000002) /* Zune: V1 ..G (struct IconList_Click *) */
#define MUIA_IconList_DoubleClick                           (MUIB_IconList | 0x00000003) /* Zune: V1 ..G BOOL                      */
#define MUIA_IconList_SelectionChanged                      (MUIB_IconList | 0x00000004) /* Zune: V1 ..G BOOL                         */

#define MUIA_IconList_Width                                 (MUIB_IconList | 0x0000000c) /* Zune: V1 .SG LONG                      */
#define MUIA_IconList_Height                                (MUIB_IconList | 0x0000000d) /* Zune: V1 .SG LONG                      */
#define MUIA_IconList_IconsMoved                            (MUIB_IconList | 0x00000010) /* Zune: V1 ..G (struct IconList_Entry *) */
#define MUIA_IconList_IconsDropped                          (MUIB_IconList | 0x00000011) /* Zune: V1 ..G (struct IconList_Entry *) */
#define MUIA_IconList_AppWindowDrop                         (MUIB_IconList | 0x00000012) /* Zune: V1 ..G (struct IconList_Entry *) */
#define MUIA_IconList_FocusIcon                             (MUIB_IconList | 0x00000013) /* Zune: V1 .SG (struct IconList_Entry *) */

#define MUIA_IconList_DisplayFlags                          (MUIB_IconList | 0x00000020) /* Zune: V1 ISG ULONG                     */
#define MUIA_IconList_SortFlags                             (MUIB_IconList | 0x00000021) /* Zune: V1 ISG ULONG                     */


/* OBSOLETE */
//#define MUIA_IconList_Left                                  (MUIB_IconList | 0x0000000a) /* Zune: V1 .SG LONG                      */
//#define MUIA_IconList_Top                                   (MUIB_IconList | 0x0000000b) /* Zune: V1 .SG LONG                      */

/* Configuration Attributes */
#define MUIB_IconList_ConfigTags                            (MUIB_IconList | 0x00000100)

#define MUIA_IconList_IconListMode                          (MUIB_IconList_ConfigTags | 0x00000000) /* Zune: V1 ISG UBYTE */
#define MUIA_IconList_LabelText_Mode                        (MUIB_IconList_ConfigTags | 0x00000001) /* Zune: V1 ISG UBYTE */
#define MUIA_IconList_LabelText_Pen                         (MUIB_IconList_ConfigTags | 0x00000002) /* Zune: V1 ISG UBYTE */
#define MUIA_IconList_LabelText_ShadowPen                   (MUIB_IconList_ConfigTags | 0x00000003) /* Zune: V1 ISG UBYTE */
#define MUIA_IconList_LabelInfoText_Font                    (MUIB_IconList_ConfigTags | 0x00000004) /* Zune: V1 ISG (struct TextFont *) */
#define MUIA_IconList_LabelInfoText_Pen                     (MUIB_IconList_ConfigTags | 0x00000005) /* Zune: V1 ISG UBYTE */
#define MUIA_IconList_LabelInfoText_ShadowPen               (MUIB_IconList_ConfigTags | 0x00000006) /* Zune: V1 ISG UBYTE */
#define MUIA_IconList_LabelText_MaxLineLen                  (MUIB_IconList_ConfigTags | 0x00000007) /* Zune: V1 ISG ULONG */
#define MUIA_IconList_LabelText_MultiLine                   (MUIB_IconList_ConfigTags | 0x00000008) /* Zune: V1 ISG ULONG */
#define MUIA_IconList_LabelText_MultiLineOnFocus            (MUIB_IconList_ConfigTags | 0x00000009) /* Zune: V1 ISG BOOL  */

#define MUIA_IconList_Icon_HorizontalSpacing                (MUIB_IconList_ConfigTags | 0x00000010) /* Zune: V1 ISG UBYTE  */
#define MUIA_IconList_Icon_VerticalSpacing                  (MUIB_IconList_ConfigTags | 0x00000011) /* Zune: V1 ISG UBYTE  */
#define MUIA_IconList_Icon_ImageSpacing                     (MUIB_IconList_ConfigTags | 0x00000012) /* Zune: V1 ISG UBYTE  */
#define MUIA_IconList_LabelText_HorizontalPadding           (MUIB_IconList_ConfigTags | 0x00000013) /* Zune: V1 ISG UBYTE  */
#define MUIA_IconList_LabelText_VerticalPadding             (MUIB_IconList_ConfigTags | 0x00000014) /* Zune: V1 ISG UBYTE  */
#define MUIA_IconList_LabelText_BorderWidth                 (MUIB_IconList_ConfigTags | 0x00000015) /* Zune: V1 ISG UBYTE  */
#define MUIA_IconList_LabelText_BorderHeight                (MUIB_IconList_ConfigTags | 0x00000016) /* Zune: V1 ISG UBYTE  */

#define MUIA_IconList_Rastport                              (MUIB_IconList | 0x000000FF)            /* Zune: V1 .SG (struct RastPort *)       */
#define MUIA_IconList_BufferRastport                        (MUIB_IconList | 0x000000FE)            /* Zune: V1 ..G BOOL      */

#define MUIA_IconList_BufferLeft                            (MUIB_IconList | 0x00000050)            /* Zune: V1 ..G LONG                      */
#define MUIA_IconList_BufferTop                             (MUIB_IconList | 0x00000051)            /* Zune: V1 ..G LONG                      */
#define MUIA_IconList_BufferWidth                           (MUIB_IconList | 0x00000052)            /* Zune: V1 ..G LONG                      */
#define MUIA_IconList_BufferHeight                          (MUIB_IconList | 0x00000053)            /* Zune: V1 ..G LONG                      */

/****************************************************************************/
/* iconlist rendering control flags */
/* SORTFLAGS 
	((flags & (ICONLIST_SORT_BY_NAME|ICONLIST_SORT_BY_DATE|ICONLIST_SORT_BY_SIZE) == 0)
	place icons with coords at fixed locations and sort remainder by name

	((flags & (ICONLIST_SORT_BY_NAME|ICONLIST_SORT_BY_DATE|ICONLIST_SORT_BY_SIZE) == ICONLIST_SORT_BY_NAME)
	sort icons by Name

	((flags & (ICONLIST_SORT_BY_NAME|ICONLIST_SORT_BY_DATE|ICONLIST_SORT_BY_SIZE) == ICONLIST_SORT_BY_DATE)
	sort icons by Date

	((flags & (ICONLIST_SORT_BY_NAME|ICONLIST_SORT_BY_DATE|ICONLIST_SORT_BY_SIZE) == ICONLIST_SORT_BY_SIZE)
	sort icons by Size

	((flags & (ICONLIST_SORT_BY_NAME|ICONLIST_SORT_BY_DATE|ICONLIST_SORT_BY_SIZE) == (ICONLIST_SORT_BY_NAME|ICONLIST_SORT_BY_DATE|ICONLIST_SORT_BY_SIZE))
	sort icons by Type
*/
#define ICONLIST_SORT_DRAWERS_MIXED  (1<<0)		/* mix folders and files when sorting  */
#define ICONLIST_SORT_DRAWERS_LAST   (1<<1)		/* ignored if mixed is set             */
#define ICONLIST_SORT_REVERSE        (1<<2)		/* reverse sort direction              */
#define ICONLIST_SORT_BY_NAME	     (1<<4)
#define ICONLIST_SORT_BY_DATE	     (1<<5)
#define ICONLIST_SORT_BY_SIZE	     (1<<6)

#define ICONLIST_SORT_MASK           (ICONLIST_SORT_BY_NAME|ICONLIST_SORT_BY_DATE|ICONLIST_SORT_BY_SIZE)

/* DISPLAYFLAGS */
#define ICONLIST_DISP_SHOWHIDDEN     (1<<0)		/* show system "hidden" files          */
#define ICONLIST_DISP_SHOWINFO	     (1<<1)		/* only show icon(s) which have *.info files */

#define ICONLIST_DISP_VERTICAL	     (1<<7)		/* tile icons vertically               */
#define ICONLIST_DISP_NOICONS	     (1<<8)		/* name only mode                      */
#define ICONLIST_DISP_DETAILS	     (1<<9)		/* name=details mode, icon=??          */

#define ICONLIST_DISP_BUFFERED       (1<<15)            /* Iconlist uses buffered rendering */

enum iconlist_ListViewModes
{
	ICON_LISTMODE_GRID = 0,
	ICON_LISTMODE_ROUGH = 1,
	iconlist_ListViewModesCount
};

enum iconlist_LabelRenderModes
{
	ICON_TEXTMODE_OUTLINE = 0,
	ICON_TEXTMODE_PLAIN = 1,
	ICON_TEXTMODE_DROPSHADOW = 2,
	iconlist_LabelRenderModesCount
};

/* Default Icon label rendering settings */
// Max no of characters to display on a single line
#define ILC_ICONLABEL_MAXLINELEN_DEFAULT               10
#define ILC_ICONLABEL_SHORTEST                         6

/* Default Icon rendering settings */
// Spacing between icons ..
#define ILC_ICON_HORIZONTALMARGIN_DEFAULT              5
#define ILC_ICON_VERTICALMARGIN_DEFAULT                5

// Padding between Icon's "image" and label frame
#define ILC_ICONLABEL_IMAGEMARGIN_DEFAULT              1

// Padding between Icons label text and frame
#define ILC_ICONLABEL_HORIZONTALTEXTMARGIN_DEFAULT     4
#define ILC_ICONLABEL_VERTICALTEXTMARGIN_DEFAULT       4

// Icon's Label frame dimensions
#define ILC_ICONLABEL_BORDERWIDTH_DEFAULT              1
#define ILC_ICONLABEL_BORDERHEIGHT_DEFAULT             1

/*** Identifier base ********************************************************/
#define MUIB_IconDrawerList           (MUIB_ZUNE | 0x00043000)  

/*** Attributes *************************************************************/
#define MUIA_IconDrawerList_Drawer    (MUIB_IconDrawerList | 0x00000000)      /* Zune: V1  isg LONG     */

/*** Identifier base ********************************************************/
#define MUIB_IconVolumeList           (MUIB_ZUNE | 0x00044000)  

#endif /* _MUI_CLASSES_ICONLIST_ATTRIBUTES_H */
