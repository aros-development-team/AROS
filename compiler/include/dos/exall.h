#ifndef DOS_EXALL_H
#define DOS_EXALL_H

/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: ExAll() handling
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef UTILITY_HOOKS_H
#   include <utility/hooks.h>
#endif

struct ExAllData
{
    struct ExAllData * ed_Next;

    UBYTE * ed_Name;
    LONG    ed_Type;     /* see below */
    ULONG   ed_Size;
    ULONG   ed_Prot;
    ULONG   ed_Days;
    ULONG   ed_Mins;
    ULONG   ed_Ticks;
    UBYTE * ed_Comment;
    UWORD   ed_OwnerUID;
    UWORD   ed_OwnerGID;
};

#define ED_NAME       1
#define ED_TYPE       2
#define ED_SIZE       3
#define ED_PROTECTION 4
#define ED_DATE       5
#define ED_COMMENT    6
#define ED_OWNER      7

struct ExAllControl
{
    ULONG         eac_Entries;
    ULONG         eac_LastKey;
    UBYTE       * eac_MatchString;
    struct Hook * eac_MatchFunc;
};

#endif /* DOS_EXALL_H */
