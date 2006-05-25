/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$

    Library code for AmigaOS, based on Dirk Stöckers Libary
    Example
*/

#define BASE_GLOBAL

#include <proto/exec.h>
#include <exec/resident.h>
#include <exec/initializers.h>
#include <intuition/intuitionbase.h>
#include <exec/execbase.h>

struct ExecBase      *SysBase;

LONG ReturnError2(void)
{
  return -1;
}

#include "muimaster_intern.h"

#define VERSION   1
#define REVISION  0
#define DATETXT	  "27.06.2003"
#define VERSTXT	  "1.0"

#define LIBNAME  "zunemaster.library"
#define IDSTRING "zunemaster.library " VERSTXT " (" DATETXT ")\r\n"

#define MYDEBUG 1
#include "debug.h"

typedef BPTR SEGLISTPTR;
#define LC_LIBHEADERTYPEPTR struct Library *

/************************************************************************/

/* First executable routine of this library; must return an error
   to the unsuspecting caller */
LONG ReturnError(void)
{
  return -1;
}

/************************************************************************/

/* MUI private functions */

__asm void MUI_Priv1(register __a6 struct Library *MUIMasterBase)
{
	D(bug("MUI_Priv1() called"));
}

__asm void MUI_Priv2(register __a6 struct Library *MUIMasterBase)
{
	D(bug("MUI_Priv2() called"));
}

__asm void MUI_Priv3(register __a6 struct Library *MUIMasterBase)
{
	D(bug("MUI_Priv3() called"));
}

__asm void MUI_Priv4(register __a6 struct Library *MUIMasterBase)
{
	D(bug("MUI_Priv4() called"));
}

/************************************************************************/

/* Some functions which are somewhere else */

ULONG SAVEDS STDARGS LC_BUILDNAME(L_InitLib) (LC_LIBHEADERTYPEPTR MUIMasterBase);
ULONG SAVEDS STDARGS LC_BUILDNAME(L_OpenLib) (LC_LIBHEADERTYPEPTR MUIMasterBase);
void  SAVEDS STDARGS LC_BUILDNAME(L_CloseLib) (LC_LIBHEADERTYPEPTR MUIMasterBase);
void  SAVEDS STDARGS LC_BUILDNAME(L_ExpungeLib) (LC_LIBHEADERTYPEPTR MUIMasterBase);

/************************************************************************/

struct LibInitData {
 UBYTE i_Type;     UBYTE o_Type;     UBYTE  d_Type;	UBYTE p_Type;
 UBYTE i_Name;     UBYTE o_Name;     STRPTR d_Name;
 UBYTE i_Flags;    UBYTE o_Flags;    UBYTE  d_Flags;	UBYTE p_Flags;
 UBYTE i_Version;  UBYTE o_Version;  UWORD  d_Version;
 UBYTE i_Revision; UBYTE o_Revision; UWORD  d_Revision;
 UBYTE i_IdString; UBYTE o_IdString; STRPTR d_IdString;
 ULONG endmark;
};

/************************************************************************/
extern const ULONG LibInitTable[4]; /* the prototype */

/* The library loader looks for this marker in the memory
   the library code and data will occupy. It is responsible
   setting up the Library base data structure. */
const struct Resident RomTag = {
  RTC_MATCHWORD,                   /* Marker value. */
  (struct Resident *)&RomTag,      /* This points back to itself. */
  (struct Resident *)LibInitTable, /* This points somewhere behind this marker. */
  RTF_AUTOINIT,                    /* The Library should be set up according to the given table. */
  VERSION,                         /* The version of this Library. */
  NT_LIBRARY,                      /* This defines this module as a Library. */
  0,                               /* Initialization priority of this Library; unused. */
  LIBNAME,                         /* Points to the name of the Library. */
  IDSTRING,                        /* The identification string of this Library. */
  (APTR)&LibInitTable              /* This table is for initializing the Library. */
};

/************************************************************************/

/* The mandatory reserved library function */
ULONG LibReserved(void)
{
  return 0;
}

/* Open the library, as called via OpenLibrary() */
ASM struct Library *LibOpen(REG(a6, struct MUIMasterBase_intern * MUIMasterBase))
{
  /* Prevent delayed expunge and increment opencnt */
  MUIMasterBase->library.lib_Flags &= ~LIBF_DELEXP;
  MUIMasterBase->library.lib_OpenCnt++;

  return &MUIMasterBase->library;
}

/* Expunge the library, remove it from memory */
ASM SEGLISTPTR LibExpunge(REG(a6, struct MUIMasterBase_intern *mb))
{
  if (!mb->library.lib_OpenCnt)
  {
    SEGLISTPTR seglist;

    seglist = mb->seglist;

    L_ExpungeLib(&mb->library);

    /* Remove the library from the public list */
    Remove((struct Node *)mb);

    /* Free the vector table and the library data */
    FreeMem((STRPTR) mb - mb->library.lib_NegSize,
    mb->library.lib_NegSize +
    mb->library.lib_PosSize);

    return seglist;
  }
  else
    mb->library.lib_Flags |= LIBF_DELEXP;

  /* Return the segment pointer, if any */
  return 0;
}

