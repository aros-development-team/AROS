#ifndef LIBRARIES_IFFPARSE_H
#define LIBRARIES_IFFPARSE_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Definitions for iffparse.library
    Lang: english
*/

#ifndef EXEC_LISTS_H
#   include <exec/lists.h>
#endif
#ifndef EXEC_PORTS_H
#   include <exec/ports.h>
#endif
#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef DEVICES_CLIPBOARD_H
#   include <devices/clipboard.h>
#endif
#ifndef AROS_MACROS_H
#   include <aros/macros.h>
#endif

struct IFFHandle
{
    IPTR  iff_Stream;
    ULONG iff_Flags;  /* see below */
    LONG  iff_Depth;
};

/* iff_Flags */
#define IFFF_READ     0L
#define IFFF_WRITE    (1L<<0)
#define IFFF_RWBITS   (IFFF_READ | IFFF_WRITE)
#define IFFF_FSEEK    (1L<<1)
#define IFFF_RSEEK    (1L<<2)
#define IFFF_RESERVED 0xFFFF0000

struct IFFStreamCmd
{
    LONG sc_Command;
    APTR sc_Buf;
    LONG sc_NBytes;
};

#define IFFCMD_INIT     0
#define IFFCMD_CLEANUP  1
#define IFFCMD_READ     2
#define IFFCMD_WRITE    3
#define IFFCMD_SEEK     4
#define IFFCMD_ENTRY    5
#define IFFCMD_EXIT     6
#define IFFCMD_PURGELCI 7

struct ContextNode
{
    struct MinNode cn_Node;

    LONG cn_ID;
    LONG cn_Type;
    LONG cn_Size;
    LONG cn_Scan;
};

struct LocalContextItem
{
    struct MinNode lci_Node;

    ULONG lci_ID;
    ULONG lci_Type;
    ULONG lci_Ident;
};

struct StoredProperty
{
    LONG sp_Size;
    APTR sp_Data;
};

struct CollectionItem
{
    struct CollectionItem * ci_Next;
    LONG                    ci_Size;
    APTR                    ci_Data;
};

struct ClipboardHandle
{
    struct IOClipReq cbh_Req;
    struct MsgPort   cbh_CBport;
    struct MsgPort   cbh_SatisfyPort;
};

/* ParseIFF() */
#define IFFPARSE_SCAN    0L
#define IFFPARSE_STEP    1L
#define IFFPARSE_RAWSTEP 2L

/* StoreLocalItem() */
#define IFFSLI_ROOT 1L
#define IFFSLI_TOP  2L
#define IFFSLI_PROP 3L

#define IFFSIZE_UNKNOWN -1L

/* Errors */
#define IFFERR_EOF        -1L
#define IFFERR_EOC        -2L
#define IFFERR_NOSCOPE    -3L
#define IFFERR_NOMEM      -4L
#define IFFERR_READ       -5L
#define IFFERR_WRITE      -6L
#define IFFERR_SEEK       -7L
#define IFFERR_MANGLED    -8L
#define IFFERR_SYNTAX     -9L
#define IFFERR_NOTIFF     -10L
#define IFFERR_NOHOOK     -11L
#define IFF_RETURN2CLIENT -12L

#define MAKE_ID(a,b,c,d) AROS_MAKE_ID((a),(b),(c),(d))

#define ID_FORM MAKE_ID('F','O','R','M')
#define ID_LIST MAKE_ID('L','I','S','T')
#define ID_CAT  MAKE_ID('C','A','T',' ')
#define ID_PROP MAKE_ID('P','R','O','P')
#define ID_NULL MAKE_ID(' ',' ',' ',' ')

#define IFFLCI_PROP         MAKE_ID('p','r','o','p')
#define IFFLCI_COLLECTION   MAKE_ID('c','o','l','l')
#define IFFLCI_ENTRYHANDLER MAKE_ID('e','n','h','d')
#define IFFLCI_EXITHANDLER  MAKE_ID('e','x','h','d')

#endif /* LIBRARIES_IFFPARSE_H */
