#ifndef KEYMAP_INTERN_H
#define KEYMAP_INTERN_H
/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Keymaps internal structure
    Lang: english
*/
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif
#ifndef EXEC_EXECBASE_H
#   include <exec/execbase.h>
#endif
#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

#define KEYMAPNAME "keymap.library"

#define DEBUG 0 /* This must be set globally because of cpak */

struct KeymapBase;
extern const UBYTE keymaptype_table[8][8];

/* Structures */
struct BufInfo
{
    UBYTE *Buffer;
    LONG BufLength;
    LONG CharsWritten;
};

/* Prototypes */
BOOL WriteToBuffer(struct BufInfo *bufinfo, UBYTE *string, LONG numchars);

/* Macros */
#define GetBitProperty(ubytearray, idx) \
    ((ubytearray)[(idx) >> 8] & ((idx) & 0x07))
    
#define KMBase(x) ((struct KeymapBase *)x)


/* Librarybase struct */
struct KeymapBase
{
    struct Library 	LibNode;
    struct ExecBase	*SysBase;
    struct KeyMap	*DefaultKeymap;
};

#define SysBase KMBase(KeymapBase)->SysBase

/* Needed for close() */
#define expunge() \
    AROS_LC0(BPTR, expunge, struct KeymapBase *, KeymapBase, 3, Keymap)


#endif /* KEYMAP_INTERN_H */
