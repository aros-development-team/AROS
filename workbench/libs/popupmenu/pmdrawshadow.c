//
// pmdrawshadow.c - Renders shadows, uses CyberGfx if available.
//
// Copyright (C) 1996 - 2002 Henrik Isaksson
// All Rights Reserved.
//

#include <cybergraphx/cybergraphics.h>
#include <proto/cybergraphics.h>
#include "pmpriv.h"

// Macros that extract colour information

#define PC16BIT1(x)     ((x&0x1f00)>>8)
#define PC16BIT2(x)     ((x&0x00F8)>>3)
#define PC16BIT3(x)     (((x&0xE000)>>13)|((x&0x0007)<<3))

#define PC15BIT1(x)     ((x&0x1f00)>>8)
#define PC15BIT2(x)     ((x&0x007C)>>2)
#define PC15BIT3(x)     (((x&0xE000)>>13)|((x&0x0003)<<3))

#define M16RED(x)       ((x&0xF800)>>11)
#define M16GREEN(x)     ((x&0x07E0)>>5)
#define M16BLUE(x)      (x&0x001F)

#define M15RED(x)       ((x&0x7C00)>>10)
#define M15GREEN(x)     ((x&0x03E0)>>5)
#define M15BLUE(x)      (x&0x001F)

// Clamping macro, make sure that x stays within l <= x <= u

#define CLAMP(x, l, u)      x=x>u?x=u:x;x=x<l?x=l:x;

struct CGFXHookMsg {
    APTR    memptr;
    ULONG   ox, oy;
    ULONG   xs, ys;
    UWORD   bytesperrow, bytesperpix;
    UWORD   colormodel;
};

struct ShRect {
    UWORD   xa, ya;
    UWORD   xb, yb;
    BYTE r, g, b;
};

