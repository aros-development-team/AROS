#ifndef _MUI_CLASSES_REGISTER_H
#define _MUI_CLASSES_REGISTER_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Register           "Register.mui"

/*** Identifier base  (for Zune extensions) *********************************/
#define MUIB_Register           (MUIB_ZUNE | 0x00002c00)

/*** Attributes *************************************************************/
#define MUIA_Register_Frame     (MUIB_MUI|0x0042349b) /* V7  i.g BOOL     */
#define MUIA_Register_Titles    (MUIB_MUI|0x004297ec) /* V7  i.g STRPTR * */

#define MUIA_Register_Columns   (MUIB_Register | 0x0000) /* Zune V1  i..  */


extern const struct __MUIBuiltinClass _MUI_Register_desc; /* PRIV */

#endif /* _MUI_CLASSES_REGISTER_H */
