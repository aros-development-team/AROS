#ifndef _MUI_CLASSES_PROP_H
#define _MUI_CLASSES_PROP_H

#ifdef _DCC
extern char MUIC_Prop[];
#else
#define MUIC_Prop "Prop.mui"
#endif

/* Methods */

#define MUIM_Prop_Decrease                  0x80420dd1 /* V16 */
#define MUIM_Prop_Increase                  0x8042cac0 /* V16 */
struct  MUIP_Prop_Decrease                  { ULONG MethodID; LONG amount; };
struct  MUIP_Prop_Increase                  { ULONG MethodID; LONG amount; };

/* Attributes */

#define MUIA_Prop_Entries                   0x8042fbdb /* V4  isg LONG              */
#define MUIA_Prop_First                     0x8042d4b2 /* V4  isg LONG              */
#define MUIA_Prop_Horiz                     0x8042f4f3 /* V4  i.g BOOL              */
#define MUIA_Prop_Slider                    0x80429c3a /* V4  isg BOOL              */
#define MUIA_Prop_UseWinBorder              0x8042deee /* V13 i.. LONG              */
#define MUIA_Prop_Visible                   0x8042fea6 /* V4  isg LONG              */
#define MUIA_Prop_OnlyTrigger               0x8042fea7 /* PRIV */

#define MUIV_Prop_UseWinBorder_None 0
#define MUIV_Prop_UseWinBorder_Left 1
#define MUIV_Prop_UseWinBorder_Right 2
#define MUIV_Prop_UseWinBorder_Bottom 3

#define MUIA_Prop_FirstReal     0x804205DC
#define MUIA_Prop_PropRelease   0x80429839
#define MUIA_Prop_DeltaFactor   0x80427C5E /* is. LONG */
#define MUIA_Prop_DoSmooth      0x804236ce /* V4 i.. LONG */

extern const struct __MUIBuiltinClass _MUI_Prop_desc;

#endif
