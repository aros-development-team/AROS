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
#define ArosInquire(query) \
    AROS_LC1(IPTR, ArosInquire, \
    AROS_LCA(ULONG, query, D0), \
    struct ArosBase *, ArosBase, 5, Aros)


#endif /* DEFINES_AROS_H */
