/*
    (C) 1997-2001 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang: english
*/

#define AROS_ALMOST_COMPATIBLE 1

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <exec/memory.h>
#include <intuition/screens.h>
#include <intuition/icclass.h>
#include <intuition/cghooks.h>
#include <intuition/imageclass.h>
#include <intuition/gadgetclass.h>
#include <string.h>

#include "asl_intern.h"
#include "layout.h"

#define SDEBUG 0
#define DEBUG 0

#include <aros/debug.h>

/*******************************************************************************************/

#define G(x) ((struct Gadget *)(x))
#define EG(x) ((struct ExtGadget *)(x))

/*******************************************************************************************/

static const struct TagItem arrowinc_to_prop [] =
{
    {GA_ID		, ASLSC_Inc	},
    {TAG_DONE		   		}
};

static const struct TagItem arrowdec_to_prop [] =
{
    {GA_ID		, ASLSC_Dec 	},
    {TAG_DONE		  		}
};

static const struct TagItem prop_to_lv [] =
{
    {PGA_Top		, ASLLV_Top 	},
    {TAG_DONE		   		}
};

static const struct TagItem lv_to_prop [] =
{
    {ASLLV_Top		, PGA_Top   	},
    {ASLLV_Total	, PGA_Total 	},
    {ASLLV_Visible	, PGA_Visible	},
    {TAG_DONE		    		}
    
};

/*******************************************************************************************/

BOOL makescrollergadget(struct ScrollerGadget *scrollergad, struct LayoutData *ld,
			struct TagItem *tags, struct AslBase_intern *AslBase)
{
    struct TagItem *ti, *h_ti, *w_ti;
    WORD x, y, w, h, aw, ah;
    UWORD flags = 0;
    BOOL freehoriz = (GetTagData(PGA_Freedom, FREEVERT, tags) == FREEHORIZ);
    BOOL result = FALSE;
    
    if ((ti = FindTagItem(GA_Left, tags)))
    {
        x = ti->ti_Data;
    } else if ((ti = FindTagItem(GA_RelRight, tags)))
    {
        x = ti->ti_Data; flags |= GFLG_RELRIGHT;
    }
    
    if ((ti = FindTagItem(GA_Top, tags)))
    {
        y = ti->ti_Data;
    } else if ((ti = FindTagItem(GA_RelBottom, tags)))
    {
        y = ti->ti_Data; flags |= GFLG_RELBOTTOM;
    }
    
    if ((w_ti = FindTagItem(GA_Width, tags)))
    {
        w = w_ti->ti_Data;
    } else if ((w_ti = FindTagItem(GA_RelWidth, tags)))
    {
        w = w_ti->ti_Data; flags |= GFLG_RELWIDTH;
    }
    
    if ((h_ti = FindTagItem(GA_Height, tags)))
    {
        h = h_ti->ti_Data;
    } else if ((h_ti = FindTagItem(GA_RelHeight, tags)))
    {
        h = h_ti->ti_Data; flags |= GFLG_RELHEIGHT;
    }
    
    if (freehoriz)
    {
        aw = h; ah = h;
	w -= aw * 2;
	if (w_ti) w_ti->ti_Data = w;
    } else {
        aw = w; ah = w;
	h -= ah * 2;
	if (h_ti) h_ti->ti_Data = h; 
    }
    
