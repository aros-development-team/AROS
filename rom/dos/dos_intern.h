/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
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

#include "fs_driver.h"

/* the alternative is to do something similar in clib or
   to have two files with the same contents
*/
#include "../../compiler/clib/__filesystem_support.h"

/* FIXME: Remove these #define xxxBase hacks
   Do not use this in new code !
*/
#ifdef TimerBase
#undef TimerBase
#endif
#define TimerBase (DOSBase->dl_TimerIO.tr_node.io_Device)

/* struct Utilitybase is used in the following file so include it
   before defining Utilitybase
*/
#include <proto/utility.h>

#ifdef UtilityBase
#undef UtilityBase
#endif
#define UtilityBase (DOSBase->dl_UtilityBase)

/* Needed for close() */
#define expunge() \
AROS_LC0(BPTR, expunge, struct DosLibrary *, DOSBase, 3, Dos)

#define RDAF_ALLOCATED_BY_READARGS (1L << 31)

struct IntDosBase
{
    struct DosLibrary pub;
    struct Library *debugBase;
};

#define DebugBase ((struct IntDosBase *)DOSBase)->debugBase

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

extern CONST struct EString EString[];

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

/* fh_Flags. The flags are AROS specific and therefore PRIVATE.. */
#define FHF_WRITE   0x80000000
#define FHF_BUF     1
#define FHF_APPEND  2
#define FHF_LINEBUF 4
#define FHF_NOBUF   8
#define FHF_OWNBUF  16

#define FPUTC(f,c) \
(((struct FileHandle *)BADDR(f))->fh_Flags&FHF_WRITE&& \
 ((struct FileHandle *)BADDR(f))->fh_Pos<((struct FileHandle *)BADDR(f))->fh_End? \
*((struct FileHandle *)BADDR(f))->fh_Pos++=c,0:FPutC(f,c))

void IOFS_SendPkt(struct DosPacket *dp, struct MsgPort *replyport, struct DosLibrary *DOSBase);

struct DosPacket *internal_WaitPkt(struct MsgPort *msgPort,
				   struct DosLibrary *DOSBase);

extern APTR BCPL_Setup(struct Process *me, BPTR segList, APTR entry, APTR DOSBase);
extern void BCPL_Cleanup(struct Process *me);
ULONG CallEntry(STRPTR argptr, ULONG argsize, LONG_FUNC entry, struct Process *me);

/* Cli dependent SetProgramName() for use in CreateNewProc() */
BOOL internal_SetProgramName(struct CommandLineInterface *cli,
    CONST_STRPTR name, struct DosLibrary *DOSBase);


/* Pattern matching function used by MatchPattern() and MatchPatternNoCase() */
BOOL patternMatch(CONST_STRPTR pat, CONST_STRPTR str, BOOL useCase,
		  struct DosLibrary *DOSBase);

/* Pattern parsing function used by ParsePattern() and ParsePatternNoCase() */
LONG patternParse(CONST_STRPTR Source, STRPTR Dest, LONG DestLength,
    BOOL useCase, struct DosLibrary *DOSBase);


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
LONG Match_BuildAChainList(CONST_STRPTR pattern, struct AnchorPath *ap,
			   struct AChain **retac, struct DosLibrary *DOSBase);
LONG Match_MakeResult(struct AnchorPath *ap, struct DosLibrary *DOSBase);

void addprocesstoroot(struct Process * , struct DosLibrary *);
void removefromrootnode(struct Process *, struct DosLibrary *);

