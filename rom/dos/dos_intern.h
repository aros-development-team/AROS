/*
    (C) 1995-2000 AROS - The Amiga Research OS
    $Id$

    Desc: Internal types and stuff for dos
    Lang: English
*/
#ifndef DOS_INTERN_H
#define DOS_INTERN_H

#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <dos/dosasl.h>
#include <utility/tagitem.h>

/* Needed for aros_print_not_implemented macro */
#include <aros/debug.h>

#ifdef SysBase
#undef SysBase
#endif
#define SysBase (DOSBase->dl_SysBase)
#ifdef UtilityBase
#undef UtilityBase
#endif
#define UtilityBase (DOSBase->dl_UtilityBase)
#ifdef TimerBase
#undef TimerBase
#endif
#define TimerBase (DOSBase->dl_TimerBase)

/* Needed for close() */
#define expunge() \
AROS_LC0(BPTR, expunge, struct DosLibrary *, DOSBase, 3, Dos)

struct DAList
{
    STRPTR *ArgBuf;
    UBYTE *StrBuf;
    STRPTR *MultVec;
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
    BPTR file;
    LONG count;
};

#define FPUTC(f,c) \
(((struct FileHandle *)BADDR(f))->fh_Flags&FHF_WRITE&& \
 ((struct FileHandle *)BADDR(f))->fh_Pos<((struct FileHandle *)BADDR(f))->fh_End? \
*((struct FileHandle *)BADDR(f))->fh_Pos++=c,0:FPutC(f,c))

LONG DoName(struct IOFileSys *iofs, STRPTR name, struct DosLibrary * DOSBase);
LONG DevName(STRPTR name, struct Device **devptr, struct DosLibrary * DOSBase);

BOOL ExecCommand(ULONG type, STRPTR command, STRPTR shell, BPTR input,
		 BPTR output, struct TagItem *tl, struct DosLibrary *DOSBase);

struct DosPacket *internal_WaitPkt(struct MsgPort *msgPort,
				   struct DosLibrary *DOSBase);

BOOL writeFullPath(struct AnchorPath * AP);
LONG followpattern(struct AnchorPath * AP, 
                   struct AChain * AC,
                   struct DosLibrary * DOSBase);
LONG createresult(struct AnchorPath * AP,
                  struct AChain * AC,
                  struct DosLibrary * DOSBase);

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

#define MP_ESCAPE	0x81 /* Before characters in [0x81;0x8a] */
#define MP_MULT 	0x82 /* _#(_a) */
#define MP_MULT_END	0x83 /* #(a_)_ */
#define MP_NOT		0x84 /* _~(_a) */
#define MP_NOT_END	0x85 /* ~(a_)_ */
#define MP_OR		0x86 /* _(_a|b) */
#define MP_OR_NEXT	0x87 /* (a_|_b) */
#define MP_OR_END	0x88 /* (a|b_)_ */
#define MP_SINGLE	0x89 /* ? */
#define MP_ALL		0x8a /* #? or * */
#define MP_SET		0x8b /* _[_ad-g] */
#define MP_NOT_SET	0x8c /* _[~_ad-g] */
#define MP_DASH 	0x8d /* [ad_-g_] */
#define MP_SET_END	0x8e /* [ad-g_]_ */


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

#endif /* DOS_INTERN_H */
