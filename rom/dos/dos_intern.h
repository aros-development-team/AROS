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
#include <dos/exall.h>
#include <dos/dosextens.h>
#include <dos/dosasl.h>
#include <dos/filehandler.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/utility.h>

#include "fs_driver.h"

/*
 * These bases are historically placed in public portion of DOSBase.
 * We won't change this.
 */
#undef TimerBase
#define TimerBase (DOSBase->dl_TimeReq->tr_node.io_Device)
#undef UtilityBase
#define UtilityBase (DOSBase->dl_UtilityBase)

/* Needed for close() */
#define expunge() \
AROS_LC0(BPTR, expunge, struct DosLibrary *, DOSBase, 3, Dos)

#define RDAF_ALLOCATED_BY_READARGS (1L << 31)

/* BCPL compatibility: At least LONG alignment required */

struct IntDosBase
{
    struct DosLibrary pub;
    struct Library *debugBase;
    struct RootNode rootNode  __attribute__((aligned(4)));
    struct ErrorString errors  __attribute__((aligned(4)));
#ifdef __arm__
    ULONG arm_Arch; /* ARM-specific info for ELF loader */
    BOOL  arm_VFP;
    BOOL  arm_VFP_v3;
#endif
};

#define IDosBase(base) ((struct IntDosBase *)base)
#define DebugBase IDosBase(DOSBase)->debugBase

struct DAList
{
    STRPTR *ArgBuf;
    UBYTE  *StrBuf;
    STRPTR *MultVec;
    BOOL    FreeRDA;
};

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
#define FHF_WRITE    0x80000000
#define FHF_BUF      0x00000001
#define FHF_APPEND   0x00000002
#define FHF_LINEBUF  0x00000004
#define FHF_NOBUF    0x00000008
#define FHF_OWNBUF   0x00000010
#define FHF_FLUSHING 0x00000020

#define FPUTC(f,c) \
(((struct FileHandle *)BADDR(f))->fh_Flags&FHF_WRITE&& \
 ((struct FileHandle *)BADDR(f))->fh_Pos<((struct FileHandle *)BADDR(f))->fh_End? \
*((struct FileHandle *)BADDR(f))->fh_Pos++=c,0:FPutC(f,c))

#define DOS_FH_MAGIC    AROS_MAKE_ID('F','h','n','d')
#define ISFILEHANDLE(f) \
    ((((struct FileHandle *)BADDR(f))->fh_Func2) == DOS_FH_MAGIC)

/* Softlink handling */
STRPTR ResolveSoftlink(BPTR cur, struct DevProc *dvp, CONST_STRPTR name, struct DosLibrary *DOSBase);
LONG RootDir(struct DevProc *dvp, struct DosLibrary *DOSBase);

/* Packet I/O */
struct DosPacket *allocdospacket(void);
void freedospacket(struct DosPacket *dp);
SIPTR dopacket(SIPTR *res2, struct MsgPort *port, LONG action, SIPTR arg1, SIPTR arg2, SIPTR arg3, SIPTR arg4, SIPTR arg5, SIPTR arg6, SIPTR arg7);
void internal_SendPkt(struct DosPacket *dp, struct MsgPort *port, struct MsgPort *replyport);
struct DosPacket *internal_WaitPkt(struct MsgPort *msgPort);
void internal_ReplyPkt(struct DosPacket *dp, struct MsgPort *replyPort, SIPTR res1, LONG res2);

#define dopacket5(base, res2, port, action, arg1, arg2, arg3, arg4, arg5) dopacket(res2, port, action, (SIPTR)(arg1), (SIPTR)(arg2), (SIPTR)(arg3), (SIPTR)(arg4), (SIPTR)(arg5), 0, 0)
#define dopacket4(base, res2, port, action, arg1, arg2, arg3, arg4)       dopacket(res2, port, action, (SIPTR)(arg1), (SIPTR)(arg2), (SIPTR)(arg3), (SIPTR)(arg4), 0, 0, 0)
#define dopacket3(base, res2, port, action, arg1, arg2, arg3)             dopacket(res2, port, action, (SIPTR)(arg1), (SIPTR)(arg2), (SIPTR)(arg3), 0, 0, 0, 0)
#define dopacket2(base, res2, port, action, arg1, arg2)                   dopacket(res2, port, action, (SIPTR)(arg1), (SIPTR)(arg2), 0, 0, 0, 0, 0)
#define dopacket1(base, res2, port, action, arg1)                         dopacket(res2, port, action, (SIPTR)(arg1), 0, 0, 0, 0, 0, 0)
#define dopacket0(base, res2, port, action)                               dopacket(res2, port, action, 0, 0, 0, 0, 0, 0, 0)

#ifdef __mc68000
extern void BCPL_Fixup(struct Process *me);
#else
#define BCPL_Fixup(p) do { } while (0)
#endif

ULONG CallEntry(STRPTR argptr, ULONG argsize, LONG_FUNC entry, struct Process *me);

struct MsgPort *RunHandler(struct DeviceNode *deviceNode, const char *path, struct DosLibrary *DOSBase);
BOOL namefrom_internal(struct DosLibrary *DOSBase, BPTR lock, STRPTR buffer, LONG length);

