#ifndef GADGETS_AROSMX_H
#define GADGETS_AROSMX_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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

/* The AROSMutualExcludeClass ist a subclass of GadgetClass.

   You should either supply GA_TextAttr or GA_LabelIText, as the height of the
   gadget is calculated by the font-height. If do not supply it, GA_Height is
   used to calculate this size. Otherwise GA_Height is ignored. */

/* Use that #define instead of a string. */
#define AROSMXCLASS	"mutualexclude.aros"
#define AROSMXNAME	"Gadgets/arosmutualexclude.gadget"

/* Tags to be passed to AROSMXCLASS. */
#define AROSMX_Dummy (TAG_USER + 0x05130000)

  /* [ISG] (UWORD) Active tick. The count starts with 0, which is also the
     default. */
#define AROSMX_Active GTMX_Active
  /* [I..] (STRPTR *) Null-Terminated array of labels for the ticks. The number
     of ticks is determined by the number of entries. This tag is required at
     object creation. */
#define AROSMX_Labels GTMX_Labels
  /* [I..] (UWORD) The vertical spacing between two lines in pixels. Default
     is 1. */
#define AROSMX_Spacing GTMX_Spacing
  /* [I..] (UWORD) The height of a tick. Note that this should be at least as
     height as fontheight + spacing. Otherwise the ticks will overlap each
     other. Default is equal to MX_HEIGHT (see <libraries/gadtools.h>). */
#define AROSMX_TickHeight (AROSMX_Dummy + 01)
  /* [I..] (LONG) The placement of the tick labels. See <intuition/gadgetclass.h>
     for definitions (GV_LabelPlace_*). */
#define AROSMX_TickLabelPlace (AROSMX_Dummy + 02)

#endif /* GADGETS_AROSMX_H */

