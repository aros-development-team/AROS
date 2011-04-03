#ifndef DEFINES_EXAMPLE_H
#define DEFINES_EXAMPLE_H

#ifndef  EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

/*
    Defines
*/
#define EXF_TestRequest(title_d1,body,gadgets) \
    AROS_LC3 (ULONG, EXF_TestRequest,  \
    AROS_LCA (UBYTE *, title_d1, D1),  \
    AROS_LCA (UBYTE *, body,     D2),  \
    AROS_LCA (UBYTE *, gadgets,  D3),  \
    struct ExampleBase *, ExampleBase, 5, Example)

#endif /* DEFINES_EXAMPLE_H */
