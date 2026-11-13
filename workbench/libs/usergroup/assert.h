#ifndef USERGROUP_ASSERT_H
#define USERGROUP_ASSERT_H

#include <proto/intuition.h>
#include <clib/alib_protos.h>

#if defined(LC_LIBDEFS_FILE)
#include LC_LIBDEFS_FILE
#endif

static inline void __inline_InMsg(struct IntuitionBase *IntuitionBase, CONST_STRPTR __arg1, ...)
{
    AROS_SLOWSTACKFORMAT_PRE(__arg1);
    struct EasyStruct libraryES;
    libraryES.es_StructSize = sizeof(libraryES);
    libraryES.es_Flags = 0;
    libraryES.es_Title = (STRPTR)MOD_NAME_STRING;
    libraryES.es_TextFormat = (STRPTR)__arg1;
    libraryES.es_GadgetFormat = "Continue";
    EasyRequestArgs(NULL, &libraryES, NULL, AROS_SLOWSTACKFORMAT_ARG(__arg1));
    AROS_SLOWSTACKFORMAT_POST(__arg1);
}
#define InMsg(...) \
    __inline_InMsg(IntuitionBase, __VA_ARGS__)

#endif /* USERGROUP_ASSERT_H */
