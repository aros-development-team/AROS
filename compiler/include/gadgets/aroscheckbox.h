#ifndef GADGETS_AROSCHECKBOX_H
#define GADGETS_AROSCHECKBOX_H

/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc: MethodIDs and AttrIDs for the AROS checkbox class.
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

/* The AROSCheckboxClass ist a subclass of GadgetClass. It is the standard
   checkbox gadget, ie a boolean gadget. You can supply a width and height
   of 0 to it, in which case, the gadget chooses an ideal size for itself.
   Expect any size then. You can also use CHECKBOX_WIDTH and CHECKBOX_HEIGHT
   (see <libraries/gadtools.h>) as sizes for the gadget. Of course, you can
   choose any size you want, too.

   You can always set GA_Image and GA_SelectRender. But these must actually be
   image objects! If you do not set GA_Image at object creation time or you
   set it to NULL, a custom image is used. GA_Image is the image of the
   checkmark in unselected state, GA_SelectRender the image in selected state.
   If GA_SelectRender is set to NULL (or not given at object creation),
   GA_Image is rendered with state IDS_SELECTED, otherwise GA_SelectRender is
   rendered with state IDS_NORMAL. Note that you must not set GA_Image to NULL
   during object runtime! */

/* Use that #define instead of a string. */
#define AROSCHECKBOXCLASS "checkbox.aros"
#define AROSCHECKBOXNAME  "Gadgets/aroscheckbox.gadget"


/* Tags to be passed to AROSCHECKBOXCLASS. */
  /* [ISG] (BOOL) Set/Get the state of the checkmark. */
#define AROSCB_Checked GTCB_Checked

#endif /* GADGETS_AROSCHECKBOX_H */

