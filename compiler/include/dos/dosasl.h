#ifndef DOS_DOSASL_H
#define DOS_DOSASL_H

/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Pattern matching
    Lang: english
*/

#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif
#ifndef EXEC_LISTS_H
#   include <exec/lists.h>
#endif
#ifndef DOS_DOS_H
#   include <dos/dos.h>
#endif

struct AnchorPath
{
    struct AChain * ap_Base;
    struct AChain * ap_Last;

    LONG                 ap_BreakBits;
    LONG                 ap_FoundBreak;
    BYTE                 ap_Flags;      /* see below */
    BYTE                 ap_Reserved;
    WORD                 ap_StrLen;
    struct FileInfoBlock ap_Info;
    UBYTE                ap_Buf[1];
};
#define ap_First   ap_Base
#define ap_Current ap_Last
#define ap_Length  ap_Flags

/* ap_Flags */
#define APB_DOWILD           0
#define APF_DOWILD       (1<<0)
#define APB_ITSWILD          1
#define APF_ITSWILD      (1<<1)
#define APB_DODIR            2
#define APF_DODIR        (1<<2)
#define APB_DIDDIR           3
#define APF_DIDDIR       (1<<3)
#define APB_NOMEMERR         4
#define APF_NOMEMERR     (1<<4)
#define APB_DODOT            5
#define APF_DODOT        (1<<5)
#define APB_DirChanged       6
#define APF_DirChanged   (1<<6)
#define APB_FollowHLinks     7
#define APF_FollowHLinks (1<<7)

struct AChain
{
    struct AChain * an_Child;
    struct AChain * an_Parent;

    BPTR                 an_Lock;
    struct FileInfoBlock an_Info;
    BYTE                 an_Flags;    /* see below */
    UBYTE                an_String[1];
};

/* an_Flags */
#define DDB_PatternBit      0
#define DDF_PatternBit  (1<<0)
#define DDB_ExaminedBit     1
#define DDF_ExaminedBit (1<<1)
#define DDB_Completed       2
#define DDF_Completed   (1<<2)
#define DDB_AllBit          3
#define DDF_AllBit      (1<<3)
#define DDB_Single          4
#define DDF_Single      (1<<4)

/* Tokens for wildcards */
#define P_ANY      0x80 /* Matches everything ("#?" and "*") */
#define P_SINGLE   0x81 /* Any character ("?") */
#define P_ORSTART  0x82 /* Opening parenthesis for OR'ing ("(") */
#define P_ORNEXT   0x83 /* Field delimiter for OR'ing ("|") */
#define P_OREND    0x84 /* Closing parenthesis for OR'ing (")") */
#define P_NOT      0x85 /* Inversion ("~") */
#define P_NOTEND   0x86 /* Inversion end */
#define P_NOTCLASS 0x87 /* Inversion class ("^") */
#define P_CLASS    0x88 /* Class ("[" and "]") */
#define P_REPBEG   0x89 /* Beginning of repetition ("[") */
#define P_REPEND   0x8a /* End of repetition ("]") */
#define P_STOP     0x8b

#define COMPLEX_BIT 1
#define EXAMINE_BIT 2

#define ERROR_BUFFER_OVERFLOW 303
#define ERROR_BREAK           304
#define ERROR_NOT_EXECUTABLE  305

#endif /* DOS_DOSASL_H */
