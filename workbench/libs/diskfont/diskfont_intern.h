#ifndef DISKFONT_INTERN_H
#define DISKFONT_INTERN_H

#ifndef AROS_ALMOST_COMPATIBLE
#define AROS_ALMOST_COMPATIBLE
#endif

#ifndef   PROTO_EXEC_H
#include  <proto/exec.h>
#endif

#ifndef   PROTO_GRAPHICS_H
#include  <proto/graphics.h>
#endif

#ifndef   PROTO_UTILITY_H
#include  <proto/utility.h>
#endif

#ifndef   PROTO_DOS_H
#include  <proto/dos.h>
#endif

#ifndef   PROTO_ALIB_H
#include  <proto/alib.h>
#endif

#ifndef		PROTO_AROSSUPPORT_H
#include	<proto/arossupport.h>
#endif

#ifndef   GRAPHICS_TEXT_H
#include  <graphics/text.h>
#endif

#ifndef   GRAPHICS_GFXBASE_H
#include  <graphics/gfxbase.h>
#endif

#ifndef   DISKFONT_DISKFONT_H
#include  <diskfont/diskfont.h>
#endif

#ifndef   EXEC_MEMORY_H
#include  <exec/memory.h>
#endif

#ifndef   DOS_DOS_H
#include  <dos/dos.h>
#endif

#ifndef   STRING_H
#include  <string.h>
#endif

#ifndef		AROS_LIBCALL_H
#include	<aros/libcall.h>
#endif



/**************/
/*            */
/* AVAILFONTS */
/*            */
/**************/


/**************/
/* Constants  */
/**************/


/* Flags for the FontInfoNode->Flags field */


#define FDF_REUSENAME   (1 << 0)
#define FDF_REUSETAGS   (1 << 1)
#define FDF_USEDEFTAGS  (1 << 2)

/* Possible returnvalues for AvailFonts hooks
( or an or'ed combination of these )
*/

#define FH_SUCCESS      (1 << 0)
#define FH_SCANFINISHED (1 << 1)

#define FH_REUSENAME    (1 << 2)
#define FH_REUSETAGS    (1 << 3)

/* Non-fatal error */
#define FH_SINGLEERROR  (1 << 4)

/* Some states in the ScanFontInfo routine */
#define SFI_NEWDESCRNODE  0
#define SFI_READDESCR     1

/* The different AvailFonts hook commands */

#define FHC_INIT          0
#define FHC_READFONTINFO  1
#define FHC_CLEANUP       2
#define FHC_GETDATE       3


/* ID for cache-file */
#define CACHE_IDSTR "AROS_FC"

/* Path to the cachefile */
#define CACHE_FILE "FONTS:cachefile"

/* Structure for storing the fontnames into a linked list */

struct FontNameNode
{
    
    struct MinNode NodeHeader;
    STRPTR FontName;
    STRPTR FontNameInBuf; /* points to the same fontname inside the buf */
};
  
/* Structure for storing taglists into a linked list */
struct FontTagsNode
{
    struct MinNode  NodeHeader;
    struct TagItem  *TagList;
    struct TagItem  *TagListInBuf; /* points to th same taglist inside the buf */
};

/* Structure for storing TAvailFonts elements */
struct FontInfoNode
{
    struct MinNode NodeHeader;
  
    struct TAvailFonts  TAF;
    
    /* ored combo of FDF_REUSENAME, FDF_REUSETAGS and FDF_USEDEFAULTTAGS */
    UBYTE   Flags;
  
    /* Pointer to this font's name and eventual tags.
    */
    struct FontNameNode *FontName;
    struct FontTagsNode *FontTags;

};


/* Structure describing the hooks that shall be used
for reading font descriptions in AvailFonts()
*/

struct AFHookDescr
{
    /* Flags that must match the flags input to AvailFonts
    for this hook to be executed. (AFF_MEMORY, AFF_DISK, etc..)
    */
    
    ULONG         ahd_Flags;
    struct Hook   *ahd_Hook;
};



/* Hook structure message structure for sending data to 
the font description reading routines */

struct FontHookCommand
{
    ULONG                 fhc_Command;
    ULONG                 fhc_Flags;
    struct FontInfoNode   *fhc_FINode;

    /* This field can be filled out by the hook. It will not be changed outside the hook */
    APTR                  fhc_UserData;
};

/* The .font files are loaded into these. */

struct FontDescr
{
    STRPTR          FontName;
    UWORD           NumTags;
    struct TagItem  *Tags;
    
    UWORD           YSize;
    UBYTE           Style;
    UBYTE           Flags;        
};
    
struct FontDescrHeader
{
    UWORD   NumEntries;
    BOOL    Tagged;
    struct  FontDescr *FontDescrArray;
};


/* Userdata needed by the MemoryFontHook */
struct MemoryFontHook_Data
{
    /* Pointer to the current font in the memory font list */
    struct TextFont *mfhd_CurrentFont;
};




struct DiskFontHook_Data
{
    BPTR                    dfhd_DirLock;
    BPTR                    dfhd_OldDirLock;
    struct FontDescrHeader  *dfhd_CurrentFDH;
    /* Index into the current FontDescr-array */
    UWORD                   dfhd_FontDescrIndex;

    struct FileInfoBlock    *dfhd_CurrentFIB;
    /* Specifies if we should use ExNext instead of Examine */
    BOOL                    dfhd_UseExNext;
    
    
};


