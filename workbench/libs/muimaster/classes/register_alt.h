#ifndef _MUI_CLASSES_REGISTER_H
#define _MUI_CLASSES_REGISTER_H

#ifdef _DCC
extern char MUIC_Register[];
#else
#define MUIC_Register "Register.mui"
#endif

/* Attributes */

#define MUIA_Register_Frame                 0x8042349b /* V7  i.g BOOL              */
#define MUIA_Register_Titles                0x804297ec /* V7  i.g STRPTR *          */

extern const struct __MUIBuiltinClass _MUI_Register_desc;

#endif
