#ifndef _MUI_CLASSES_ICONLIST_ATTRIBUTES_H
#define _MUI_CLASSES_ICONLIST_ATTRIBUTES_H

/*
    Copyright  2002-2007, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Identifier base ********************************************************/
#define MUIB_IconList                   (MUIB_ZUNE | 0x00004200)

#define MUIV_IconList_NextSelected_Start 0
#define MUIV_IconList_NextSelected_End   1

/*** Attributes *************************************************************/
#define MUIA_IconList_DoubleClick                           (MUIB_IconList | 0x00000000) /* Zune: V1 ..G BOOL                      */
#define MUIA_IconList_Left                                  (MUIB_IconList | 0x00000001) /* Zune: V1 .SG LONG                      */
#define MUIA_IconList_Top                                   (MUIB_IconList | 0x00000002) /* Zune: V1 .SG LONG                      */
#define MUIA_IconList_Width                                 (MUIB_IconList | 0x00000003) /* Zune: V1 .SG LONG                      */
#define MUIA_IconList_Height                                (MUIB_IconList | 0x00000004) /* Zune: V1 .SG LONG                      */
#define MUIA_IconList_IconsDropped                          (MUIB_IconList | 0x00000005) /* Zune: V1 ..G (struct IconList_Entry *) */
#define MUIA_IconList_Clicked                               (MUIB_IconList | 0x00000006) /* Zune: V1 ..G (struct IconList_Click *) */
#define MUIA_IconList_IconsMoved                            (MUIB_IconList | 0x00000007) /* Zune: V1 ..G (struct IconList_Entry *) */
#define MUIA_IconList_AppWindowDrop                         (MUIB_IconList | 0x00000008) /* Zune: V1 ..G (struct IconList_Entry *) */

#define MUIA_IconList_FocusIcon         					(MUIB_IconList | 0x00000010) /* Zune: V1 .SG (struct IconList_Entry *) */

#define MUIA_IconList_DisplayFlags                          (MUIB_IconList | 0x00000020) /* Zune: V1 ISG ULONG                     */
#define MUIA_IconList_SortFlags                             (MUIB_IconList | 0x00000021) /* Zune: V1 ISG ULONG                     */

/* Configuration Attributes */
#define MUIB_IconList_ConfigTags                            (MUIB_IconList | 0x00000090)

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
#define MUIA_IconList_LabelText_HorizontalSpacing           (MUIB_IconList_ConfigTags | 0x00000013) /* Zune: V1 ISG UBYTE  */
#define MUIA_IconList_LabelText_VerticalSpacing             (MUIB_IconList_ConfigTags | 0x00000014) /* Zune: V1 ISG UBYTE  */
#define MUIA_IconList_LabelText_BorderWidth                 (MUIB_IconList_ConfigTags | 0x00000015) /* Zune: V1 ISG UBYTE  */
#define MUIA_IconList_LabelText_BorderHeight                (MUIB_IconList_ConfigTags | 0x00000016) /* Zune: V1 ISG UBYTE  */

#define MUIA_IconList_Rastport                              (MUIB_IconList | 0x000000FF) /* Zune: V1 ISG (struct RastPort *)       */

/****************************************************************************/
#define ICONENTRY_DRAWMODE_NONE      0         /* Do nothing .. */
#define ICONENTRY_DRAWMODE_PLAIN     1         /* Draw operations should clear the background first .. */
#define ICONENTRY_DRAWMODE_NOBACK    2         /* Draw operations shouldnt clear the background        */
#define ICONENTRY_DRAWMODE_BACKONLY  3         /* Draw operation should _only_ draw the background     */

/* Internal Icon state flags */
#define ICONENTRY_FLAG_SELECTED      (1<<1)		/* icon selected state              */
#define ICONENTRY_FLAG_FOCUS         (1<<2)		/* icon input focus state           */
#define ICONENTRY_FLAG_VISIBLE		 (1<<3)		/* icon for entry should be drawn   */
#define ICONENTRY_FLAG_HASICON	     (1<<4)		/* entry has an '.info' file        */
#define ICONENTRY_FLAG_TODAY	     (1<<7)		/* entry's timestamp is from today    */

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

/*** Identifier base ********************************************************/
#define MUIB_IconDrawerList         (MUIB_ZUNE | 0x00004300)  

/*** Attributes *************************************************************/
#define MUIA_IconDrawerList_Drawer  (MUIB_IconDrawerList | 0x00000000) /* Zune: V1  isg LONG     */

/*** Identifier base ********************************************************/
#define MUIB_IconVolumeList         (MUIB_ZUNE | 0x00004400)  

#endif /* _MUI_CLASSES_ICONLIST_ATTRIBUTES_H */
