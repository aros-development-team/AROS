/* Host stand-in for <exec/nodes.h>: only what __optionallibs.c and
   __exitfunc.h use. */
#ifndef EXEC_NODES_H
#define EXEC_NODES_H
#include <stddef.h>
typedef unsigned char UBYTE;
struct Node {
    struct Node *ln_Succ;
    struct Node *ln_Pred;
    UBYTE        ln_Type;
    const char  *ln_Name;
};
#endif
