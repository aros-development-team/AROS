/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef _MUI_CLASSES_BODYCHUNK_H
#define _MUI_CLASSES_BODYCHUNK_H

/****************************************************************************/
/** Bodychunk                                                              **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Bodychunk[];
#else
#define MUIC_Bodychunk "Bodychunk.mui"
#endif

/* Attributes */

#define MUIA_Bodychunk_Body                 0x8042ca67 /* V8  isg UBYTE *           */
#define MUIA_Bodychunk_Compression          0x8042de5f /* V8  isg UBYTE             */
#define MUIA_Bodychunk_Depth                0x8042c392 /* V8  isg LONG              */
#define MUIA_Bodychunk_Masking              0x80423b0e /* V8  isg UBYTE             */

extern const struct __MUIBuiltinClass _MUI_Bodychunk_desc;

#endif
