#ifndef _WANDERER_CLASSES_ICON_ATTRIBUTES_H
#define _WANDERER_CLASSES_ICON_ATTRIBUTES_H

/*
    Copyright  2008-2009, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Identifier base ********************************************************/
#define MUIB_Icon                      (MUIB_ZUNE | 0x00042000)


/*** Attributes *************************************************************/
#define MUIA_Icon_Clicked                               (MUIB_Icon | 0x00000001) /* Zune: V1 ..G (struct Icon_Click *) */
#define MUIA_Icon_DoubleClick                           (MUIB_Icon | 0x00000002) /* Zune: V1 ..G BOOL                      */
#define MUIA_Icon_SelectionChanged                      (MUIB_Icon | 0x00000003) /* Zune: V1 ..GBOOL                         */

#define MUIA_Icon_Width                                 (MUIB_Icon | 0x0000000c) /* Zune: V1 .SG LONG                      */
#define MUIA_Icon_Height                                (MUIB_Icon | 0x0000000d) /* Zune: V1 .SG LONG                      */
#define MUIA_Icon_IconsMoved                            (MUIB_Icon | 0x00000010) /* Zune: V1 ..G (struct Icon_Entry *) */
#define MUIA_Icon_IconsDropped                          (MUIB_Icon | 0x00000011) /* Zune: V1 ..G (struct Icon_Entry *) */
#define MUIA_Icon_AppWindowDrop                         (MUIB_Icon | 0x00000012) /* Zune: V1 ..G (struct Icon_Entry *) */
#define MUIA_Icon_FocusIcon                             (MUIB_Icon | 0x00000013) /* Zune: V1 .SG (struct Icon_Entry *) */

#define MUIA_Icon_DisplayFlags                          (MUIB_Icon | 0x00000020) /* Zune: V1 ISG ULONG                     */
#define MUIA_Icon_SortFlags                             (MUIB_Icon | 0x00000021) /* Zune: V1 ISG ULONG                     */


/* OBSOLETE */
//#define MUIA_Icon_Left                                  (MUIB_Icon | 0x0000000a) /* Zune: V1 .SG LONG                      */
//#define MUIA_Icon_Top                                   (MUIB_Icon | 0x0000000b) /* Zune: V1 .SG LONG                      */

/* Configuration Attributes */
#define MUIB_Icon_ConfigTags                            (MUIB_Icon | 0x00000100)

#define MUIA_Icon_IconMode                          (MUIB_Icon_ConfigTags | 0x00000000) /* Zune: V1 ISG UBYTE */
#define MUIA_Icon_LabelText_Mode                        (MUIB_Icon_ConfigTags | 0x00000001) /* Zune: V1 ISG UBYTE */
#define MUIA_Icon_LabelText_Pen                         (MUIB_Icon_ConfigTags | 0x00000002) /* Zune: V1 ISG UBYTE */
#define MUIA_Icon_LabelText_ShadowPen                   (MUIB_Icon_ConfigTags | 0x00000003) /* Zune: V1 ISG UBYTE */
#define MUIA_Icon_LabelInfoText_Font                    (MUIB_Icon_ConfigTags | 0x00000004) /* Zune: V1 ISG (struct TextFont *) */
#define MUIA_Icon_LabelInfoText_Pen                     (MUIB_Icon_ConfigTags | 0x00000005) /* Zune: V1 ISG UBYTE */
#define MUIA_Icon_LabelInfoText_ShadowPen               (MUIB_Icon_ConfigTags | 0x00000006) /* Zune: V1 ISG UBYTE */
#define MUIA_Icon_LabelText_MaxLineLen                  (MUIB_Icon_ConfigTags | 0x00000007) /* Zune: V1 ISG ULONG */
#define MUIA_Icon_LabelText_MultiLine                   (MUIB_Icon_ConfigTags | 0x00000008) /* Zune: V1 ISG ULONG */
#define MUIA_Icon_LabelText_MultiLineOnFocus            (MUIB_Icon_ConfigTags | 0x00000009) /* Zune: V1 ISG BOOL  */

