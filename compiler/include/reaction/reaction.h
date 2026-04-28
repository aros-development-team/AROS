/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction - ClassAct/ReAction compatible reaction/reaction.h
          Master include file for the ReAction/ClassAct GUI toolkit
*/

#ifndef REACTION_REACTION_H
#define REACTION_REACTION_H

/*
 * This header provides a convenient way to include all ReAction/ClassAct
 * related headers at once, matching the original AmigaOS ClassAct SDK.
 */

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef EXEC_LIBRARIES_H
#include <exec/libraries.h>
#endif
#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif
#ifndef INTUITION_INTUITION_H
#include <intuition/intuition.h>
#endif
#ifndef INTUITION_CLASSES_H
#include <intuition/classes.h>
#endif
#ifndef INTUITION_GADGETCLASS_H
#include <intuition/gadgetclass.h>
#endif
#ifndef INTUITION_IMAGECLASS_H
#include <intuition/imageclass.h>
#endif
#ifndef INTUITION_ICCLASS_H
#include <intuition/icclass.h>
#endif

/* Core class headers */
#include <classes/window.h>

/* Gadget class headers */
#include <gadgets/layout.h>
#include <gadgets/button.h>
#include <gadgets/checkbox.h>
#include <gadgets/chooser.h>
#include <gadgets/clicktab.h>
#include <gadgets/fuelgauge.h>
#include <gadgets/getfile.h>
#include <gadgets/getfont.h>
#include <gadgets/getscreenmode.h>
#include <gadgets/integer.h>
#include <gadgets/listbrowser.h>
#include <gadgets/palette.h>
#include <gadgets/radiobutton.h>
#include <gadgets/scroller.h>
#include <gadgets/slider.h>
#include <gadgets/space.h>
#include <gadgets/speedbar.h>
#include <gadgets/string.h>
#include <gadgets/textfield.h>

/* Image class headers */
#include <images/bevel.h>
#include <images/bitmap.h>
#include <images/drawlist.h>
#include <images/glyph.h>
#include <images/label.h>
#include <images/penmap.h>
#include <images/led.h>

/* Include macros */
#include <reaction/reaction_macros.h>

#endif /* REACTION_REACTION_H */
