#include <hidd/compositing.h>
#include <hidd/graphics.h>
#include <oop/oop.h>

/* Inline stubs for calling the driver */

static inline OOP_Object *composer_LoadViewPorts(OOP_Object *o, struct HIDD_ViewPortData *Data, BOOL *Active, struct GfxBase *GfxBase)
{
    struct pHidd_Compositing_BitMapStackChanged bscmsg =
    {
    	mID    : PrivGBase(GfxBase)->HiddCompositingMethodBase + moHidd_Compositing_BitMapStackChanged,
        data   : Data,
        active : Active
    };

    return (OOP_Object *)OOP_DoMethod(o, &bscmsg.mID);
}

static inline BOOL composer_ScrollBitMap(OOP_Object *o, OOP_Object *bitmap, SIPTR *x, SIPTR *y, struct GfxBase *GfxBase)
{
    struct pHidd_Compositing_BitMapPositionChange msg =
    {
        mID	   : PrivGBase(GfxBase)->HiddCompositingMethodBase + moHidd_Compositing_BitMapPositionChange,
        bm	   : bitmap,
        newxoffset : x,
        newyoffset : y
    };

    return OOP_DoMethod(o, &msg.mID);
}

static inline void composer_UpdateBitMap(OOP_Object *o, OOP_Object *bitmap, UWORD x, UWORD y, UWORD w, UWORD h, struct GfxBase *GfxBase)
{
    struct pHidd_Compositing_BitMapRectChanged msg =
    {
    	mID    : PrivGBase(GfxBase)->HiddCompositingMethodBase + moHidd_Compositing_BitMapRectChanged,
    	bm     : bitmap,
    	x      : x,
    	y      : y,
    	width  : w,
    	height : h
    };

    OOP_DoMethod(o, &msg.mID);
}

/* Service functions defined in compositing_driver.c */

ULONG composer_Install(OOP_Class *cl, struct GfxBase *GfxBase);
void composer_Setup(struct monitor_driverdata *mdd, struct GfxBase *GfxBase);