struct marker
{
    UBYTE type; /* 0: Split 1: MP_NOT */
    CONST_STRPTR pat; /* Pointer into pattern */
    CONST_STRPTR str; /* Pointer into string */
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

#define  STRING_DISK_NOT_VALIDATED                  -4000
#define  STRING_DISK_WRITE_PROTECTED                -4001
#define  STRING_DEVICE_NOT_MOUNTED_INSERT           -4002
#define  STRING_DEVICE_NOT_MOUNTED_REPLACE          -4003
#define  STRING_DEVICE_NOT_MOUNTED_REPLACE_TARGET   -4004
#define  STRING_DISK_FULL                           -4005
#define  STRING_NOT_A_DOS_DISK                      -4006
#define  STRING_NO_DISK                             -4007
#define  STRING_ABORT_BUSY                          -4008
#define  STRING_ABORT_DISK_ERROR                    -4009

#define  STRING_RETRY           -5000
#define  STRING_CANCEL          -5001
#define  STRING_REQUESTTITLE    -5002

#include "dos_commanderrors.h"

/* Force attempts to use DosLibrary->dl_Errors to fail. This is used by
   locale.library's replacement function for DosGetString() to peek
   the pointer of the catalog to use */
   
#define dl_Errors   	    	do_not_use_is_reserved_for_locale_dosgetstring_replacement

#define  __is_task(task)  (((struct Task *)task)->tc_Node.ln_Type == NT_TASK)
#define  __is_process(task)  (((struct Task *)task)->tc_Node.ln_Type == NT_PROCESS)

struct seginfo
{
    struct MinNode node;
    APTR           addr;
    char           name[32];
};

struct debug_segnode
{
    struct MinNode node;
    UBYTE          name[200];
    BPTR           seglist;
    IPTR	   start_address; // start address of loaded executable segment
    struct MinList seginfos;
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
APTR vbuf_alloc(FileHandlePtr fh, STRPTR buf, ULONG size);
void vbuf_inject(BPTR fh, CONST_STRPTR argptr, ULONG argsize, struct DosLibrary *DOSBase);

LONG FWriteChars(BPTR file, CONST UBYTE* buffer, ULONG length, struct DosLibrary *DOSBase);


#ifdef AROS_FAST_BSTR

#define CMPBSTR(x, y) Stricmp(BADDR(x), BADDR(y))
#define CMPNICBSTR(x, y, n) Strnicmp(x, BADDR(y), n)

#else

BOOL CMPBSTR(BSTR, BSTR);
BOOL CMPNICBSTR(CONST_STRPTR, BSTR, UBYTE);

#endif

#ifdef AROS_DOS_PACKETS

BSTR C2BSTR(CONST_STRPTR);
char *BSTR2C(BSTR);
BOOL CMPCBSTR(CONST_STRPTR, BSTR);
BOOL CMPICBSTR(CONST_STRPTR, BSTR);
SIPTR dopacket5(struct DosLibrary *DOSBase, SIPTR *res2, struct MsgPort *port, LONG action, SIPTR arg1, SIPTR arg2, SIPTR arg3, SIPTR arg4, SIPTR arg5);
SIPTR dopacket4(struct DosLibrary *DOSBase, SIPTR *res2, struct MsgPort *port, LONG action, SIPTR arg1, SIPTR arg2, SIPTR arg3, SIPTR arg4);
SIPTR dopacket3(struct DosLibrary *DOSBase, SIPTR *res2, struct MsgPort *port, LONG action, SIPTR arg1, SIPTR arg2, SIPTR arg3);
SIPTR dopacket2(struct DosLibrary *DOSBase, SIPTR *res2, struct MsgPort *port, LONG action, SIPTR arg1, SIPTR arg2);
SIPTR dopacket1(struct DosLibrary *DOSBase, SIPTR *res2, struct MsgPort *port, LONG action, SIPTR arg1);
SIPTR dopacket0(struct DosLibrary *DOSBase, SIPTR *res2, struct MsgPort *port, LONG action);
void fixfib(struct FileInfoBlock*);

struct PacketHelperStruct
{
	BSTR name;
	struct MsgPort *port;
	BPTR lock;
	struct DevProc *dp;
};

BOOL getpacketinfo(struct DosLibrary *DOSBase, CONST_STRPTR, struct PacketHelperStruct*);
BOOL getdevpacketinfo(struct DosLibrary *DOSBase, CONST_STRPTR devname, CONST_STRPTR name, struct PacketHelperStruct *phs);
void freepacketinfo(struct DosLibrary *DOSBase, struct PacketHelperStruct*);

LONG InternalOpen(CONST_STRPTR name, LONG action, 
    struct FileHandle *handle, LONG soft_nesting, struct DosLibrary *DOSBase);

#define ASSERT_VALID_FILELOCK(lock) do { \
    	struct FileLock *fl = BADDR(lock); \
    	if (fl && fl->fl_Access != SHARED_LOCK && fl->fl_Access != EXCLUSIVE_LOCK) { \
    	    bug("%s() called with a bogus FileLock! B=%x FL=%x %s/%s/%d\n", __FUNCTION__, lock, fl, __FILE__,__FUNCTION__,__LINE__); \
    	} \
    } while (0);

#else

#define ASSERT_VALID_FILELOCK(lock)

#endif
    
#endif /* DOS_INTERN_H */
