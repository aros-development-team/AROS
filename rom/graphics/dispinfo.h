#ifndef DISPINFO_H
#define DISPINFO_H

#ifndef PROTO_GRAPHICS_H
#include <proto/graphics.h>
#endif

/****************************************************************************************/

/* RTG display Mode ID construction:

   nnnn xx yy

   nnnn - Number of card in the system, counting starts from 0x0010.
     xx - sync index
     yy - pixelformat index

   As i mentioned, RTG mode counting starts from 0x0010. Lower number means
   Amiga(tm) chipset mode. Modes from 0x0000 to 0x000A are officially defined
   in include/graphics/modeid.h, modes 0x000B - 0x000F are reserved, just in case.

   Note that chipset mode IDs store modifier flags instead of sync/pixelformat object
   indexes. When chipset driver is implemented, this will need to be handled in a special
   way.

   There is no more difference between Amiga mode ID and HIDD Mode ID. All are the
   same, except HIDDs need to ignore card number (in future).

   Sonic <pavel_fedin@mail.ru>

*/

/* This macro is obsolete */
#define AMIGA_TO_HIDD_MODEID(modeid) (modeid)

/* This is a temporary hack which forcibly adds card number (since currently
   we work only with RTG and only with one card) */

#define NOTNULLMASK 0x00100000
#define HIDD_TO_AMIGA_MODEID(modeid) ((modeid) | NOTNULLMASK)

/****************************************************************************************/

HIDDT_ModeID get_hiddmode_for_amigamodeid(ULONG modeid, struct GfxBase *GfxBase);
HIDDT_ModeID get_best_resolution_and_depth(OOP_Object *gfxhidd, struct GfxBase *GfxBase);

/****************************************************************************************/

#endif
