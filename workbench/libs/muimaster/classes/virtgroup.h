/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef _MUI_CLASSES_VIRTGROUP_H
#define _MUI_CLASSES_VIRTGROUP_H

/****************************************************************************/
/** Virtgroup                                                              **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Virtgroup[];
#else
#define MUIC_Virtgroup "Virtgroup.mui"
#endif

/* Methods */


/* Attributes */

#define MUIA_Virtgroup_Height               0x80423038 /* V6  ..g LONG              */
#define MUIA_Virtgroup_Input                0x80427f7e /* V11 i.. BOOL              */
#define MUIA_Virtgroup_Left                 0x80429371 /* V6  isg LONG              */
#define MUIA_Virtgroup_Top                  0x80425200 /* V6  isg LONG              */
#define MUIA_Virtgroup_Width                0x80427c49 /* V6  ..g LONG              */

extern const struct __MUIBuiltinClass _MUI_Virtgroup_desc;

#endif
