/*
    Copyright � 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef DISKFONT_INTERN_H
#define DISKFONT_INTERN_H

#ifndef PROTO_EXEC_H
#   include  <proto/exec.h>
#endif
#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif
#ifndef DISKFONT_DISKFONT_H
#   include  <diskfont/diskfont.h>
#endif
#ifndef EXEC_MEMORY_H
#   include  <exec/memory.h>
#endif
#ifndef EXEC_LISTS_H
#   include  <exec/lists.h>
#endif
#include <exec/semaphores.h>
#ifndef AROS_ASMCALL_H
#   include <aros/asmcall.h>
#endif
#ifndef UTILITY_HOOKS_H
#   include <utility/hooks.h>
#endif
#ifndef DOS_DOS_H
#   include <dos/dos.h>
#endif
#include <aros/libcall.h>

/* Options */

#define ALWAYS_ZERO_LIBCOUNT 1

/**************/
/*	      */
/* AVAILFONTS */
/*	      */
/**************/


/**************/
/* Constants  */
/**************/

/* Number of font hooks used */
#define NUMFONTHOOKS 2

#define FONTTYPE_ROMFONT     FPF_ROMFONT
#define FONTTYPE_DISKFONT    FPF_DISKFONT
#define FONTTYPE_SCALEDFONT  0
#define FONTTYPE_OUTLINEFONT (FPF_ROMFONT | FPF_DISKFONT)
#define FONTTYPE_FLAGMASK    (FPF_ROMFONT | FPF_DISKFONT)

#define IS_SCALED_FONT(ta) (((ta)->tta_Flags & FONTTYPE_FLAGMASK) == FONTTYPE_SCALEDFONT)
#define IS_OUTLINE_FONT(ta) (((ta)->tta_Flags & FONTTYPE_FLAGMASK) == FONTTYPE_OUTLINEFONT)

/* ID for cache-file */
#ifdef __MORPHOS__
#define CACHE_IDSTR "MORPHOS_FC"
#else
#define CACHE_IDSTR "AROS_FC"
#endif

/* Path to the cachefile */
#ifdef __MORPHOS__
#define CACHE_FILE      "FONTS:_fontcache"
#else
#define CACHE_FILE      "fontcache"
#endif
#define PROGDIRFONTSDIR "PROGDIR:Fonts/"
#define FONTSDIR        "FONTS:"

struct OTagList
{
    STRPTR		filename;
    struct TagItem	*tags;
    ULONG		data[1];
};

struct FontDescrHeader
{
    UWORD		NumOutlineEntries;
    UWORD		NumEntries;
    UWORD		ContentsID;
    BOOL		Tagged;
    struct  TTextAttr	*TAttrArray;
    struct  OTagList	*OTagList;
};




struct AADiskFontHeader
{
    struct Node			dfh_DF;
    UWORD			dfh_FileID;
    UWORD			dfh_Revision;
    BPTR			dfh_Segment;
    char			dfh_Name[MAXFONTNAME];
    struct ColorTextFont	dfh_TF;
};



/**************/
/* Prototypes */
/**************/

struct DiskfontBase; /* prerefrence */

/* memoryfontfunc.c */

APTR MF_IteratorInit(struct DiskfontBase *);
struct TTextAttr *MF_IteratorGetNext(APTR, struct DiskfontBase *);
VOID MF_IteratorFree(APTR, struct DiskfontBase *);

/* diskfontfunc.c */

VOID CleanUpFontsDirEntryList(struct DiskfontBase *);
APTR DF_IteratorInit(struct TTextAttr *, struct DiskfontBase *);
struct TTextAttr *DF_IteratorGetNext(APTR, struct DiskfontBase *);
VOID DF_IteratorRemember(APTR, struct DiskfontBase *);
struct TextFont *DF_IteratorRememberOpen(APTR, struct DiskfontBase *);
VOID DF_IteratorFree(APTR, struct DiskfontBase *);
struct TextFont *DF_OpenFontPath(struct TextAttr *, struct DiskfontBase *);

/* diskfont_io.c */

struct DiskFontHeader *ConvDiskFont(BPTR, CONST_STRPTR, BOOL, struct DiskfontBase *);
void DisposeConvDiskFont(struct DiskFontHeader *, struct DiskfontBase *);
struct TextFont *ReadDiskFont(struct TTextAttr *, CONST_STRPTR, struct DiskfontBase *);

/* af_fontdescr_io.c */

struct FontDescrHeader *ReadFontDescr(CONST_STRPTR, struct DiskfontBase *);
VOID FreeFontDescr(struct FontDescrHeader *, struct DiskfontBase *);

/* dosstreamhook.c */

AROS_UFP3(LONG, dosstreamhook,
    AROS_UFPA(struct Hook *,   hook, A0),
    AROS_UFPA(BPTR,            file, A2),
    AROS_UFPA(ULONG *,         msg, A1)
);

/* bullet.c */

STRPTR OTAG_MakeFileName(CONST_STRPTR, struct DiskfontBase *);
VOID OTAG_FreeFileName(STRPTR, struct DiskfontBase *);
struct OTagList *OTAG_GetFile(CONST_STRPTR, struct DiskfontBase *);
VOID OTAG_KillFile(struct OTagList *, struct DiskfontBase *);
UBYTE OTAG_GetFontStyle(struct OTagList *, struct DiskfontBase *);
UBYTE OTAG_GetSupportedStyles(struct OTagList *, struct DiskfontBase *);
UBYTE OTAG_GetFontFlags(struct OTagList *, struct DiskfontBase *);
struct TextFont *OTAG_ReadOutlineFont(struct TTextAttr *, struct TTextAttr *, struct OTagList *, struct DiskfontBase *);

/* basicfuncs.c */

#define MAKE_REAL_SEGMENT(x) (MKBADDR(((IPTR)(x)) - sizeof(BPTR)))

APTR AllocSegment(BPTR *, ULONG, ULONG, struct DiskfontBase *);
struct TagItem *ReadTags(BPTR, ULONG, struct DiskfontBase *);
struct TagItem *ReadTagsNum(BPTR, ULONG *, struct DiskfontBase *);
/*BOOL WriteTags(BPTR, struct TagItem *, struct DiskfontBase *);*/
BOOL WriteTagsNum(BPTR, const struct TagItem *, struct DiskfontBase *);
ULONG NumTags(const struct TagItem *, struct DiskfontBase *);
ULONG CopyTagItems(struct TagItem *, const struct TagItem *, struct DiskfontBase *);


/********************/
/* Some nice macros */
/********************/

#undef	AFH
#define AFH(p)  ((struct AvailFontsHeader *)p)

#undef	AVF
#define AVF(p)  ((struct AvailFonts       *)p)

#undef	TAVF
#define TAVF(p) ((struct TAvailFonts      *)p)

#undef	TI
#define TI(t)   ((struct TagItem          *)t)

#undef	UB
#define UB(p)   ((UBYTE                   *)p)

#undef	TFE
#define TFE(t)  ((struct TextFontExtension*)t)

/* Some external stuff (diskfont_init.c) */

/* Internal prototypes */

struct DiskfontBase
{
    struct Library	   lib;

    /* dosstreamhandler hook neede for endian io funcs */
    struct Hook		   dsh;
    struct List		   diskfontlist;
    
    struct MinList         fontsdirentrylist;
    struct SignalSemaphore fontssemaphore;
    
    /* MemHandler interrupt for flushing library */
    struct Interrupt       memint;
};

#define DFB(dfb)        ((struct DiskfontBase *)dfb)

#endif /* diskfont_intern.h */
