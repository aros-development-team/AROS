#ifndef _MUI_CLASSES_LEVELMETER_H
#define _MUI_CLASSES_LEVELMETER_H

/*
    Copyright © 2002-2006, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Levelmeter          "Levelmeter.mui"

/*** Identifier base (for AROS extensions) ***********************************/
/* Class-local Zune extension block. */
#define MUIB_Levelmeter         (MUIB_ZUNE | 0x00003800)

/*** Attributes *************************************************************/
#define MUIA_Levelmeter_Label   (MUIB_MUI | 0x00420dd5) /* V11 isg STRPTR */
#define MUIA_Levelmeter_Style \
    (MUIB_Levelmeter | 0x00000001) /* Zune: V1 isg ULONG */
#define MUIA_Levelmeter_Segments \
    (MUIB_Levelmeter | 0x00000002) /* Zune: V1 isg ULONG */
#define MUIA_Levelmeter_Horiz \
    (MUIB_Levelmeter | 0x00000003) /* Zune: V1 isg BOOL */
#define MUIA_Levelmeter_WarningSegments \
    (MUIB_Levelmeter | 0x00000004) /* Zune: V1 isg ULONG */
#define MUIA_Levelmeter_CriticalSegments \
    (MUIB_Levelmeter | 0x00000005) /* Zune: V1 isg ULONG */

#define MUIV_Levelmeter_Style_Needle      0
#define MUIV_Levelmeter_Style_Segments    1

#define MUIV_Levelmeter_Segments_Default  16
#define MUIV_Levelmeter_Segments_Max      256


extern const struct __MUIBuiltinClass _MUI_Levelmeter_desc;     /* PRIV */

#endif /* _MUI_CLASSES_LEVELMETER_H */
