#ifndef _CLASSES_FAMILY_H
#define _CLASSES_FAMILY_H

#ifndef EXEC_LISTS_H
#include <exec/lists.h>
#endif

struct MUI_FamilyData
{
    struct List childs;
};

#ifdef _DCC
extern char MUIC_Family[];
#else
#define MUIC_Family "Family.mui"
#endif

/* Methods */

#define MUIM_Family_AddHead                 0x8042e200 /* V8  */
#define MUIM_Family_AddTail                 0x8042d752 /* V8  */
#define MUIM_Family_Insert                  0x80424d34 /* V8  */
#define MUIM_Family_Remove                  0x8042f8a9 /* V8  */
#define MUIM_Family_Sort                    0x80421c49 /* V8  */
#define MUIM_Family_Transfer                0x8042c14a /* V8  */
struct  MUIP_Family_AddHead                 { ULONG MethodID; Object *obj; };
struct  MUIP_Family_AddTail                 { ULONG MethodID; Object *obj; };
struct  MUIP_Family_Insert                  { ULONG MethodID; Object *obj; Object *pred; };
struct  MUIP_Family_Remove                  { ULONG MethodID; Object *obj; };
struct  MUIP_Family_Sort                    { ULONG MethodID; Object *obj[1]; };
struct  MUIP_Family_Transfer                { ULONG MethodID; Object *family; };

/* Attributes */

#define MUIA_Family_Child                   0x8042c696 /* V8  i.. Object *          */
#define MUIA_Family_List                    0x80424b9e /* V8  ..g struct MinList *  */

extern const struct __MUIBuiltinClass _MUI_Family_desc;

#endif
