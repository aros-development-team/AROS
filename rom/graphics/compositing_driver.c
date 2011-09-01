#define DEBUG 1

#include <aros/debug.h>
#include <graphics/driver.h>
#include <hidd/compositing.h>
#include <oop/oop.h>

#include <proto/oop.h>

#include "graphics_intern.h"
#include "compositing_driver.h"

#define HiddCompositingAttrBase CDD(GfxBase)->hiddCompositingAttrBase

ULONG composer_Install(OOP_Class *cl, struct GfxBase *GfxBase)
{
    struct monitor_driverdata *mdd;

    D(bug("[Composer] Installing class 0x%p\n", cl));

    /*
     * Completely lazy initialization.
     * It even can't be done in other way because OOP_GetMethodID() on an unregistered
     * interface will fail (unlike OOP_ObtainAttrBase).
     */
    if (!HiddCompositingAttrBase)
    	HiddCompositingAttrBase = OOP_ObtainAttrBase(IID_Hidd_Compositing);

    if (!HiddCompositingAttrBase)
    	return DD_NO_MEM;

    if (!PrivGBase(GfxBase)->HiddCompositingMethodBase)
    {
    	PrivGBase(GfxBase)->HiddCompositingMethodBase = OOP_GetMethodID(IID_Hidd_Compositing, 0);

	if (!PrivGBase(GfxBase)->HiddCompositingMethodBase)
	{
    	    OOP_ReleaseAttrBase(IID_Hidd_Compositing);

    	    return DD_NO_MEM;
    	}
    }

    CDD(GfxBase)->composerClass = cl;

    for (mdd = CDD(GfxBase)->monitors; mdd; mdd = mdd->next)
    {
	/* If the driver needs software composer, but has no one, instantiate it. */
    	if ((mdd->flags & DF_SoftCompose) && (!mdd->composer))
    	{
    	    composer_Setup(mdd, GfxBase);
    	}
    }

    return DD_OK;
}

void composer_Setup(struct monitor_driverdata *mdd, struct GfxBase *GfxBase)
{
    /* Note that if we have fakegfx object, we'll actually work on top of it... */
    mdd->composer = OOP_NewObjectTags(CDD(GfxBase)->composerClass, NULL, aHidd_Compositing_GfxHidd, mdd->gfxhidd, TAG_DONE);

    /* ... but print name of the original driver, to be informative */
    D(bug("[Composer] Added compositing object 0x%p to driver 0x%p (%s)\n", mdd->composer, mdd->gfxhidd,
     	  OOP_OCLASS(mdd->gfxhidd_orig)->ClassNode.ln_Name));
}
