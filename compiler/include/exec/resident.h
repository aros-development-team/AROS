#ifndef EXEC_RESIDENT_H
#define EXEC_RESIDENT_H

/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$

    Resident modules.
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

#ifndef UTILITY_TAGITEM_H
#   include <utility/tagitem.h>
#endif

struct Resident
{
    UWORD                  rt_MatchWord; /* equal to RTC_MATCHWORD (see below) */
    const struct Resident *rt_MatchTag;  /* Pointer to this struct */
    APTR                   rt_EndSkip;
    UBYTE                  rt_Flags;     /* see below */
    UBYTE                  rt_Version;
    UBYTE                  rt_Type;
    BYTE                   rt_Pri;
    CONST_STRPTR           rt_Name;
    CONST_STRPTR           rt_IdString;
    APTR                   rt_Init;

    /* Extension taken over from MorphOS. Only valid
       if RTF_EXTENDED is set */
       
    UWORD            	   rt_Revision;
    struct TagItem        *rt_Tags;
};

#define RTC_MATCHWORD  (0x4AFC)

#define RTF_COLDSTART  (1<<0)
#define RTF_SINGLETASK (1<<1)
#define RTF_AFTERDOS   (1<<2)
#define RTF_AUTOINIT   (1<<7)

#define RTF_EXTENDED   (1<<6) /* MorphOS extension: extended
                                 structure fields are valid */

#define RTW_NEVER      (0)
#define RTW_COLDSTART  (1)

#endif /* EXEC_RESIDENT_H */
