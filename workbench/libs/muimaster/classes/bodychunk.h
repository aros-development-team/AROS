/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _MUI_CLASSES_BODYCHUNK_H
#define _MUI_CLASSES_BODYCHUNK_H

/*** Name *******************************************************************/
#define MUIC_Bodychunk              "Bodychunk.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Bodychunk              (MUIB_ZUNE | 0x00000500)  

/*** Attributes *************************************************************/
#define MUIA_Bodychunk_Body         (MUIB_MUI|0x0042ca67) /* V8  isg UBYTE * */
#define MUIA_Bodychunk_Compression  (MUIB_MUI|0x0042de5f) /* V8  isg UBYTE   */
#define MUIA_Bodychunk_Depth        (MUIB_MUI|0x0042c392) /* V8  isg LONG    */
#define MUIA_Bodychunk_Masking      (MUIB_MUI|0x00423b0e) /* V8  isg UBYTE   */


extern const struct __MUIBuiltinClass _MUI_Bodychunk_desc; /* PRIV */

#endif /* _MUI_CLASSES_BODYCHUNK_H */
