#ifndef DISPINFO_H
#define DISPINFO_H

#ifndef PROTO_GRAPHICS_H
#include <proto/graphics.h>
#endif

/****************************************************************************************/

/*
   RTG display Mode ID construction:

   nnnn xx yy

   nnnn - Number of card in the system, counting starts from 0x0010.
     xx - sync index
     yy - pixelformat index

   xx and yy parts are obtained from the HIDD. Card number is maintained by
   graphics.library. Resulting mode ID is obtained by logical OR between these
   two components.

   As i mentioned, RTG mode counting starts from 0x0010. Lower number means
   Amiga(tm) chipset mode. Modes from 0x0000 to 0x000A are officially defined
   in include/graphics/modeid.h, modes 0x000B - 0x000F are reserved, just in case.

   Note that chipset mode IDs store modifier flags instead of sync/pixelformat object
   indexes. When chipset driver is implemented, this will need to be handled in a special
   way (by overloading mode ID processing methods in the driver).

   Sonic <pavel_fedin@mail.ru>
*/

#define AROS_RTG_MONITOR_ID   0x00100000 /* First RTG monitor ID     */
#define AROS_MONITOR_ID_MASK  0xFFFF0000 /* Internal monitor ID mask */

/****************************************************************************************/

HIDDT_ModeID get_best_resolution_and_depth(struct monitor_driverdata *mdd, struct GfxBase *GfxBase);
struct monitor_driverdata *MonitorFromSpec(struct MonitorSpec *mspc, struct GfxBase *GfxBase);

/****************************************************************************************/

#endif
