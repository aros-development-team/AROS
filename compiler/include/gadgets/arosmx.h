#ifndef GADGETS_AROSMX_H
#define GADGETS_AROSMX_H

/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: MethodIDs and AttrIDs for the AROS mutualexclude class.
    Lang: english
*/
#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef UTILITY_TAGITEM_H
#   include <utility/tagitem.h>
#endif
#ifndef LIBRARIES_GADTOOLS_H
#   include <libraries/gadtools.h>
#endif

/* The AROSMutualExcludeClass ist a subclass of GadgetClass. */

/* Use that #define instead of a string. */
#define AROSMXCLASS "mutualexclude.aros"


/* Tags to be passed to AROSMXCLASS. */
  /* [ISG] (UWORD) Active tick. The count starts with 0, which is also the
     default. */
#define AROSMX_Active GTMX_Active
  /* [I..] (STRPTR *) Null-Terminated array of labels for the ticks. The number
     of ticks is determined by the number of entries. This tag is required at
     object creation. */
#define AROSMX_Labels GTMX_Labels
  /* [I..] (UWORD) The vertical spacing between two ticks in pixels. Default
     is 1. */
#define AROSMX_Spacing GTMX_Spacing

#endif /* GADGETS_AROSMX_H */

