#ifndef _MUI_CLASSES_GAUGE
#define _MUI_CLASSES_GAUGE

/****************************************************************************/
/** Gauge                                                                  **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Gauge[];
#else
#define MUIC_Gauge "Gauge.mui"
#endif

/* Attributes */

#define MUIA_Gauge_Current                  0x8042f0dd /* V4  isg LONG              */
#define MUIA_Gauge_Divide                   0x8042d8df /* V4  isg BOOL              */
#define MUIA_Gauge_Horiz                    0x804232dd /* V4  i.. BOOL              */
#define MUIA_Gauge_InfoText                 0x8042bf15 /* V7  isg STRPTR            */
#define MUIA_Gauge_Max                      0x8042bcdb /* V4  isg LONG              */

extern const struct __MUIBuiltinClass _MUI_Gauge_desc;

#endif
