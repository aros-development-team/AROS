/*
 * Select a driver to call for operations between struct BitMap (source) and OOP_Object (destination).
 * Used by BltBitMapRastPort() and BltMaskBitMapRastPort().
 *
 * Selection rules:
 * 1. If one of drivers is fakegfx.hidd, we must use it in order
 *    to de-masquerade fakefb objects.
 * 2. If one of drivers is our default software bitmap driver,
 *    we use another one, which can be an accelerated video driver.
 */
static inline OOP_Object *SelectDriverObject(struct BitMap *srcbm, OOP_Object *dstbm_obj, struct GfxBase *GfxBase)
{
    struct monitor_driverdata *driver  = GET_BM_DRIVERDATA(srcbm);
    OOP_Object *gfxhidd = driver->gfxhidd;
    OOP_Object *dest_gfxhidd;

    OOP_GetAttr(dstbm_obj, aHidd_BitMap_GfxHidd, (IPTR *)&dest_gfxhidd);

    if (driver == (struct monitor_driverdata *)CDD(GfxBase))
    {
	/*
	 * If source bitmap is generic software one, we select destination bitmap.
	 * It can be either fakegfx or accelerated hardware driver.
	 */
    	return dest_gfxhidd;
    }
    else if (OOP_OCLASS(dest_gfxhidd) == CDD(GfxBase)->fakegfxclass)
    {
	/*
	 * If destination bitmap is fakegfx bitmap, we use its driver.
	 * Source one might be not fakegfx.
	 */
    	return dest_gfxhidd;
    }
    else
    {
	/*
	 * If both tests failed, we use source driver. We know that source it not a
	 * generic software driver, and destination is not fakegfx. So, source
	 * can be either fakegfx or hardware driver.
	 */
	return gfxhidd;
    }
}

static inline OOP_Object *GetDriverData(struct RastPort *rp, struct GfxBase *GfxBase)
{
    /*
     * Here we do a little bit of hacky-wacky magic.
     * As we all know, there is no common call for RastPort deinitialization.
     * At the other hand, we need some way to represent a RastPort as GC object to our graphics drivers.
     * Fortunately, RastPort has enough private space in it to embed a specially-made GC object.
     *
     * There are several restrictions in such approach:
     * 1. We must never OOP_DisposeObject() this GC. It is not a real issue, because this object is assumed
     *    to be owned by graphics.library, and only it has rights to dispose it.
     * 2. We can't use subclasses of this class. Since GC is just a data container, nobody needed to subclass
     *    it until and including now. I hope this won't ever become an issue. Additionally, with current
     *	  graphics.library design, GC objects are freely passed accross different display drivers.
     *
     * Well, so, we need to build an OOP_Object of GC class without doing any allocations.
     * Is it possible? Yes, it is. We need to know three facts in order to do this:
     * 1. OOP_Object is actually a class pointer, *preceding* instance data structure.
     * 2. GC's superclass is root class. So, its instance data has offset 0.
     * 3. As i said above, size of the complete object perfectly fits inside RastPort's private area.
     */

    /* Take our instance data, this will be our object pointer. Having it as APTR gets rid of warnings */
    APTR gc = RP_GC(rp);

    /*
     * Now fill in attributes. We could perfectly use OOP_SetAttrs(), but why?
     * It's our own object, let's go fast!
     */
    if (!(rp->Flags & RPF_NO_PENS))
    {
    	/* If PenMode is disabled, FG and BG are already set */
    	GC_FG(gc) = rp->BitMap ? BM_PIXEL(rp->BitMap, rp->FgPen & PEN_MASK) : rp->FgPen;
    	GC_BG(gc) = rp->BitMap ? BM_PIXEL(rp->BitMap, rp->BgPen & PEN_MASK) : rp->BgPen;
    }
    GC_DRMD(gc)   = vHidd_GC_DrawMode_Copy;
    GC_COLEXP(gc) = 0;

    /* Now set GC's DrawMode (effectively ROP) and color expansion (actually transparency) flags */
    if (rp->DrawMode & JAM2)
    {
	GC_COLEXP(gc) = vHidd_GC_ColExp_Opaque;
    }
    else if (rp->DrawMode & COMPLEMENT)
    {
	GC_DRMD(gc) = vHidd_GC_DrawMode_Invert;
    }
    else if ((rp->DrawMode & (~INVERSVID)) == JAM1)
    {
	GC_COLEXP(gc) = vHidd_GC_ColExp_Transparent;
    }

    /* FIXME: Handle INVERSVID by swapping apen and bpen ? */

    OOP_OCLASS(gc) = CDD(GfxBase)->gcClass;	/* Set class pointer			*/
    return gc;					/* Our handmade object is ready to use!	*/
}
