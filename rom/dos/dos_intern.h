/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Internal types and stuff for dos
    Lang: English
*/
#ifndef DOS_INTERN_H
#define DOS_INTERN_H

#include <aros/system.h>
#include <exec/io.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <dos/dosasl.h>
#include <utility/tagitem.h>

#include "dos_dosdoio.h"

#ifdef TimerBase
#undef TimerBase
#endif
#define TimerBase (DOSBase->dl_TimerBase)

/* Needed for close() */
#define expunge() \
AROS_LC0(BPTR, expunge, struct DosLibrary *, DOSBase, 3, Dos)

#define RDAF_ALLOCATED_BY_READARGS (1L << 31)

struct DAList
{
    STRPTR *ArgBuf;
    UBYTE  *StrBuf;
    STRPTR *MultVec;
    BOOL    FreeRDA;
};

struct EString
{
    LONG Number;
    STRPTR String;
};

extern struct EString EString[];

#ifndef EOF
#define EOF -1
#endif
#ifndef IOBUFSIZE
#define IOBUFSIZE 4096
#endif

struct vfp
{
    BPTR               file;
    LONG               count;
    struct DosLibrary *DOSBase;
};

#define FPUTC(f,c) \
(((struct FileHandle *)BADDR(f))->fh_Flags&FHF_WRITE&& \
 ((struct FileHandle *)BADDR(f))->fh_Pos<((struct FileHandle *)BADDR(f))->fh_End? \
*((struct FileHandle *)BADDR(f))->fh_Pos++=c,0:FPutC(f,c))

LONG DoName(struct IOFileSys *iofs, CONST_STRPTR name,
	    struct DosLibrary *DOSBase);
LONG DevName(CONST_STRPTR name, struct Device **devptr,
	     struct DosLibrary *DOSBase);

void InitIOFS(struct IOFileSys *iofs, ULONG type,
	      struct DosLibrary *DOSBase);

struct Device *GetDevice(CONST_STRPTR name, struct Unit **unit,
			 struct DosLibrary *DOSBase);

struct Device *GetVolume(CONST_STRPTR name, struct Unit **unit,
			 struct DosLibrary *DOSBase);

struct DosPacket *internal_WaitPkt(struct MsgPort *msgPort,
				   struct DosLibrary *DOSBase);



/* Cli dependent SetProgramName() for use in CreateNewProc() */
BOOL internal_SetProgramName(struct CommandLineInterface *cli, STRPTR name,
			     struct DosLibrary *DOSBase);


/* Pattern matching function used by MatchPattern() and MatchPatternNoCase() */
BOOL patternMatch(STRPTR pat, STRPTR str, BOOL useCase,
		  struct DosLibrary *DOSBase);

/* Pattern parsing function used by ParsePattern() and ParsePatternNoCase() */
LONG patternParse(STRPTR Source, STRPTR Dest, LONG DestLength, BOOL useCase,
		  struct DosLibrary *DOSBase);


LONG InternalSeek
( 
    struct FileHandle *fh, 
    LONG               position, 
    LONG               mode, 
    struct DosLibrary *DOSBase 
);
LONG InternalFlush( struct FileHandle *fh, struct DosLibrary *DOSBase );

                  
/* match_misc.c */

struct AChain *Match_AllocAChain(LONG extrasize, struct DosLibrary *DOSBase);
void Match_FreeAChain(struct AChain *ac, struct DosLibrary *DOSBase);
LONG Match_BuildAChainList(STRPTR pattern, struct AnchorPath *ap,
			   struct AChain **retac, struct DosLibrary *DOSBase);
LONG Match_MakeResult(struct AnchorPath *ap, struct DosLibrary *DOSBase);

void addprocesstoroot(struct Process * , struct DosLibrary *);
void removefromrootnode(struct Process *, struct DosLibrary *);

struct marker
{
    UBYTE type; /* 0: Split 1: MP_NOT */
    STRPTR pat; /* Pointer into pattern */
    STRPTR str; /* Pointer into string */
};

