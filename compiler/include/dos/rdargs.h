#ifndef DOS_RDARGS_H
#define DOS_RDARGS_H

/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: ReadArgs() definitions
    Lang: english
*/

#ifndef EXEC_NODES_H
#   include <exec/nodes.h>
#endif
#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

struct CSource
{
    UBYTE * CS_Buffer;
    LONG    CS_Length;
    LONG    CS_CurChr;
};

struct RDArgs
{
    struct CSource RDA_Source;
    LONG           RDA_DAList;
    UBYTE        * RDA_Buffer;
    LONG           RDA_BufSiz;
    UBYTE        * RDA_ExtHelp;
    LONG           RDA_Flags;
};

#define RDAB_STDIN         0
#define RDAF_STDIN    (1L<<0)
#define RDAB_NOALLOC       1
#define RDAF_NOALLOC  (1L<<1)
#define RDAB_NOPROMPT      2
#define RDAF_NOPROMPT (1L<<2)

#define MAX_TEMPLATE_ITEMS 100
#define MAX_MULTIARGS      128

#endif /* DOS_RDARGS_H */