// shadefunc - CGFX hook that processes a ShRect structure passed in
// hook->h_Data.
ULONG shadefunc(struct Hook *hook, struct RastPort *rp, struct CGFXHookMsg *m)
{
    UBYTE		*ptr=(UBYTE *)m->memptr;
    UWORD		*wrow;
    struct ShRect	*rect = (struct ShRect *)hook->h_Data;
    register    WORD	tr, tg, tb;
    register    UWORD	tmpw;
    BYTE		r, g, b;
    UBYTE		*row;
    int			y, z = m->bytesperrow;
    int			x;

    r = rect->r;
    g = rect->g;
    b = rect->b;

    switch(m->colormodel) {
        case PIXFMT_ARGB32:
            for(y=rect->ya;y<rect->yb;y++) {
                row=&ptr[y*z];
                for(x=rect->xa*4;x<4*rect->xb;x+=4) {
                    tr=row[x+1];
                    tr=tr+r;
                    CLAMP(tr, 0, 255);
                    row[x+1]=tr;

                    tg=row[x+2];
                    tg=tg+g;
                    CLAMP(tg, 0, 255);
                    row[x+2]=tg;

                    tb=row[x+3];
                    tb=tb+b;
                    CLAMP(tb, 0, 255);
                    row[x+3]=tb;
                }
            }
            break;
        case PIXFMT_BGRA32:
            for(y=rect->ya;y<rect->yb;y++) {
                row=&ptr[y*z];
                for(x=rect->xa*4;x<4*rect->xb;x+=4) {
                    tb=row[x];
                    tb=tb+b;
                    CLAMP(tb, 0, 255);
                    row[x]=tb;

                    tg=row[x+1];
                    tg=tg+g;
                    CLAMP(tg, 0, 255);
                    row[x+1]=tg;

                    tr=row[x+2];
                    tr=tr+r;
                    CLAMP(tr, 0, 255);
                    row[x+2]=tr;
                }
            }
            break;
        case PIXFMT_RGBA32:
            for(y=rect->ya;y<rect->yb;y++) {
                row=&ptr[y*z];
                for(x=rect->xa*4;x<4*rect->xb;x+=4) {
                    tr=row[x];
                    tr=tr+r;
                    CLAMP(tr, 0, 255);
                    row[x]=tr;

                    tg=row[x+1];
                    tg=tg+g;
                    CLAMP(tg, 0, 255);
                    row[x+1]=tg;

                    tb=row[x+2];
                    tb=tb+b;
                    CLAMP(tb, 0, 255);
                    row[x+2]=tb;
                }
            }
            break;

        case PIXFMT_RGB24:
            for(y=rect->ya;y<rect->yb;y++) {
                row=&ptr[y*z];
                for(x=rect->xa*3;x<3*rect->xb;x+=3) {
                    tr=row[x];
                    tr=tr+r;
                    CLAMP(tr, 0, 255);
                    row[x]=tr;

                    tg=row[x+1];
                    tg=tg+g;
                    CLAMP(tg, 0, 255);
                    row[x+1]=tg;

                    tb=row[x+2];
                    tb=tb+b;
                    CLAMP(tb, 0, 255);
                    row[x+2]=tb;
                }
            }
            break;
        case PIXFMT_BGR24:
		//kprintf("bgr24\n");
            for(y=rect->ya;y<rect->yb;y++) {
                row=&ptr[y*z];
                for(x=rect->xa*3;x<3*rect->xb;x+=3) {
                    tb=row[x];
                    tb=tb+b;
                    CLAMP(tb, 0, 255);
                    row[x]=tb;

                    tg=row[x+1];
                    tg=tg+g;
                    CLAMP(tg, 0, 255);
                    row[x+1]=tg;

                    tr=row[x+2];
                    tr=tr+r;
                    CLAMP(tr, 0, 255);
                    row[x+2]=tr;
                }
            }
            break;

        case PIXFMT_RGB16:
            r>>=3;
            b>>=3;
            g>>=2;
            for(y=rect->ya;y<rect->yb;y++) {
                wrow=(UWORD *)&ptr[y*z];
                for(x=rect->xa;x<rect->xb;x++) {
                    tmpw=wrow[x];
                    tr=M16RED(tmpw);
                    tr=tr+r;
                    CLAMP(tr, 0, 31);
                    tb=M16BLUE(tmpw);
                    tb=tb+b;
                    CLAMP(tb, 0, 31);
                    tg=M16GREEN(tmpw);
                    tg=tg+g;
                    CLAMP(tg, 0, 63);
                    wrow[x]=(tr<<11)|(tb)|(tg<<5);
                }
            }
            break;
        case PIXFMT_BGR16:
            r>>=3;
            b>>=3;
            g>>=2;
            for(y=rect->ya;y<rect->yb;y++) {
                wrow=(UWORD *)&ptr[y*z];
                for(x=rect->xa;x<rect->xb;x++) {
                    tmpw=wrow[x];
                    tr=M16BLUE(tmpw);
                    tr=tr+r;
                    CLAMP(tr, 0, 31);
                    tb=M16RED(tmpw);
                    tb=tb+b;
                    CLAMP(tb, 0, 31);
                    tg=M16GREEN(tmpw);
                    tg=tg+g;
                    CLAMP(tg, 0, 63);
                    wrow[x]=(tr<<11)|(tb)|(tg<<5);
                }
            }
            break;
        case PIXFMT_BGR16PC:
            r>>=3;
            b>>=3;
            g>>=2;
            for(y=rect->ya;y<rect->yb;y++) {
                wrow=(UWORD *)&ptr[y*z];
                for(x=rect->xa;x<rect->xb;x++) {
                    tmpw=wrow[x];
                    tr=PC16BIT1(tmpw);
                    tr=tr+r;
                    CLAMP(tr, 0, 31);
                    tb=PC16BIT2(tmpw);
                    tb=tb+b;
                    CLAMP(tb, 0, 31);
                    tg=PC16BIT3(tmpw);
                    tg=tg+g;
                    CLAMP(tg, 0, 63);
                    wrow[x]=(tr<<8)|(tb<<3)|((tg&0x38)>>3)|((tg&0x7)<<13);
                }
            }
            break;
        case PIXFMT_RGB16PC:
            r>>=3;
            b>>=3;
            g>>=2;
            for(y=rect->ya;y<rect->yb;y++) {
                wrow=(UWORD *)&ptr[y*z];
                for(x=rect->xa;x<rect->xb;x++) {
                    tmpw=wrow[x];
                    tb=PC16BIT1(tmpw);
                    tb=tb+b;
                    CLAMP(tb, 0, 31);
                    tr=PC16BIT2(tmpw);
                    tr=tr+r;
                    CLAMP(tr, 0, 31);
                    tg=PC16BIT3(tmpw);
                    tg=tg+g;
                    CLAMP(tg, 0, 63);
                    wrow[x]=(tb<<8)|(tr<<3)|((tg&0x38)>>3)|((tg&0x7)<<13);
                }
            }
            break;
        case PIXFMT_RGB15:
            r>>=3;
            b>>=3;
            g>>=3;
            for(y=rect->ya;y<rect->yb;y++) {
                wrow=(UWORD *)&ptr[y*z];
                for(x=rect->xa;x<rect->xb;x++) {
                    tmpw=wrow[x];
                    tr=M15RED(tmpw);
                    tr=tr+r;
                    CLAMP(tr, 0, 31);
                    tb=M15BLUE(tmpw);
                    tb=tb+b;
                    CLAMP(tb, 0, 31);
                    tg=M15GREEN(tmpw);
                    tg=tg+g;
                    CLAMP(tg, 0, 31);
                    wrow[x]=(tr<<10)|(tb)|(tg<<5);
                }
            }
            break;
        case PIXFMT_BGR15:
            r>>=3;
            b>>=3;
            g>>=3;
            for(y=rect->ya;y<rect->yb;y++) {
                wrow=(UWORD *)&ptr[y*z];
                for(x=rect->xa;x<rect->xb;x++) {
                    tmpw=wrow[x];
                    tr=M15BLUE(tmpw);
                    tr=tr+r;
                    CLAMP(tr, 0, 31);
                    tb=M15RED(tmpw);
                    tb=tb+b;
                    CLAMP(tb, 0, 31);
                    tg=M15GREEN(tmpw);
                    tg=tg+g;
                    CLAMP(tg, 0, 31);
                    wrow[x]=(tr<<10)|(tb)|(tg<<5);
                }
            }
            break;
        case PIXFMT_BGR15PC:
            r>>=3;
            b>>=3;
            g>>=3;
            for(y=rect->ya;y<rect->yb;y++) {
                wrow=(UWORD *)&ptr[y*z];
                for(x=rect->xa;x<rect->xb;x++) {
                    tmpw=wrow[x];
                    tr=PC15BIT1(tmpw);
                    tr=tr+r;
                    CLAMP(tr, 0, 31);
                    tb=PC15BIT2(tmpw);
                    tb=tb+b;
                    CLAMP(tb, 0, 31);
                    tg=PC15BIT3(tmpw);
                    tg=tg+g;
                    CLAMP(tg, 0, 31);
                    wrow[x]=(tr<<8)|(tb<<2)|((tg)>>3)|((tg&0x7)<<13);
                }
            }
            break;
        case PIXFMT_RGB15PC:
            r>>=3;
            b>>=3;
            g>>=3;
            for(y=rect->ya;y<rect->yb;y++) {
                wrow=(UWORD *)&ptr[y*z];
                for(x=rect->xa;x<rect->xb;x++) {
                    tmpw=wrow[x];
                    tb=PC15BIT1(tmpw);
                    tb=tb+b;
                    CLAMP(tb, 0, 31);
                    tr=PC15BIT2(tmpw);
                    tr=tr+r;
                    CLAMP(tr, 0, 31);
                    tg=PC15BIT3(tmpw);
                    tg=tg+g;
                    CLAMP(tg, 0, 31);
                    wrow[x]=(tb<<8)|(tr<<2)|((tg)>>3)|((tg&0x7)<<13);
                }
            }
            break;
    }

    return 0;
}

