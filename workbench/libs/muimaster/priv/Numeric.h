/****************************************************************************/
/** Numeric                                                                **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Numeric[];
#else
#define MUIC_Numeric "Numeric.mui"
#endif

/* Methods */

#define MUIM_Numeric_Decrease               0x804243a7 /* V11 */
#define MUIM_Numeric_Increase               0x80426ecd /* V11 */
#define MUIM_Numeric_ScaleToValue           0x8042032c /* V11 */
#define MUIM_Numeric_SetDefault             0x8042ab0a /* V11 */
#define MUIM_Numeric_Stringify              0x80424891 /* V11 */
#define MUIM_Numeric_ValueToScale           0x80423e4f /* V11 */
struct  MUIP_Numeric_Decrease               { ULONG MethodID; LONG amount; };
struct  MUIP_Numeric_Increase               { ULONG MethodID; LONG amount; };
struct  MUIP_Numeric_ScaleToValue           { ULONG MethodID; LONG scalemin; LONG scalemax; LONG scale; };
struct  MUIP_Numeric_SetDefault             { ULONG MethodID; };
struct  MUIP_Numeric_Stringify              { ULONG MethodID; LONG value; };
struct  MUIP_Numeric_ValueToScale           { ULONG MethodID; LONG scalemin; LONG scalemax; };

/* Attributes */

#define MUIA_Numeric_CheckAllSizes          0x80421594 /* V11 isg BOOL              */
#define MUIA_Numeric_Default                0x804263e8 /* V11 isg LONG              */
#define MUIA_Numeric_Format                 0x804263e9 /* V11 isg STRPTR            */
#define MUIA_Numeric_Max                    0x8042d78a /* V11 isg LONG              */
#define MUIA_Numeric_Min                    0x8042e404 /* V11 isg LONG              */
#define MUIA_Numeric_Reverse                0x8042f2a0 /* V11 isg BOOL              */
#define MUIA_Numeric_RevLeftRight           0x804294a7 /* V11 isg BOOL              */
#define MUIA_Numeric_RevUpDown              0x804252dd /* V11 isg BOOL              */
#define MUIA_Numeric_Value                  0x8042ae3a /* V11 isg LONG              */