#define MUIA_Icon_Icon_HorizontalSpacing                (MUIB_Icon_ConfigTags | 0x00000010) /* Zune: V1 ISG UBYTE  */
#define MUIA_Icon_Icon_VerticalSpacing                  (MUIB_Icon_ConfigTags | 0x00000011) /* Zune: V1 ISG UBYTE  */
#define MUIA_Icon_Icon_ImageSpacing                     (MUIB_Icon_ConfigTags | 0x00000012) /* Zune: V1 ISG UBYTE  */
#define MUIA_Icon_LabelText_HorizontalPadding           (MUIB_Icon_ConfigTags | 0x00000013) /* Zune: V1 ISG UBYTE  */
#define MUIA_Icon_LabelText_VerticalPadding             (MUIB_Icon_ConfigTags | 0x00000014) /* Zune: V1 ISG UBYTE  */
#define MUIA_Icon_LabelText_BorderWidth                 (MUIB_Icon_ConfigTags | 0x00000015) /* Zune: V1 ISG UBYTE  */
#define MUIA_Icon_LabelText_BorderHeight                (MUIB_Icon_ConfigTags | 0x00000016) /* Zune: V1 ISG UBYTE  */

#define MUIA_Icon_Rastport                              (MUIB_Icon | 0x000000FF)            /* Zune: V1 .SG (struct RastPort *)       */
#define MUIA_Icon_BufferRastport                        (MUIB_Icon | 0x000000FE)            /* Zune: V1 ..G BOOL      */

#define MUIA_Icon_BufferLeft                            (MUIB_Icon | 0x00000050)            /* Zune: V1 ..G LONG                      */
#define MUIA_Icon_BufferTop                             (MUIB_Icon | 0x00000051)            /* Zune: V1 ..G LONG                      */
#define MUIA_Icon_BufferWidth                           (MUIB_Icon | 0x00000052)            /* Zune: V1 ..G LONG                      */
#define MUIA_Icon_BufferHeight                          (MUIB_Icon | 0x00000053)            /* Zune: V1 ..G LONG                      */

/****************************************************************************/
#define ICONENTRY_DRAWMODE_NONE      0         /* Do nothing .. */
#define ICONENTRY_DRAWMODE_PLAIN     1         /* Draw operations should clear the background first .. */
#define ICONENTRY_DRAWMODE_NOBACK    2         /* Draw operations shouldnt clear the background        */
#define ICONENTRY_DRAWMODE_BACKONLY  3         /* Draw operation should _only_ draw the background     */

/* Internal Icon state flags */
#define ICONENTRY_FLAG_NEEDSUPDATE      (1<<1)        /* icon needs rendered                */
#define ICONENTRY_FLAG_SELECTED         (1<<2)        /* icon selected state                */
#define ICONENTRY_FLAG_FOCUS            (1<<3)        /* icon input focus state             */
#define ICONENTRY_FLAG_VISIBLE          (1<<4)        /* icon for entry should be drawn     */
#define ICONENTRY_FLAG_HASICON          (1<<5)        /* entry has an '.info' file          */
#define ICONENTRY_FLAG_TODAY            (1<<6)        /* entry's timestamp is from today    */
#define ICONENTRY_FLAG_LASSO            (1<<7)        /* icon is being altered by a lasso  */
#define ICONENTRY_FLAG_RESERVED         (1<<8)        /* reserved for local use */


/* For Icons of type ST_ROOT */
#define ICONENTRY_VOL_DISABLED         (1<<0)        /* Media is Read-Only           */
#define ICONENTRY_VOL_READONLY         (1<<1)        /* Media is Read-Only           */
#define ICONENTRY_VOL_OFFLINE         (1<<2)        /* Volume is Offline            */
#define ICONENTRY_VOL_REMOVABLE         (1<<3)        /* Media is Removable           */

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

#endif /* _WANDERER_CLASSES_ICON_ATTRIBUTES_H */
