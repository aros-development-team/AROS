#ifndef FONTREQHOOKS_H
#define FONTREQHOOKS_H

/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Font requester specific defs.
    Lang: english
*/
#ifndef LAYOUT_H
#    include "layout.h"
#endif
#ifndef INTUITION_CLASSES_H
#   include <intuition/classes.h>
#endif
#ifndef DISKFONT_DISKFONT_H
#   include <diskfont/diskfont.h>
#endif
struct FOUserData
{
    Object *NameList;
    Object *SizeList;
    Object *NameListview;
    Object *SizeListview;
    
    Object *OKBut;
    Object *CancelBut;
    
    Object *Prop;
    Object *ButFrame;
    
    struct Hook NameDisplayHook;
    struct Hook SizeDisplayHook;
    struct Hook NameConstructHook;
    struct Hook NameDestructHook;
    
    struct AvailFontsHeader *AFBuf;
    
    STRPTR DispHookBuf;
    
    UWORD  ButHeight;
    UWORD  ButWidth;
    
    UBYTE  Flags;
    
};

#define FOFLG_LAYOUTED (1 << 0)


struct NameEntry
{
    STRPTR	ne_FontName; /* The fontname to be showed in the listview */
    UWORD	ne_AFOffset; /* The idx of that special AvailFonts struct */
};

#endif /* FONTREQHOOKS_H */
