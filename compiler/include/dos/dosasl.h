#ifndef DOS_DOSASL_H
#define DOS_DOSASL_H

/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
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

/**********************************************************************
 ************************** Pattern Matching **************************
 **********************************************************************/

/* PRIVATE structure, which describes an anchor for matching functions. */
struct AChain
{
    struct AChain * an_Child;  /* The next anchor */
    struct AChain * an_Parent; /* The last anchor */

    BPTR                 an_Lock;     /* Lock of this anchor */
    struct FileInfoBlock an_Info;     /* The fib, describing this anchor */
    BYTE                 an_Flags;    /* see below */
    UBYTE                an_String[1];
};

/* an_Flags */
#define DDB_PatternBit  0
#define DDB_ExaminedBit 1
#define DDB_Completed   2
#define DDB_AllBit      3
#define DDB_Single      4
#define DDF_PatternBit  (1<<DDB_PatternBit)
#define DDF_ExaminedBit (1<<DDB_ExaminedBit)
#define DDF_Completed   (1<<DDB_Completed)
#define DDF_AllBit      (1<<DDB_AllBit)
#define DDF_Single      (1<<DDB_Single)


/* Structure as used with MatchFirst() and MatchNext(). */
struct AnchorPath
{
    struct AChain * ap_Base; /* First anchor. */
    struct AChain * ap_Last; /* Last anchor. */

      /* Signal bits at which the function using this structure should return
         to the caller. See <dos/dos.h> and <exec/tasks.h> for bit definitions.
      */
    LONG                 ap_BreakBits;
      /* Signal bits that caused the function to break. */
    LONG                 ap_FoundBreak;
    BYTE                 ap_Flags;    /* see below */
    BYTE                 ap_Reserved; /* PRIVATE */
      /* Size of ap_Buf (see below). This may be zero. */
    WORD                 ap_Strlen;
      /* Embedded FileInfoBlock structure as defined in <dos/dos.h>. This
         describes any files found by matching-functions. */
    struct FileInfoBlock ap_Info;
      /* Buffer for the fully qualified pathname of files found by
         matching-functions. This may be as large as you want (including
         zero bytes). Put its size into ap_StrLen. */
    UBYTE                ap_Buf[1];
};
#define ap_First   ap_Base
#define ap_Current ap_Last
#define ap_Length  ap_Flags

/* ap_Flags. Some of the flags are set by the matching-functions and some
   are read-only. */
#define APB_DOWILD       0 /* Obsolete. */
#define APB_ITSWILD      1 /* There is actually a wildcard in the supplied
                              string. READ-ONLY */
#define APB_DODIR        2 /* Set, if a directory is to be entered.
                              Applications may clear this bit to prohibit the
                              matching-functions from entering a directory. */
#define APB_DIDDIR       3 /* Set, if directory was already searched.
                              READ-ONLY */
#define APB_NOMEMERR     4 /* Set, if function was out of memory. READ-ONLY */
#define APB_DODOT        5 /* '.' may refer to the current directory
                              (unix-style). */
#define APB_DirChanged   6 /* Directory changed since last call. */
#define APB_FollowHLinks 7 /* Follow hardlinks, too. */

#define APF_DOWILD       (1<<APB_DOWILD)
#define APF_ITSWILD      (1<<APB_ITSWILD)
#define APF_DODIR        (1<<APB_DODIR)
#define APF_DIDDIR       (1<<APB_DIDDIR)
#define APF_NOMEMERR     (1<<APB_NOMEMERR)
#define APF_DODOT        (1<<APB_DODOT)
#define APF_DirChanged   (1<<APB_DirChanged)
#define APF_FollowHLinks (1<<APB_FollowHLinks)

/* Predefined tokens for wildcards. The characters are replaced by these
   tokens in the tokenized string returned by the ParsePattern() function
   family. */
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

/* Additional error-numbers. Main chunk of error-numbers is defined in
   <dos/dos.h>. */
#define ERROR_BUFFER_OVERFLOW 303 /* Supplied or internal buffer too small. */
#define ERROR_BREAK           304 /* One of the break signals was received. */
#define ERROR_NOT_EXECUTABLE  305 /* A file is not an executable. */

#endif /* DOS_DOSASL_H */
