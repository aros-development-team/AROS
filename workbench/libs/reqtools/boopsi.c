#include <graphics/gfxbase.h>
#include <graphics/rastport.h>
#include <graphics/text.h>
#include <intuition/intuition.h>
#include <intuition/imageclass.h>
#include <intuition/intuition.h>
#ifdef __AROS__
#include <aros/asmcall.h>
#endif

#include "filereq.h"

#include <string.h>

extern struct IClass *ButtonImgClass;

extern ULONG ASM myTextLength (ASM_REGPARAM(a1, char *,),
    	    	    	       ASM_REGPARAM(a0, struct TextAttr *,),
			       ASM_REGPARAM(a3, UBYTE *,),
			       ASM_REGPARAM(a2, struct Image *,),
			       ASM_REGPARAM(d7, ULONG,));

const UWORD defaultpens[] =
{
    1, 0, 1, 2, 1, 3, 1, 0, 2
};

/****************************************************************************************/

char KeyFromStr(char *string, char underchar)
{
    char c, ret = 0;
    
    while ((c = *string++))
    {
        if (c == underchar)
	{
	    ret = ToUpper(*string);
	    break;
	}
    }
    
    return ret;
}

/****************************************************************************************/

ULONG myTextLength(char *str, struct TextAttr *attr, UBYTE *underscore,
		   struct Image *im, ULONG do_lod)
{
    struct RastPort temprp;
    struct TextFont *font = NULL;
    WORD   pixellen = 0;
    
    InitRastPort(&temprp);
    
    if (attr)
    {
        font = OpenFont(attr);
	if (font)
	{
	    SetFont(&temprp, font);
	} 
	else
	{
	   SetFont(&temprp, GfxBase->DefaultFont);
	}
    }
    else
    {
        SetFont(&temprp, GfxBase->DefaultFont);
    }
    
    if (str)
    {
        struct LocalObjData 	*lod = (struct LocalObjData *)0xDEADBEAF;	
	char			c, *str2;
        WORD 			len = strlen(str), underoff;

        if (do_lod)
	{
	    im->PlaneOnOff = 0;
	    
	    lod = INST_DATA(ButtonImgClass, im);
	    lod->lod_UnderOff = len;
	    
	    im->Height = temprp.TxHeight;
	}
	
        pixellen = TextLength(&temprp, str, len);
	
	str2 = str;
	
	underoff = -1;
	
	while((c = *str2++))
	{
	    underoff++;
	    
	    if (c == *underscore)
	    {
		pixellen -= TextLength(&temprp, underscore, 1);
		
	        if (do_lod)
		{
		    im->PlaneOnOff = *str2; 			/* store code of underscored key */
		    lod->lod_UnderOff = underoff;		/* store offset of underscore */
		    lod->lod_RestLen = len - underoff - 1;	/* store len of remaining string */
		    lod->lod_UnderY = temprp.TxBaseline + 2;    /* Y position of underscore */

		    lod->lod_UnderWidth = TextLength(&temprp, str2, 1);
		}
	        break;
	    }
	}

    } /* if (str) */
    
#ifdef __AROS__
    DeinitRastPort(&temprp);
#endif

    if (font) CloseFont(font);

    return pixellen;
}

/****************************************************************************************/

#define imsg ((struct impDraw *)msg)

