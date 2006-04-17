//
// Popup Menu Images
// ©1996-1997 Henrik Isaksson
//

#include <exec/types.h>
#include <intuition/intuition.h>
#include <cybergraphx/cybergraphics.h>
#include <proto/cybergraphics.h>
#include "pmpriv.h"

extern BOOL V40Gfx;

UBYTE chip arrowData[9*2] =
{
	0x80,0x00,0xC0,0x00,0xE0,0x00,0xF0,0x00,0xF8,0x00,0xF0,0x00,0xE0,0x00,0xC0,0x00,
	0x80,0x00
};

struct Image arrow =
{
	0, 0,		/* LeftEdge, TopEdge */
	5, 9, 1,	/* Width, Height, Depth */
	(void *)arrowData,	/* ImageData */
	0x0001, 0x0000,	/* PlanePick, PlaneOnOff */
	NULL		/* NextImage */
};

struct Image *MakeSysImage(struct PM_Root *p, ULONG image, ULONG ist)
{
	struct Image *img=NULL;
/*	struct DrawInfo dri;
	UWORD TP[MAX_PENS];

	CopyMem(p->DrawInfo,&dri,sizeof(struct DrawInfo));
        if(dri.dri_Version>=2) {
            if(ist==IST_INACTIVE) {
                TP[SHINEPEN] = p->DrawInfo->dri_Pens[SHINEPEN];
                TP[SHADOWPEN]=PM_Pens_Get(PMPEN_SHADOW);
                TP[BACKGROUNDPEN]=PM_Pens_Get(PMPEN_BG);
                TP[FILLPEN]=PM_Pens_Get(PMPEN_ACTIVEBG);
                TP[BARBLOCKPEN]=TP[BACKGROUNDPEN];
                TP[BARDETAILPEN]=PM_Pens_Get(PMPEN_TEXT);
            } else {
                TP[SHINEPEN]=PM_Pens_Get(PMPEN_SHINE);
                TP[SHADOWPEN]=PM_Pens_Get(PMPEN_SHADOW);
                TP[BACKGROUNDPEN]=PM_Pens_Get(PMPEN_ACTIVEBG);
                TP[FILLPEN]=PM_Pens_Get(PMPEN_BG);
		TP[BARBLOCKPEN]=TP[BACKGROUNDPEN];
                TP[BARDETAILPEN]=PM_Pens_Get(PMPEN_ACTIVETEXT);
            }
            dri.dri_Pens=TP;
        }*/

        img = NewObject(NULL, SYSICLASS,
            //SYSIA_DrawInfo, &dri,
		SYSIA_DrawInfo, p->DrawInfo,
		SYSIA_Size, SYSISIZE_HIRES,
		SYSIA_Which,    image,
		IA_Left,    0,
		IA_Top,     0,
	    //IA_BGPen,	0,
	    //IA_FGPen,	TP[BARDETAILPEN],
            //IA_Width,   p->MenuFont->tf_YSize,
            //IA_Height,  p->MenuFont->tf_YSize,
            TAG_DONE);


    return img;
}

void PM_Image_Allocate(struct PM_Root *p)
{
	int i;

	for(i = 0; i < PMIMG_LAST; i++)
		p->MenuImages[i] = 0;

	p->MenuImages[PMIMG_AMIGAKEY] = MakeSysImage(p, AMIGAKEY, 0);
	p->MenuImages[PMIMG_CHECKMARK] = MakeSysImage(p, MENUCHECK, 0);
	p->MenuImages[PMIMG_EXCLUDE] = MakeSysImage(p, MENUCHECK, 0);
	p->MenuImages[PMIMG_SUBMENU] = &arrow; //MakeSysImage(p, RIGHTIMAGE, 0);
}

void PM_Image_Free(struct PM_Root *p)
{
	if(p->MenuImages[PMIMG_AMIGAKEY])
		DisposeObject(p->MenuImages[PMIMG_AMIGAKEY]);
	if(p->MenuImages[PMIMG_CHECKMARK])
		DisposeObject(p->MenuImages[PMIMG_CHECKMARK]);
	if(p->MenuImages[PMIMG_EXCLUDE])
		DisposeObject(p->MenuImages[PMIMG_EXCLUDE]);
	/*if(p->MenuImages[PMIMG_SUBMENU])
		DisposeObject(p->MenuImages[PMIMG_SUBMENU]); */
}

struct PrefsImage *PM_Image_Get(ULONG type, struct PopupMenu *item)
{
	return NULL;
}

UWORD PM_Image_Draw(struct PM_Window *w, ULONG type, WORD l, struct DrawInfo *dri, ULONG state, struct PopupMenu *item)
{
#if 1
#warning "trying to get rid of global p"
    	struct PM_Root *p = w->p;
#endif

	if(p->MenuImages[type]) {
		int y = item->Top + item->Height/2 - p->MenuImages[type]->Height/2;
		if(l < 0) l += item->Left + item->Width - p->MenuImages[type]->Width;
		DrawImageState(w->RPort, p->MenuImages[type], l, y, state, dri);
		return p->MenuImages[type]->Width;
	}
	return 0;
}

UWORD PM_Image_Height(struct PM_Root *p, ULONG type, struct PopupMenu *item)
{
	if(p->MenuImages[type])
		return p->MenuImages[type]->Height;
	return 0;
}

UWORD PM_Image_Width(struct PM_Root *p, ULONG type, struct PopupMenu *item)
{
	if(p->MenuImages[type])
		return p->MenuImages[type]->Width;
	return 0;
}
