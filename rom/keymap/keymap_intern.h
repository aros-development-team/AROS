#ifndef KEYMAP_INTERN_H
#define KEYMAP_INTERN_H
/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
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
#ifndef DEVICES_KEYMAP_H
#   include <devices/keymap.h>
#endif
#include <dos/bptr.h>

#define KEYMAPNAME "keymap.library"

#define DEBUG 0 /* This must be set globally because of cpak */

struct KeymapBase;
extern const UBYTE keymaptype_table[8][8];
extern const UBYTE keymapstr_table[8][8];

/* Structures */
struct BufInfo
{
    UBYTE *Buffer;
    LONG BufLength;
    LONG CharsWritten;
};

struct KeyInfo
{
    UBYTE	Key_MapType; /* KCF_xxx */ 
    
    /* 4 character combo, pointer to string descr, or pointer to deadkey descr,
    ** all ccording to Key_MapType
    */
    IPTR 	Key_Mapping;
    
    UBYTE	KCFQual; /* The qualifiers for the keycode, converted to KCF_xxx format */ 
};


/* Prototypes */
BOOL WriteToBuffer(struct BufInfo *bufinfo, UBYTE *string, LONG numchars);
WORD GetKeyInfo(struct KeyInfo *ki, UWORD code, UWORD qual, struct KeyMap *km);
WORD GetDeadKeyIndex(UWORD code, UWORD qual, struct KeyMap *km);
	
/* Macros */
#define GetBitProperty(ubytearray, idx) \
    ( (ubytearray)[(idx) / 8] & ( 1 << ((idx) & 0x07) ))

/* Get one of the for characters in km_LoKeyMap or km_HiKeyMap addresses,
** id can be 0, 1, 2, 3
*/
#define GetMapChar(key_mapping, idx)    \
	( (key_mapping >> ((3 - (idx)) * 8)) & 0x000000FF )
    
#define KMBase(x) ((struct KeymapBase *)x)


/* Librarybase struct */
struct KeymapBase
{
    struct Library 		 LibNode;
    struct ExecBase		*ExecBase;
    BPTR                         SegList;
    struct KeyMap		*DefaultKeymap;
    struct KeyMapNode		*DefKeymapNode;
    struct KeyMapResource	 KeymapResource;
};


#define SysBase KMBase(KeymapBase)->ExecBase

#endif /* KEYMAP_INTERN_H */
