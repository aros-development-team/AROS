/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef _MUI_CLASSES_NUMERIC_H
#define _MUI_CLASSES_NUMERIC_H

#define MUIC_Numeric "Numeric.mui"

#ifndef METHOD_USER
#define METHOD_USER TAG_USER
#endif

/* Numeric methods */
#define MUIM_Numeric_Decrease      (METHOD_USER|0x004243a7) /* MUI: V11 */
#define MUIM_Numeric_Increase      (METHOD_USER|0x00426ecd) /* MUI: V11 */
#define MUIM_Numeric_ScaleToValue  (METHOD_USER|0x0042032c) /* MUI: V11 */
#define MUIM_Numeric_SetDefault    (METHOD_USER|0x0042ab0a) /* MUI: V11 */
#define MUIM_Numeric_Stringify     (METHOD_USER|0x00424891) /* MUI: V11 */
#define MUIM_Numeric_ValueToScale  (METHOD_USER|0x00423e4f) /* MUI: V11 */
struct MUIP_Numeric_Decrease       {ULONG MethodID; LONG amount;};
struct MUIP_Numeric_Increase       {ULONG MethodID; LONG amount;};
struct MUIP_Numeric_ScaleToValue   {ULONG MethodID; LONG scalemin; LONG scalemax; LONG scale;};
struct MUIP_Numeric_SetDefault     {ULONG MethodID;};
struct MUIP_Numeric_Stringify      {ULONG MethodID; LONG value;};
struct MUIP_Numeric_ValueToScale   {ULONG MethodID; LONG scalemin; LONG scalemax;};

/* Numeric attributes */
#define MUIA_Numeric_CheckAllSizes (TAG_USER|0x00421594) /* MUI: V11 isg BOOL   */
#define MUIA_Numeric_Default       (TAG_USER|0x004263e8) /* MUI: V11 isg LONG   */
#define MUIA_Numeric_Format        (TAG_USER|0x004263e9) /* MUI: V11 isg STRPTR */
#define MUIA_Numeric_Max           (TAG_USER|0x0042d78a) /* MUI: V11 isg LONG   */
#define MUIA_Numeric_Min           (TAG_USER|0x0042e404) /* MUI: V11 isg LONG   */
#define MUIA_Numeric_Reverse       (TAG_USER|0x0042f2a0) /* MUI: V11 isg BOOL   */
#define MUIA_Numeric_RevLeftRight  (TAG_USER|0x004294a7) /* MUI: V11 isg BOOL   */
#define MUIA_Numeric_RevUpDown     (TAG_USER|0x004252dd) /* MUI: V11 isg BOOL   */
#define MUIA_Numeric_Value         (TAG_USER|0x0042ae3a) /* MUI: V11 isg LONG   */

extern const struct __MUIBuiltinClass _MUI_Numeric_desc; /* PRIV */

#endif
