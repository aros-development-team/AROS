#ifndef DEFINES_AROS_H
#define DEFINES_AROS_H

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

/*
    Defines
*/
#define ArosInquireA(taglist) \
    AROS_LC1(ULONG, ArosInquireA, \
    AROS_LCA(struct TagItem *, taglist, A0), \
    struct ArosBase *, ArosBase, 5, Aros)


#endif /* DEFINES_AROS_H */
