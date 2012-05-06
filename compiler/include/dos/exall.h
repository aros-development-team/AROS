#ifndef DOS_EXALL_H
#define DOS_EXALL_H

/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ExAll() handling.
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef UTILITY_HOOKS_H
#   include <utility/hooks.h>
#endif

/* Structure (as used in ExAll()), containing information about a file. This
   structure is only as long as it need to be. If is for example ED_SIZE was
   specified, when calling ExAll(), this structure only consists of the fields
   ed_Name through ed_Size. Therefore you can use the ED_ definitions below
   as longword offsets into this structure. */
struct ExAllData64
{
    struct ExAllData64 * ed_Next;

    UBYTE * ed_Name;     /* Name of the file. */
    LONG    ed_Type;     /* Type of file. See <dos/dosextens.h>. */
    UQUAD   ed_Size;     /* Size of file. */
    ULONG   ed_Prot;     /* Protection bits. */

    /* The following three fields are de facto an embedded datestamp
       structure (see <dos/dos.h>), which describes the last modification
       date. */
    ULONG ed_Days;
    ULONG ed_Mins;
    ULONG ed_Ticks;

    UBYTE * ed_Comment;  /* The file comment. */

    UWORD ed_OwnerUID; /* The owner ID. */
    UWORD ed_OwnerGID; /* The group-owner ID. */
};

struct ExAllData32
{
    struct ExAllData32 * ed_Next;

    UBYTE * ed_Name;     /* Name of the file. */
    LONG    ed_Type;     /* Type of file. See <dos/dosextens.h>. */
    ULONG   ed_Size;     /* Size of file. */
    ULONG   ed_Prot;     /* Protection bits. */

    /* The following three fields are de facto an embedded datestamp
       structure (see <dos/dos.h>), which describes the last modification
       date. */
    ULONG ed_Days;
    ULONG ed_Mins;
    ULONG ed_Ticks;

    UBYTE * ed_Comment;  /* The file comment. */

    UWORD ed_OwnerUID; /* The owner ID. */
    UWORD ed_OwnerGID; /* The group-owner ID. */
};

#if (__DOS64)
#define ExAllData ExAllData64
#else
#define ExAllData ExAllData32
#endif

/* Type argument for ExAll(). Each number includes the information of all
   lower numbers, too. If you specify for example ED_SIZE, you will get
   information about name, type and the size of a file. Note that all
   filehandlers must handle all types up to ED_OWNER. If they do not support
   a type, they must return ERROR_WRONG_NUMBER (see <dos/dos.h>). Currently
   that means, if a value higher than ED_OWNER is specified, filehandlers
   must fail with this error. */
#define ED_NAME       1 /* Filename. */
#define ED_TYPE       2 /* Type of file. See <dos/dosextens.h>. */
#define ED_SIZE       3 /* Size of file. */
#define ED_PROTECTION 4 /* Protection bits. */
#define ED_DATE       5 /* Last modification date. */
#define ED_COMMENT    6 /* Addtional file comment. */
#define ED_OWNER      7 /* Owner information. */


/* Structure as used for controlling ExAll(). Allocate this structure by using
   AllocDosObject(DOS_EXALLCONTROL,...) only. All fields must be initialized
   to 0, before using this structure. (AllocDosObject() does that for you.)
   After calling ExAll() the first time, this structure is READ-ONLY. */
struct ExAllControl
{
      /* The number of entries that were returned in the buffer. */
    ULONG         eac_Entries;
    IPTR          eac_LastKey;     /* PRIVATE */
      /* Parsed pattern string, as created by ParsePattern(). This may be NULL.
      */
    UBYTE       * eac_MatchString;
      /* You may supply a hook, which is called for each entry. This hook
         should return TRUE, if the current entry is to be included in
         the file list and FALSE, if it should be ignored. */
    struct Hook * eac_MatchFunc;
};

#endif /* DOS_EXALL_H */