#ifdef __AROS__
AROS_UFH3(IPTR, myBoopsiDispatch,
	  AROS_UFHA(Class *, cl, A0),
	  AROS_UFHA(struct Image *, im, A2),
	  AROS_UFHA(Msg, msg, A1))
{
    AROS_USERFUNC_INIT
#else
IPTR myBoopsiDispatch(REGPARAM(a0, Class *, cl),
    	    	      REGPARAM(a2, struct Image *, im),
		      REGPARAM(a1, Msg, msg))
{
#endif
    struct LocalObjData *data;
    struct TextFont	*font, *oldfont;
    struct RastPort	*rp;
    struct Gadget 	*gad;
    UWORD 		*pens;
    WORD 		xpos, ypos;
    IPTR 		retval = 0;
    
    switch(msg->MethodID)
    {
        case OM_NEW:
 	    retval = DoSuperMethodA(cl, (Object *)im, msg);
 	    if (retval)
	    {
	        UBYTE underscorechar;
	        union {
	    	    struct {
	    		WORD Width;
	    		WORD Height;
	    	    } size;
	    	    ULONG magic;
	        } __tmp;
		
 	        im = (struct Image *)retval;
		
		data = INST_DATA(cl, im);
		data->lod_IData = *(struct InitData *)im->ImageData;
		
		underscorechar = (UBYTE)data->lod_IData.idata_Underscore;
			
 		im->Width = myTextLength(data->lod_IData.idata_Label,
					 data->lod_IData.idata_TextAttr,
					 &underscorechar,
					 im,
					 cl->cl_InstOffset	/* for C Routines its not really more than a flag */
					 );
		
		/* Calculate text position if we have a gadget */
 					 
		if ((gad = data->lod_IData.idata_Gadget))
		{
		    im->LeftEdge = (gad->Width - im->Width) / 2;
		    im->TopEdge = (gad->Height - im->Height) / 2;
		    
		}
		
		im->ImageData = (UWORD *)im;
		__tmp.magic = BUTTON_MAGIC_LONGWORD;
		im->Width = __tmp.size.Width;
		im->Height = __tmp.size.Height;
	    }
	    break;
	
	case IM_DRAW:
	    data = INST_DATA(cl, im);
	    
	    rp = imsg->imp_RPort;
	    
	    SetDrMd(rp, JAM1);
	    oldfont = rp->Font;
	    font = OpenFont(data->lod_IData.idata_TextAttr);

	    if (font)
	        SetFont(rp, font);
	    else
	        SetFont(rp, GfxBase->DefaultFont);
	
	    SetSoftStyle(rp, data->lod_IData.idata_TextAttr->ta_Style, data->lod_IData.idata_TextAttr->ta_Style);
	    
	    xpos = imsg->imp_Offset.X;
	    ypos = imsg->imp_Offset.Y;
	    
	    pens = imsg->imp_DrInfo->dri_Pens;
	    if (!pens) pens = (UWORD *)defaultpens;
	    
	    if ((gad = data->lod_IData.idata_Gadget))
	    {
	        xpos = gad->LeftEdge;
		ypos = gad->TopEdge;
		
	        SetAPen(rp, pens[(imsg->imp_State == IDS_SELECTED) ? FILLPEN : BACKGROUNDPEN]);

		RectFill(rp, xpos,
			     ypos,
			     gad->LeftEdge + gad->Width - 1,
			     gad->TopEdge + gad->Height -1 );
		im->PlanePick = (imsg->imp_State == IDS_SELECTED) ? FILLTEXTPEN : TEXTPEN; 
	    }
	    
	    xpos += im->LeftEdge;
	    ypos += im->TopEdge;

	    SetAPen(rp, pens[im->PlanePick]);
	    
   	    Move(rp, xpos, ypos + rp->TxBaseline);
	    
	    /* Draw first part of text (entire text if no underscore */
	    
	    if (data->lod_UnderOff)
	        Text(rp, data->lod_IData.idata_Label, data->lod_UnderOff);
	
	    /* Is there an underscore in text */
	    
	    if (im->PlaneOnOff)
	    {
	        /* First draw rest of text */
		
		xpos = rp->cp_x;
		
		Text(rp, data->lod_IData.idata_Label + data->lod_UnderOff + 1, data->lod_RestLen);

		/* Draw underscore */
		
		Move(rp, xpos, ypos + data->lod_UnderY);
		Draw(rp, xpos + data->lod_UnderWidth - 1, ypos + data->lod_UnderY);		
	    }
	    
	    if (gad)
	    {
	        struct TagItem beveltags[] =
		{
		    {GT_VisualInfo	, (IPTR)data->lod_IData.idata_VisualInfo	},
		    {GTBB_Recessed  	, TRUE	    	    	    	    	    	},
		    {TAG_DONE								}
		};
		
		if (imsg->imp_State != IDS_SELECTED) beveltags[1].ti_Tag = TAG_IGNORE;
		
	        /* draw bevelbox around gadget container (recessed if selected) */

		DrawBevelBoxA(rp,
			      gad->LeftEdge,
			      gad->TopEdge,
			      gad->Width,
			      gad->Height,
			      beveltags);
	    }
	    
	    SetFont(rp, oldfont);
	    if (font) CloseFont(font);
	    break;
	    
	default:
	    retval = DoSuperMethodA(cl, (Object *)im, msg);
	    break;
	    
    } /* switch(msg->MethodID) */
    
    return retval;
#ifdef __AROS__
    AROS_USERFUNC_EXIT
#endif    
}

/****************************************************************************************/
/****************************************************************************************/
