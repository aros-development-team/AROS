#ifndef _MUI_CLASSES_SCROLLGROUP_H
#define _MUI_CLASSES_SCROLLGROUP_H

/****************************************************************************/
/** Scrollgroup                                                            **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Scrollgroup[];
#else
#define MUIC_Scrollgroup "Scrollgroup.mui"
#endif

/* Methods */


/* Attributes */

#define MUIA_Scrollgroup_Contents           0x80421261 /* V4  i.g Object *          */
#define MUIA_Scrollgroup_FreeHoriz          0x804292f3 /* V9  i.. BOOL              */
#define MUIA_Scrollgroup_FreeVert           0x804224f2 /* V9  i.. BOOL              */
#define MUIA_Scrollgroup_HorizBar           0x8042b63d /* V16 ..g Object *          */
#define MUIA_Scrollgroup_UseWinBorder       0x804284c1 /* V13 i.. BOOL              */
#define MUIA_Scrollgroup_VertBar            0x8042cdc0 /* V16 ..g Object *          */

extern const struct __MUIBuiltinClass _MUI_Scrollgroup_desc;

#endif
