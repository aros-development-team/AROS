#include "CompilerSpecific.h"
#include "isapnp_private.h"
#include "controller.h"
#include "devices.h"
#include "init.h"
#include "pnp.h"
#include "pnp_structs.h"

void Req( const char* text, struct ISAPNPBase *res)
{
    bug("%s\n", text);
}

BOOL
HandleStartArgs( struct ISAPNP_Card* card,
                 struct ISAPNPBase*  res )
{
    /*
     * TODO: We will start up early because we can have even ISA PnP video
     * cards, disk controllers, etc. Because of this we can't load
     * any list of disabled devices. Think off what to do with them.
     */
    return TRUE;
}
