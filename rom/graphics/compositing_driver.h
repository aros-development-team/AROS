#include <hidd/compositing.h>
#include <hidd/graphics.h>
#include <oop/oop.h>

/* Inline stubs for calling the driver */

static inline void composer_LoadViewPorts(OOP_Object *o, struct HIDD_ViewPortData *Data, struct GfxBase *GfxBase)
{
    struct pHidd_Compositing_BitMapStackChanged bscmsg =
    {
    	mID  : PrivGBase(GfxBase)->HiddCompositingMethodBase + moHidd_Compositing_BitMapStackChanged,
        data : Data
    };

    OOP_DoMethod(o, &bscmsg.mID);
}

/* Service functions defined in compositing_driver.c */

ULONG composer_Install(OOP_Class *cl, struct GfxBase *GfxBase);
void composer_Setup(struct monitor_driverdata *mdd, struct GfxBase *GfxBase);
