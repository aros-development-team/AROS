/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef FAKEGFXHIDD_H
#define FAKEGFXHIDD_H

#define CLID_Hidd_FakeGfxHidd	"hidd.gfx.fake"
#define CLID_Hidd_FakeFB	"hidd.bitmap.fakefb"

#define IID_Hidd_FakeGfxHidd	"hidd.gfx.fake"
#define IID_Hidd_FakeFB		"hidd.bitmap.fakefb"


#define HiddFakeGfxHiddAttrBase	__IHidd_FakeGfxHidd
#define HiddFakeFBAttrBase	__IHidd_FakeFB

enum {
    aoHidd_FakeGfxHidd_RealGfxHidd,	/* [I..] Object *	*/
    
    num_Hidd_FakeGfxHidd_Attrs
};

#define aHidd_FakeGfxHidd_RealGfxHidd	(HiddFakeGfxHiddAttrBase + aoHidd_FakeGfxHidd_RealGfxHidd )
#define aHidd_FakeGfxHidd_	(HiddFakeGfxHiddAttrBase + aoHidd_FakeGfxHidd_)

enum {
    aoHidd_FakeFB_RealBitMap,
    aoHidd_FakeFB_FakeGfxHidd,
    
    num_Hidd_FakeFB_Attrs
};    

#define aHidd_FakeFB_RealBitMap	(HiddFakeFBAttrBase + aoHidd_FakeFB_RealBitMap	)
#define aHidd_FakeFB_FakeGfxHidd	(HiddFakeFBAttrBase + aoHidd_FakeFB_FakeGfxHidd	)
#define aHidd_FakeFB_	(HiddFakeFBAttrBase + aoHidd_FakeFB_)

OOP_Object *init_fakegfxhidd(BOOL noframebuffer, OOP_Object *gfxhidd, struct GfxBase *GfxBase);
VOID cleanup_fakegfxhidd(struct GfxBase *GfxBase);

#endif /* FAKEGFXHIDD_H */
