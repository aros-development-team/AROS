/*
    Copyright � 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef _MUI_CLASSES_REGISTER_H
#define _MUI_CLASSES_REGISTER_H

#define MUIC_Register "Register.mui"

/* Register attributes */
#define MUIA_Register_Frame     (TAG_USER|0x0042349b) /* V7  i.g BOOL   */
#define MUIA_Register_Titles    (TAG_USER|0x004297ec) /* V7  i.g STRPTR */

extern const struct __MUIBuiltinClass _MUI_Register_desc; /* PRIV */

#endif