    if ((scrollergad->prop = NewObjectA(AslBase->aslpropclass, NULL, tags)))
    {
        struct TagItem arrow_tags[] =
	{
	    {GA_Left		, freehoriz ? x + w - 1 : x	},
	    {GA_Top		, freehoriz ? y : y + h - 1	},
	    {GA_Width		, aw				},
	    {GA_Height		, ah				},
	    {GA_RelVerify	, TRUE				},
	    {GA_Immediate	, TRUE				},
	    {GA_Previous	, (IPTR)scrollergad->prop	},
	    {GA_ID		, ID_ARROWDEC			},
	    {TAG_DONE						}	    
	};
	
	if (flags & GFLG_RELRIGHT) arrow_tags[0].ti_Tag = GA_RelRight;
	if (flags & GFLG_RELWIDTH)
	{
	    arrow_tags[0].ti_Tag = GA_RelRight;
	    if (freehoriz) arrow_tags[0].ti_Data += 2;
	}
	
	if (flags & GFLG_RELBOTTOM) arrow_tags[1].ti_Tag = GA_RelBottom;
	if (flags & GFLG_RELHEIGHT)	
	{
	    arrow_tags[1].ti_Tag = GA_RelBottom;
	    if (!freehoriz) arrow_tags[1].ti_Data += 2;
	}
	
	if ((scrollergad->arrow1 = NewObjectA(AslBase->aslarrowclass, NULL, arrow_tags)))
	{
	    if (freehoriz)
	    {		
	        arrow_tags[0].ti_Data += aw;
	    } else {
	        arrow_tags[1].ti_Data += ah;
	    }
	    arrow_tags[6].ti_Data = (IPTR)scrollergad->arrow1;
	    arrow_tags[7].ti_Data = ID_ARROWINC;
	    
	    if ((scrollergad->arrow2 = NewObjectA(AslBase->aslarrowclass, NULL, arrow_tags)))
	    {
	        struct TagItem image_tags[] =
		{
		    {SYSIA_Which	, freehoriz ? LEFTIMAGE : UPIMAGE	},
		    {SYSIA_DrawInfo	, (IPTR)ld->ld_Dri			},
		    {SYSIA_Style	, SYSISTYLE_GADTOOLS			},
		    {IA_Width		, aw					},
		    {IA_Height		, ah					},
		    {TAG_DONE							}		
		};
	        struct Image *im;
	    	
		if ((im = NewObjectA(NULL, SYSICLASS, image_tags)))
		{
		    SetAttrs(scrollergad->arrow1, GA_Image, (IPTR)im,
		    				  TAG_DONE);
						  
		    image_tags[0].ti_Data = (freehoriz ? RIGHTIMAGE : DOWNIMAGE);
		    
		    if ((im = NewObjectA(NULL, SYSICLASS, image_tags)))
		    {
		        struct TagItem ic_tags [] =
			{
			    {ICA_TARGET	, (IPTR) scrollergad->prop	},
			    {ICA_MAP    , (IPTR) arrowdec_to_prop	},
			    {TAG_DONE					}
			};
			
		        SetAttrs(scrollergad->arrow2, GA_Image, (IPTR)im,
						      TAG_DONE);
			
			SetAttrsA(scrollergad->arrow1, ic_tags);
			ic_tags[1].ti_Data = (IPTR)arrowinc_to_prop;
			SetAttrsA(scrollergad->arrow2, ic_tags);
			
			result = TRUE;
			
		    } /* if ((im = NewObjectA(NULL, SYSICLASS, image_tags))) */
		    
		} /* if ((im = NewObject(NULL, SYSICLASS, image_tags))) */
		
	    } /* if ((scrollergad->arrow2 = NewObjectA(NULL, BUTTONGCLASS, arrow_tags))) */
	    
	} /* if (scrollergad->arrow1 = NewObjectA(NULL, BUTTONGCLASS, arrow_tags)) */
	
    } /* if ((scrollergad->prop = NewObjectA(AslBase->aslpropclass, NULL, tags))) */
    
    if (!result) killscrollergadget(scrollergad, AslBase);
    
    return result;
}

/*******************************************************************************************/

void killscrollergadget(struct ScrollerGadget *scrollergad, struct AslBase_intern *AslBase)
{
    if (scrollergad->prop) DisposeObject(scrollergad->prop);
    scrollergad->prop = NULL;
    
    if (scrollergad->arrow1)
    {
        if (G(scrollergad->arrow1)->GadgetRender) DisposeObject(G(scrollergad->arrow1)->GadgetRender);
        DisposeObject(scrollergad->arrow1);
    }
    scrollergad->arrow1 = NULL;
    
    if (scrollergad->arrow2)
    {
        if (G(scrollergad->arrow2)->GadgetRender) DisposeObject(G(scrollergad->arrow2)->GadgetRender);
        DisposeObject(scrollergad->arrow2);
    }
    scrollergad->arrow2 = NULL;
}

/*******************************************************************************************/

void getgadgetcoords(struct Gadget *gad, struct GadgetInfo *gi, WORD *x, WORD *y, WORD *w, WORD *h)
{
    *x = gad->LeftEdge + ((gad->Flags & GFLG_RELRIGHT)  ? gi->gi_Domain.Width  - 1 : 0);
    *y = gad->TopEdge  + ((gad->Flags & GFLG_RELBOTTOM) ? gi->gi_Domain.Height - 1 : 0);
    *w = gad->Width    + ((gad->Flags & GFLG_RELWIDTH)  ? gi->gi_Domain.Width  : 0);
    *h = gad->Height   + ((gad->Flags & GFLG_RELHEIGHT) ? gi->gi_Domain.Height : 0);
}

/*******************************************************************************************/

void connectscrollerandlistview(struct ScrollerGadget *scrollergad, Object *listview,
				struct AslBase_intern *AslBase)
{
    struct TagItem ic_tags[] =
    {
        {ICA_TARGET	, (IPTR)listview	},
	{ICA_MAP	, (IPTR)prop_to_lv	},
	{TAG_DONE				}
    };

#warning This would not work in AmigaOS, because there gadgetclass is really lame and does not check for notifying loops
    
    SetAttrsA(scrollergad->prop, ic_tags);
    
    ic_tags[0].ti_Data = (IPTR)scrollergad->prop;
    ic_tags[1].ti_Data = (IPTR)lv_to_prop;
    
    SetAttrsA(listview,  ic_tags);
}

/*******************************************************************************************/

void FreeObjects(Object **first, Object **last, struct AslBase_intern *AslBase)
{
    Object **objptr;
    
    for(objptr = first; objptr != last; objptr++)
    {
        if (*objptr)
	{
	    DisposeObject(*objptr);
	    *objptr = NULL;
	}	    
    }
}

/*******************************************************************************************/
