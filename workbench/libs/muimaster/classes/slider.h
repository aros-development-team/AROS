#ifndef _MUI_CLASSES_SLIDER_H
#define _MUI_CLASSES_SLIDER_H

#ifdef _DCC
extern char MUIC_Slider[];
#else
#define MUIC_Slider "Slider.mui"
#endif

/* Attributes */

#define MUIA_Slider_Horiz                   0x8042fad1 /* V11 isg BOOL              */
#ifdef MUI_OBSOLETE
#define MUIA_Slider_Level                   0x8042ae3a /* V4  isg LONG              */
#endif /* MUI_OBSOLETE */
#ifdef MUI_OBSOLETE
#define MUIA_Slider_Max                     0x8042d78a /* V4  isg LONG              */
#endif /* MUI_OBSOLETE */
#ifdef MUI_OBSOLETE
#define MUIA_Slider_Min                     0x8042e404 /* V4  isg LONG              */
#endif /* MUI_OBSOLETE */
#define MUIA_Slider_Quiet                   0x80420b26 /* V6  i.. BOOL              */
#ifdef MUI_OBSOLETE
#define MUIA_Slider_Reverse                 0x8042f2a0 /* V4  isg BOOL              */
#endif /* MUI_OBSOLETE */

extern const struct __MUIBuiltinClass _MUI_Slider_desc;

#endif