/* States we can be in if the buffer is filled */
#define BFSTATE_FONTINFO    0
#define BFSTATE_FONTNAME    1
#define BFSTATE_FONTTAGS    2
#define BFSTATE_DEFAULTTAGS 3


/* Structure that keeps list needed by AvailFonts, and
  also keeps the current state of the process of
   */

struct AF_Lists
{
    struct MinList      FontInfoList;
    struct MinList      FontNameList;
    struct MinList      FontTagsList;
  
    /* What node was currently being written when buffer was full ? */
    struct MinNode      *BufferFullNode;
    
    /* What state were we in when the buffer was full ? */
    UWORD               BufferFullState;
    
    /* Pointer into where we should start simulating copying into the buffer
    for reading bytes needed 
    */
    APTR                BufferFullPtr;
    
    struct TagItem      *DefTagsInBuf;

    
    APTR                CurrentBufPtr;
};





/**************/
/* Prototypes */
/**************/

struct DiskfontBase_intern; /* prerefrence */


BOOL  ScanFontInfo(ULONG, struct MinList *, struct MinList *, struct MinList *, struct DiskfontBase_intern *);

struct FontNameNode *AllocFontNameNode( STRPTR,          	struct DiskfontBase_intern *);
struct FontTagsNode *AllocFontTagsNode( struct TagItem *,	struct DiskfontBase_intern *);

BOOL  CopyDescrToBuffer (UBYTE *, ULONG, ULONG, struct AF_Lists *, struct DiskfontBase_intern *);
ULONG CountBytesNeeded  (UBYTE *, ULONG, struct AF_Lists *, struct DiskfontBase_intern *);
VOID  UpdatePointers    (UBYTE *, ULONG, struct AF_Lists *, struct DiskfontBase_intern *);

/* Functions for .font file I/O */

struct FontDescrHeader *ReadFontDescr(BPTR,		struct DiskfontBase_intern *);
VOID FreeFontDescr(struct FontDescrHeader *,	struct DiskfontBase_intern *);


IPTR MemoryFontFunc ( struct Hook *,struct FontHookCommand *, struct DiskfontBase_intern *);
IPTR DiskFontFunc   ( struct Hook *,struct FontHookCommand *, struct DiskfontBase_intern *);


ULONG NumTags(struct TagItem *, struct DiskfontBase_intern *);
ULONG CopyTagItems(struct TagItem *, struct TagItem *, struct DiskfontBase_intern *);

struct TagItem *ReadTags  (BPTR, ULONG, struct DiskfontBase_intern *);
BOOL  WriteTags(BPTR, struct TagItem *, struct DiskfontBase_intern *);

BOOL ReadCache(ULONG, struct AF_Lists *, struct DiskfontBase_intern *);
BOOL WriteCache(struct AF_Lists *, struct DiskfontBase_intern *);

BOOL OKToReadCache(struct DiskfontBase_intern *);

VOID FreeAFLists(struct AF_Lists *, struct DiskfontBase_intern *);

AROS_UFP3(LONG, dosstreamhook,
    AROS_UFPA(struct Hook *,   hook, A0),
    AROS_UFPA(BPTR,            file, A2),
    AROS_UFPA(ULONG *,         msg, A1)
);

/********************/
/* Some nice macros */
/********************/

#undef	AFH 
#define AFH(p)  ((struct AvailFontsHeader *)p)

#undef	AVF
#define AVF(p)  ((struct AvailFonts       *)p)

#undef	TAVF
#define TAVF(p) ((struct TAvailFonts      *)p)

#undef	FIN
#define FIN(n)  ((struct FontInfoNode     *)n)

#undef	FNN
#define FNN(n)  ((struct FontNameNode     *)n)

#undef	FTN
#define FTN(n)  ((struct FontTagsNode     *)n)

#undef	TI
#define TI(t)   ((struct TagItem          *)t)

#undef	UB
#define UB(p)   ((UBYTE                   *)p)

#undef	TFE
#define	TFE(t)	((struct TextFontExtension*)t)

/* Some external stuff (diskfont_init.c) */

/* Internal prototypes */


struct DiskfontBase_intern
{
    struct Library    library;
    struct ExecBase * sysbase;
    BPTR              seglist;

    struct Library       * dosbase;
    struct GfxBase       * gfxbase;
    struct Library       * utilitybase;
    
    /* dosstreamhandler hook neede for endian io funcs */
    struct Hook		dsh;
};

/* The following typedefs are necessary, because the names of the global
   variables storing the library base pointers  and the corresponding
   structs are equal.
   This is a hack, of course. */
typedef struct GfxBase GraphicsBase;

#define DFB(dfb)        ((struct DiskfontBase_intern *)dfb)
#undef SysBase
#define SysBase (DFB(DiskfontBase)->sysbase)
#undef DOSBase
#define DOSBase (DFB(DiskfontBase)->dosbase)
#undef GfxBase
#define GfxBase (DFB(DiskfontBase)->gfxbase)
#undef UtilityBase
#define UtilityBase (DFB(DiskfontBase)->utilitybase)

#define expunge() \
AROS_LC0(BPTR, expunge, struct DiskfontBase_intern *, DiskfontBase, 3, Diskfont)


#endif /* diskfont_intern.h */