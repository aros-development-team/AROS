#ifndef __BUILTIN_H__
#define __BUILTIN_H__

#ifdef _AROS
#include <aros/asmcall.h>
#endif

struct __MUIBuiltinClass {
    CONST_STRPTR name;
    CONST_STRPTR supername;
    ULONG        datasize;

    AROS_UFP3(IPTR, (*dispatcher),
        AROS_UFPA(Class  *,  cl, A0),
        AROS_UFPA(Object *, obj, A2),
        AROS_UFPA(Msg     , msg, A1));
};

typedef const struct __MUIBuiltinClass _MUIBuiltinClass;

/*
 * builtin classes descriptors, in tree order with creation order :)
 */
extern _MUIBuiltinClass _MUI_Notify_desc; /* 1 */
extern _MUIBuiltinClass  _MUI_Family_desc; /* 2 */
extern _MUIBuiltinClass  _MUI_Application_desc; /* 3 */
extern _MUIBuiltinClass  _MUI_Window_desc; /* 4 */
extern _MUIBuiltinClass  _MUI_Area_desc;  /* 5a */
extern _MUIBuiltinClass   _MUI_Rectangle_desc;  /* 5b */
extern _MUIBuiltinClass   _MUI_Image_desc; /* 8 */
extern _MUIBuiltinClass   _MUI_Text_desc; /* 11 */
extern _MUIBuiltinClass   _MUI_Numeric_desc; /* 12 */
extern _MUIBuiltinClass    _MUI_Slider_desc; /* 13 */
extern _MUIBuiltinClass   _MUI_Group_desc;  /* 5c */
extern _MUIBuiltinClass _MUI_Dataspace_desc; /* 9 */
extern _MUIBuiltinClass  _MUI_Configdata_desc; /* 10 */

Class *_zune_builtin_class_create (CONST_STRPTR className);

#ifndef _AROS
void _zune_add_root_class (void);
#endif

#endif
