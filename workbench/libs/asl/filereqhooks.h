#ifndef FILEREQHOOKS_H
#define FILEREQHOOKS_H

/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc: File requester specific defs.
    Lang: english
*/

#ifndef LAYOUT_H
#    include "layout.h"
#endif
#ifndef INTUITION_CLASSES_H
#   include <intuition/classes.h>
#endif
#ifndef DOS_EXALL_H
#   include <dos/exall.h>
#endif
#ifndef DOS_DOSEXTENS_H
#   include <dos/dosextens.h>
#endif

#define DEF_PROPWIDTH 20


struct FRUserData
{
    Class		*DirListClass;

    Object		*DirList;
    Object		*Prop;
    
	
    Object		*ButFrame;

    Object		*OKBut;
    Object		*VolumesBut;
    Object		*ParentBut;
    Object		*CancelBut;
    
    struct Gadget	*FileNameGad;
    struct Gadget	*PatternGad;
    struct Gadget	*PathGad;
    
    STRPTR		CurPath;
    ULONG		PathBufSize;
    
    
    UWORD ButWidth;
    UWORD ButHeight;

    UBYTE Flags;
	
};

/* Has the gadgetry been layouted before ? */
#define FRFLG_LAYOUTED (1 << 0)


#endif /* FILEREQHOOKS_H */
