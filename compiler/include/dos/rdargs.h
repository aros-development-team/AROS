#ifndef DOS_RDARGS_H
#define DOS_RDARGS_H

/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Definitions for the dos function ReadArgs().
    Lang: english
*/

#ifndef EXEC_NODES_H
#   include <exec/nodes.h>
#endif
#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

/**********************************************************************
 ******************************* CSource ******************************
 **********************************************************************/

/* This structure emulates an input stream by using a buffer. */
struct CSource
{
      /* The buffer, which contains the stream. In most cases this may be NULL,
         in which case the current input stream is used. */
    UBYTE * CS_Buffer;
    LONG    CS_Length; /* The length of the buffer. */
    LONG    CS_CurChr; /* The current position in the buffer. */
};

/**********************************************************************
 ***************************** ReadArgs() *****************************
 **********************************************************************/

/* The main structure used for ReadArgs(). It contains everything needed for
   ReadArgs() handling. Allocate this structure with AllocDosObject(). */
struct RDArgs
{
      /* Embedded CSource structure (see above). If CS_Buffer of this structure
         is != NULL, use this structure as source for parsing, otherwise use
         Input() as source. */
    struct CSource RDA_Source;

    IPTR RDA_DAList; /* PRIVATE. Must be initialized to 0. */

    /* The next two fields allow an application to supply a buffer in which
       ReadArgs() will store parsed data. If either of these fields is 0,
       ReadArgs() allocates this buffer itself. */
    UBYTE * RDA_Buffer; /* Pointer to buffer. May be NULL. */
    LONG    RDA_BufSiz; /* Size of the supplied buffer. May be 0. */

      /* Additional help, if user requests it, by supplying '?' as argument. */
    UBYTE * RDA_ExtHelp;
    LONG    RDA_Flags; /* see below */
};

/* RDA_Flags */
#define RDAB_STDIN    0 /* Use Input() instead of the supplied command line. */
#define RDAB_NOALLOC  1 /* Do not allocate more space. */
#define RDAB_NOPROMPT 2 /* Do not prompt for input. */

#define RDAF_STDIN    (1L<<RDAB_STDIN)
#define RDAF_NOALLOC  (1L<<RDAB_NOALLOC)
#define RDAF_NOPROMPT (1L<<RDAB_NOPROMPT)

/**********************************************************************
 **************************** Miscellaneous ***************************
 **********************************************************************/

/* Maximum number of items in a template. This may change in future versions.
*/
#define MAX_TEMPLATE_ITEMS 100

/* The maximum number of arguments in an item, which allows to specify multiple
   arguments (flag '/M'). This may change in future versions.*/
#define MAX_MULTIARGS      128

#endif /* DOS_RDARGS_H */
