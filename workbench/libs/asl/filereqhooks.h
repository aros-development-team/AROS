#ifndef FILEREQHOOKS_H
#define FILEREQHOOKS_H

/*
    (C) 1997 AROS - The Amiga Replacement OS
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

/* Including \0 */
#define FILENAMEBUFSIZE 10
#define DATEBUFSIZE 	20
#define DISPHOOKBUFSIZE FILENAMEBUFSIZE + DATEBUFSIZE


#define DEF_PROPWIDTH 20

struct FRUserData
{

    Object		*FileList;
    Object		*VolumesList;
    Object		*Listview;
    Object		*Prop;
    
    /* Neede by displayhook for re-entrancy */
    STRPTR		DispHookBuf;
	
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
    
    struct Hook		FLConstructHook;
    struct Hook		FLDestructHook;
    struct Hook		FLDisplayHook;
    
    struct Hook		VLConstructHook;
    struct Hook		VLDestructHook;
    struct Hook		VLDisplayHook;

    
    
    UWORD ButWidth;
    UWORD ButHeight;

    UBYTE Flags;
	
};

/* Has the gadgetry been layouted before ? */
#define FRFLG_LAYOUTED (1 << 0)
/* Filelist or volumeslist current list ? */
#define FRFLG_FILELIST (1 << 1)


struct VolumeInfo
{
    STRPTR	vi_Name;
    LONG	vi_Type;
};


BOOL GetDir(STRPTR, Object *, struct AslBase_intern *);
BOOL GetVolumes(Object *, struct AslBase_intern *);


AROS_UFP3(VOID, FLDisplayHook,
    AROS_UFPA(struct Hook *,		hook,		A0),
    AROS_UFPA(STRPTR *,			dharray,	A2),
    AROS_UFPA(struct ExAllData *, 	ead,		A1)
);
AROS_UFP3(VOID, VLDisplayHook,
    AROS_UFPA(struct Hook *,		hook,		A0),
    AROS_UFPA(STRPTR *,			dharray,	A2),
    AROS_UFPA(struct VolumeInfo *, 	vi,		A1)
);

AROS_UFP3(APTR, VLConstructHook,
    AROS_UFPA(struct Hook *,		hook,		A0),
    AROS_UFPA(APTR,			pool,		A2),
    AROS_UFPA(struct DosList *,		dlist,		A1)
);
AROS_UFP3(VOID, VLDestructHook,
    AROS_UFPA(struct Hook *,		hook,		A0),
    AROS_UFPA(APTR,			pool,		A2),
    AROS_UFPA(struct VolumeInfo *,	vi,		A1)
);
AROS_UFP3(APTR, FLConstructHook,
    AROS_UFPA(struct Hook *,		hook,		A0),
    AROS_UFPA(APTR,			pool,		A2),
    AROS_UFPA(struct ExAllData *,	ead,		A1)
);

AROS_UFP3(VOID, FLDestructHook,
    AROS_UFPA(struct Hook *,		hook,		A0),
    AROS_UFPA(APTR,			pool,		A2),
    AROS_UFPA(struct ExAllData *,	ead,		A1)
);

#endif /* FILEREQHOOKS_H */
