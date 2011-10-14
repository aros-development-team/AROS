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
