#ifndef _MUI_CLASSES_POPSTRING_H
#define _MUI_CLASSES_POPSTRING_H

/****************************************************************************/
/** Popstring                                                              **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Popstring[];
#else
#define MUIC_Popstring "Popstring.mui"
#endif

/* Methods */

#define MUIM_Popstring_Close                0x8042dc52 /* V7  */
#define MUIM_Popstring_Open                 0x804258ba /* V7  */
struct  MUIP_Popstring_Close                { ULONG MethodID; LONG result; };
struct  MUIP_Popstring_Open                 { ULONG MethodID; };

/* Attributes */

#define MUIA_Popstring_Button               0x8042d0b9 /* V7  i.g Object *          */
#define MUIA_Popstring_CloseHook            0x804256bf /* V7  isg struct Hook *     */
#define MUIA_Popstring_OpenHook             0x80429d00 /* V7  isg struct Hook *     */
#define MUIA_Popstring_String               0x804239ea /* V7  i.g Object *          */
#define MUIA_Popstring_Toggle               0x80422b7a /* V7  isg BOOL              */



/****************************************************************************/
/** Popobject                                                              **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Popobject[];
#else
#define MUIC_Popobject "Popobject.mui"
#endif

/* Attributes */

#define MUIA_Popobject_Follow               0x80424cb5 /* V7  isg BOOL              */
#define MUIA_Popobject_Light                0x8042a5a3 /* V7  isg BOOL              */
#define MUIA_Popobject_Object               0x804293e3 /* V7  i.g Object *          */
#define MUIA_Popobject_ObjStrHook           0x8042db44 /* V7  isg struct Hook *     */
#define MUIA_Popobject_StrObjHook           0x8042fbe1 /* V7  isg struct Hook *     */
#define MUIA_Popobject_Volatile             0x804252ec /* V7  isg BOOL              */
#define MUIA_Popobject_WindowHook           0x8042f194 /* V9  isg struct Hook *     */



/****************************************************************************/
/** Poplist                                                                **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Poplist[];
#else
#define MUIC_Poplist "Poplist.mui"
#endif

/* Attributes */

#define MUIA_Poplist_Array                  0x8042084c /* V8  i.. char **           */



/****************************************************************************/
/** Popscreen                                                              **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Popscreen[];
#else
#define MUIC_Popscreen "Popscreen.mui"
#endif

/* Attributes */




/****************************************************************************/
/** Popasl                                                                 **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Popasl[];
#else
#define MUIC_Popasl "Popasl.mui"
#endif

/* Attributes */

#define MUIA_Popasl_Active                  0x80421b37 /* V7  ..g BOOL              */
#define MUIA_Popasl_StartHook               0x8042b703 /* V7  isg struct Hook *     */
#define MUIA_Popasl_StopHook                0x8042d8d2 /* V7  isg struct Hook *     */
#define MUIA_Popasl_Type                    0x8042df3d /* V7  i.g ULONG             */

extern const struct __MUIBuiltinClass _MUI_Popstring_desc;
extern const struct __MUIBuiltinClass _MUI_Popasl_desc;
extern const struct __MUIBuiltinClass _MUI_Popobject_desc;

#endif
