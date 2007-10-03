#ifndef _MUI_CLASSES_NUMERIC_H
#define _MUI_CLASSES_NUMERIC_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Numeric               "Numeric.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Numeric               (MUIB_ZUNE | 0x00001e00)  

/*** Methods ****************************************************************/
#define MUIM_Numeric_Decrease      (MUIB_MUI|0x004243a7) /* MUI: V11 */
#define MUIM_Numeric_Increase      (MUIB_MUI|0x00426ecd) /* MUI: V11 */
#define MUIM_Numeric_ScaleToValue  (MUIB_MUI|0x0042032c) /* MUI: V11 */
#define MUIM_Numeric_SetDefault    (MUIB_MUI|0x0042ab0a) /* MUI: V11 */
#define MUIM_Numeric_Stringify     (MUIB_MUI|0x00424891) /* MUI: V11 */
#define MUIM_Numeric_ValueToScale  (MUIB_MUI|0x00423e4f) /* MUI: V11 */
struct MUIP_Numeric_Decrease       {STACKED ULONG MethodID; STACKED LONG amount;};
struct MUIP_Numeric_Increase       {STACKED ULONG MethodID; STACKED LONG amount;};
struct MUIP_Numeric_ScaleToValue   {STACKED ULONG MethodID; STACKED LONG scalemin; STACKED LONG scalemax; STACKED LONG scale;};
struct MUIP_Numeric_SetDefault     {STACKED ULONG MethodID;};
struct MUIP_Numeric_Stringify      {STACKED ULONG MethodID; STACKED LONG value;};
struct MUIP_Numeric_ValueToScale   {STACKED ULONG MethodID; STACKED LONG scalemin; STACKED LONG scalemax;};

/*** Attributes *************************************************************/
#define MUIA_Numeric_CheckAllSizes (MUIB_MUI|0x00421594) /* MUI: V11 isg BOOL   */
#define MUIA_Numeric_Default       (MUIB_MUI|0x004263e8) /* MUI: V11 isg LONG   */
#define MUIA_Numeric_Format        (MUIB_MUI|0x004263e9) /* MUI: V11 isg STRPTR */
#define MUIA_Numeric_Max           (MUIB_MUI|0x0042d78a) /* MUI: V11 isg LONG   */
#define MUIA_Numeric_Min           (MUIB_MUI|0x0042e404) /* MUI: V11 isg LONG   */
#define MUIA_Numeric_Reverse       (MUIB_MUI|0x0042f2a0) /* MUI: V11 isg BOOL   */
#define MUIA_Numeric_RevLeftRight  (MUIB_MUI|0x004294a7) /* MUI: V11 isg BOOL   */
#define MUIA_Numeric_RevUpDown     (MUIB_MUI|0x004252dd) /* MUI: V11 isg BOOL   */
#define MUIA_Numeric_Value         (MUIB_MUI|0x0042ae3a) /* MUI: V11 isg LONG   */

#define MUIM_Numeric_ValueToScaleExt (MUIB_Numeric | 0x00000000) /* ZUNE only */
struct MUIP_Numeric_ValueToScaleExt   {STACKED ULONG MethodID; STACKED LONG value; STACKED LONG scalemin; STACKED LONG scalemax;};

extern const struct __MUIBuiltinClass _MUI_Numeric_desc; /* PRIV */

#endif /* _MUI_CLASSES_NUMERIC_H */