/* Close the library, as called by CloseLibrary() */
ASM SEGLISTPTR LibClose(REG(a6, struct MUIMasterBase_intern *mb))
{
  if(!(--mb->library.lib_OpenCnt))
  {
    if (mb->library.lib_Flags & LIBF_DELEXP)
      return LibExpunge(mb);
  }
  return 0;
}

#undef SysBase
extern struct ExecBase *SysBase;

/* Initialize library */
ASM struct Library *LibInit(REG(a0, SEGLISTPTR seglist), REG(d0, struct MUIMasterBase_intern *mb), REG(a6, struct ExecBase *sysbase))
{
#ifdef _M68060
  if(!(sysbase->AttnFlags & AFF_68060))
    return 0;
#elif defined (_M68040)
  if(!(sysbase->AttnFlags & AFF_68040))
    return 0;
#elif defined (_M68030)
  if(!(sysbase->AttnFlags & AFF_68030))
    return 0;
#elif defined (_M68020)
  if(!(sysbase->AttnFlags & AFF_68020))
    return 0;
#endif

  /* Remember stuff */
  mb->seglist = seglist;

  /* Fill some globals */
//  MUIMasterBase = (struct Library *)mb;
  mb->sysbase = sysbase;
	SysBase = sysbase;

  D(bug("Librarybase at 0x%lx\n",mb));

  if (L_InitLib(&mb->library))
    return &mb->library;

  /* Free the vector table and the library data */
  FreeMem((STRPTR)mb - mb->library.lib_NegSize,
  mb->library.lib_NegSize +
  mb->library.lib_PosSize);
  return 0;
}

/************************************************************************/
/* This is the table of functions that make up the library. The first
   four are mandatory, everything following it are user callable
   routines. The table is terminated by the value -1. */

static const APTR LibVectors[] = {
  (APTR) LibOpen,
  (APTR) LibClose,
  (APTR) LibExpunge,
  (APTR) LibReserved,
  (APTR) MUI_NewObjectA,
  (APTR) MUI_DisposeObject,
  (APTR) MUI_RequestA,
  (APTR) MUI_AllocAslRequest,
  (APTR) MUI_AslRequest,
  (APTR) MUI_FreeAslRequest,
  (APTR) MUI_Error,
  (APTR) MUI_SetError,
  (APTR) MUI_GetClass,
  (APTR) MUI_FreeClass,
  (APTR) MUI_RequestIDCMP,
  (APTR) MUI_RejectIDCMP,
  (APTR) MUI_Redraw,
  (APTR) MUI_CreateCustomClass,
  (APTR) MUI_DeleteCustomClass,
  (APTR) MUI_MakeObjectA,
  (APTR) MUI_Layout,
  (APTR) MUI_Priv1,
  (APTR) MUI_Priv2,
  (APTR) MUI_Priv3,
  (APTR) MUI_Priv4,
  (APTR) MUI_ObtainPen,
  (APTR) MUI_ReleasePen,
  (APTR) MUI_AddClipping,
  (APTR) MUI_RemoveClipping,
  (APTR) MUI_AddClipRegion,
  (APTR) MUI_RemoveClipRegion,
  (APTR) MUI_BeginRefresh,
  (APTR) MUI_EndRefresh,
  (APTR) -1
};

static const struct LibInitData LibInitData = {
#ifdef __VBCC__    /* VBCC does not like OFFSET macro */
 0xA0,  8, NT_LIBRARY,                0,
 0x80, 10, LIBNAME,
 0xA0, 14, LIBF_SUMUSED|LIBF_CHANGED, 0,
 0x90, 20, VERSION,
 0x90, 22, REVISION,
 0x80, 24, IDSTRING,
#else
 0xA0, (UBYTE) OFFSET(Node,    ln_Type),      NT_LIBRARY,		 0,
 0x80, (UBYTE) OFFSET(Node,    ln_Name),      LIBNAME,
 0xA0, (UBYTE) OFFSET(Library, lib_Flags),    LIBF_SUMUSED|LIBF_CHANGED, 0,
 0x90, (UBYTE) OFFSET(Library, lib_Version),  VERSION,
 0x90, (UBYTE) OFFSET(Library, lib_Revision), REVISION,
 0x80, (UBYTE) OFFSET(Library, lib_IdString), IDSTRING,
#endif
 0
};

/* The following data structures and data are responsible for
   setting up the Library base data structure and the library
   function vector.
*/
const ULONG LibInitTable[4] = {
  (ULONG)sizeof(struct MUIMasterBase_intern), /* Size of the base data structure */
  (ULONG)LibVectors,             /* Points to the function vector */
  (ULONG)&LibInitData,           /* Library base data structure setup table */
  (ULONG)LibInit                 /* The address of the routine to do the setup */
};

void _CXFERR(void)
{
    D(bug("CFXERR\n"));
}