/* Platform-overridable boot sequence */
void __dos_Boot(struct DosLibrary *DOSBase, ULONG BootFlags, UBYTE Flags);
BOOL __dos_IsBootable(struct DosLibrary *DOSBase, BPTR Lock);

/* Cli dependent SetProgramName() for use in CreateNewProc() */
BOOL internal_SetProgramName(struct CommandLineInterface *cli,
    CONST_STRPTR name, struct DosLibrary *DOSBase);
/* Duplicate a cli_CommandDir BPTR list */
BPTR internal_CopyPath(BPTR boldpath, struct DosLibrary * DOSBase);


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
{                                                                       \
    if(macnt==128)                                                      \
    {                                                                   \
        if(macur->next==NULL)                                           \
        {                                                               \
            macur->next=AllocMem(sizeof(struct markerarray),MEMF_ANY);  \
            if(macur->next==NULL)                                       \
                ERROR(ERROR_NO_FREE_STORE);                             \
            macur->next->prev=macur;                                    \
        }                                                               \
        macur=macur->next;                                              \
        macnt=0;                                                        \
    }                                                                   \
    macur->marker[macnt].type=(t);                                      \
    macur->marker[macnt].pat=(p);                                       \
    macur->marker[macnt].str=(s);                                       \
    macnt++;                                                            \
}

#define POP(t,p,s)                      \
{                                       \
    macnt--;                            \
    if(macnt<0)                         \
    {                                   \
        macnt=127;                      \
        macur=macur->prev;              \
        if(macur==NULL)                 \
            ERROR(0);                   \
    }                                   \
    (t)=macur->marker[macnt].type;      \
    (p)=macur->marker[macnt].pat;       \
    (s)=macur->marker[macnt].str;       \
}

#define MP_ESCAPE               0x81 /* Before characters in [0x81;0x8a] */
#define MP_MULT                 0x82 /* _#(_a) */
#define MP_MULT_END             0x83 /* #(a_)_ */
#define MP_NOT                  0x84 /* _~(_a) */
#define MP_NOT_END              0x85 /* ~(a_)_ */
#define MP_OR                   0x86 /* _(_a|b) */
#define MP_OR_NEXT              0x87 /* (a_|_b) */
#define MP_OR_END               0x88 /* (a|b_)_ */
#define MP_SINGLE               0x89 /* ? */
#define MP_ALL                  0x8a /* #? or * */
#define MP_SET                  0x8b /* _[_ad-g] */
#define MP_NOT_SET              0x8c /* _[~_ad-g] */
#define MP_DASH                 0x8d /* [ad_-g_] */
#define MP_SET_END              0x8e /* [ad-g_]_ */

/* Whether MatchFirst/MatchNext/MatchEnd in case of the base
   AChain should just take the currentdir lock pointer, or
   make a real duplicate with DupLock() */

#define MATCHFUNCS_NO_DUPLOCK   0

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
    IPTR           start_address; // start address of loaded executable segment
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
BOOL vbuf_inject(BPTR fh, CONST_STRPTR argptr, ULONG argsize, struct DosLibrary *DOSBase);

LONG FWriteChars(BPTR file, CONST UBYTE* buffer, ULONG length, struct DosLibrary *DOSBase);


#ifdef AROS_FAST_BSTR

#define CMPBSTR(x, y)       Stricmp(BADDR(x), BADDR(y))
#define CMPNICBSTR(x, y, n) Strnicmp(x, BADDR(y), n)
#define BSTR2C(s)           ((STRPTR)BADDR(s))
#define FreeCSTR(s)

#else

BOOL CMPBSTR(BSTR, BSTR);
BOOL CMPNICBSTR(CONST_STRPTR, BSTR, UBYTE);
char *BSTR2C(BSTR);

#define FreeCSTR(s) FreeVec(s)

#endif

#ifdef AROS_FAST_BSTR
#define C2BSTR(x)       ((char *)(x))
#define FREEC2BSTR(x)   do { } while (0)
#define CMPCBSTR(a,b)   strcmp(a,b)
#define CMPICBSTR(a,b)  Stricmp(a,b)
#else
BSTR C2BSTR(CONST_STRPTR);
#define FREEC2BSTR(bstr) FreeVec(BADDR(bstr))
BOOL CMPCBSTR(CONST_STRPTR, BSTR);
BOOL CMPICBSTR(CONST_STRPTR, BSTR);
#endif

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

#define ASSERT_VALID_FILELOCK(lock) do { \
        struct FileLock *fl = BADDR(lock); \
        if (fl && fl->fl_Access != SHARED_LOCK && fl->fl_Access != EXCLUSIVE_LOCK) { \
            bug("%s() bogus FileLock! '%s' %x %d %s/%s/%d\n", \
                __FUNCTION__, SysBase->ThisTask->tc_Node.ln_Name, fl, fl->fl_Access, __FILE__,__FUNCTION__,__LINE__); \
        } \
    } while (0);

/* Shell utilities */
BPTR findseg_cli(BOOL isBoot, struct DosLibrary *DOSBase);

BPTR findseg_shell(BOOL isBoot, struct DosLibrary *DOSBase);

/* Helper for IN:, OUT:, ERR:, STDIN:, STDOUT:, STDERR:
 */
BOOL pseudoLock(CONST_STRPTR name, LONG lockMode, BPTR *lock, LONG *ret, struct DosLibrary *DOSBase);

#endif /* DOS_INTERN_H */
