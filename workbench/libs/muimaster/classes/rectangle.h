#ifndef _MUI_CLASSES_RECTANGLE_H
#define _MUI_CLASSES_RECTANGLE_H

/****************************************************************************/
/** Rectangle                                                              **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Rectangle[];
#else
#define MUIC_Rectangle "Rectangle.mui"
#endif

/* Attributes */

#define MUIA_Rectangle_BarTitle             0x80426689 /* V11 i.g STRPTR            */
#define MUIA_Rectangle_HBar                 0x8042c943 /* V7  i.g BOOL              */
#define MUIA_Rectangle_VBar                 0x80422204 /* V7  i.g BOOL              */

extern const struct __MUIBuiltinClass _MUI_Rectangle_desc;

#endif
