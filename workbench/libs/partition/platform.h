#ifndef PLATFORM_H
#define PLATFORM_H

#include "compilerspecific.h"
#include "arosmacros.h"

#ifdef __AMIGAOS__
#define NEWLIST(_l)                              \
do                                               \
{                                                \
    struct List *l = (struct List *)(_l);        \
                                                 \
    l->lh_TailPred = (struct Node *)l;           \
    l->lh_Tail     = 0;                          \
    l->lh_Head     = (struct Node *)&l->lh_Tail; \
} while (0)
#endif

#endif /* PLATFORM_H */