// PM_DrawShadow - This function draws a shadow in a PM_Window. The shadow
// will be a rectangle defined by x, y - xb, yb (left, top - right, bottom)

void PM_DrawShadow(struct PM_Window *w, int x, int y, int xb, int yb)
{
    static UWORD	pat[] = { 0xaaaa, 0x5555 };
#ifndef __AROS__
    struct Hook		shadehook;
    struct ShRect	rect;
    ULONG		depth = 0, iscgfx = FALSE;
//    BOOL		realshadow = PM_Prefs->RealShadows;
    BOOL		realshadow = TRUE;

    // Shall we draw a real CGFX shadow?
    if(CyberGfx && realshadow) {
    	iscgfx=GetCyberMapAttr(w->RPort->BitMap, CYBRMATTR_ISCYBERGFX);
    	if(iscgfx) depth=GetCyberMapAttr(w->RPort->BitMap, CYBRMATTR_DEPTH);
    	if(iscgfx && depth>8) {
            rect.xa = x + w->Wnd->LeftEdge;
            rect.xb = xb + 1 + w->Wnd->LeftEdge;
            rect.ya = y + w->Wnd->TopEdge;
            rect.yb = yb + 1 + w->Wnd->TopEdge;

            rect.r = PM_Prefs->pmp_ShadowR;
            rect.g = PM_Prefs->pmp_ShadowG;
            rect.b = PM_Prefs->pmp_ShadowB;

            shadehook.h_Data = &rect;
	    shadehook.h_Entry = HookEntry;
            shadehook.h_SubEntry = (HOOKFUNC)shadefunc;
                
            DoCDrawMethodTagList(&shadehook, w->RPort, NULL);

        } else realshadow = FALSE;
    } else realshadow = FALSE;

    if(!realshadow)
#endif    
    {

	    // A "real" shadow could not be drawn, or it was not desired.

            SetAPen(w->RPort, 1); 	// Should use DRIPen
            SetAfPt(w->RPort, pat, 1);
            PM_RectFill(w, x, y, xb, yb);
            SetAfPt(w->RPort, NULL, 0);
    }
}