struct markerarray
{
    struct markerarray *next;
    struct markerarray *prev;
    struct marker marker[128];
};

#define PUSH(t,p,s)                                                     \
{									\
    if(macnt==128)                                                      \
    {									\
	if(macur->next==NULL)                                           \
	{								\
	    macur->next=AllocMem(sizeof(struct markerarray),MEMF_ANY);  \
	    if(macur->next==NULL)                                       \
		ERROR(ERROR_NO_FREE_STORE);                             \
	    macur->next->prev=macur;					\
	}								\
	macur=macur->next;						\
	macnt=0;							\
    }									\
    macur->marker[macnt].type=(t);                                      \
    macur->marker[macnt].pat=(p);                                       \
    macur->marker[macnt].str=(s);                                       \
    macnt++;								\
}

#define POP(t,p,s)                      \
{					\
    macnt--;				\
    if(macnt<0)                         \
    {					\
	macnt=127;			\
	macur=macur->prev;		\
	if(macur==NULL)                 \
	    ERROR(0);                   \
    }					\
    (t)=macur->marker[macnt].type;      \
    (p)=macur->marker[macnt].pat;       \
    (s)=macur->marker[macnt].str;       \
}

#define MP_ESCAPE		0x81 /* Before characters in [0x81;0x8a] */
#define MP_MULT 		0x82 /* _#(_a) */
#define MP_MULT_END		0x83 /* #(a_)_ */
#define MP_NOT			0x84 /* _~(_a) */
#define MP_NOT_END		0x85 /* ~(a_)_ */
#define MP_OR			0x86 /* _(_a|b) */
#define MP_OR_NEXT		0x87 /* (a_|_b) */
#define MP_OR_END		0x88 /* (a|b_)_ */
#define MP_SINGLE		0x89 /* ? */
#define MP_ALL			0x8a /* #? or * */
#define MP_SET			0x8b /* _[_ad-g] */
#define MP_NOT_SET		0x8c /* _[~_ad-g] */
#define MP_DASH 		0x8d /* [ad_-g_] */
#define MP_SET_END		0x8e /* [ad-g_]_ */

/* Whether MatchFirst/MatchNext/MatchEnd in case of the base
   AChain should just take the currentdir lock pointer, or
   make a real duplicate with DupLock() */

#define MATCHFUNCS_NO_DUPLOCK 	0

/* DosGetString additional codes (printf style parametrized) */

#define  STRING_INSERT_VOLUME   -4000
#define  STRING_VOLUME_FULL     -4001
#define  STRING_NO_DISK         -4002
#define  STRING_NO_DOS_DISK     -4003
#define  STRING_MUST_REPLACE    -4004

#define  STRING_RETRY           -5000
#define  STRING_CANCEL          -5001
#define  STRING_REQUESTTITLE    -5002

#include <dos_commanderrors.h>

/* Force attempts to use DosLibrary->dl_Errors to fail. This is used by
   locale.library's replacement function for DosGetString() to peek
   the pointer of the catalog to use */
   
#define dl_Errors   	    	do_not_use_is_reserved_for_locale_dosgetstring_replacement

#define  __is_task(task)  (((struct Task *)task)->tc_Node.ln_Type == NT_TASK)
#define  __is_process(task)  (((struct Task *)task)->tc_Node.ln_Type == NT_PROCESS)

struct debug_segnode
{
    struct MinNode node;
    UBYTE          name[200];
    BPTR           seglist;
    IPTR	   start_address; // start address of loaded executable segment
};

struct InternalExAllControl
{
    struct ExAllControl   eac;
    /* Used for ExAll emulation. If non null, it means
       ExAll emulation is being performed.  */
    struct FileInfoBlock *fib;
};    


typedef struct FileHandle* FileHandlePtr;

void vbuf_free(FileHandlePtr fh);

IPTR vbuf_alloc(FileHandlePtr fh, ULONG size, struct DosLibrary *DOSBase);

LONG FWriteChars(BPTR file, CONST UBYTE* buffer, ULONG length, struct DosLibrary *DOSBase);
    
#endif /* DOS_INTERN_H */
