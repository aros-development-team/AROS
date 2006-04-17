//
// PopupMenu
// ©1996-2000 Henrik Isaksson
//
// Graphic rendering routines
//

#include "pmpriv.h"

//
// Generic PM_Window graphics
//

void PM_DrawBg(struct PM_Window *pw, int x, int y, int xb, int yb)
{
	int xa = x, ya = y;

	if(pw->bg.BgArray) {
		WritePixelArray(pw->bg.BgArray, x, y,
			pw->Width*3, pw->RPort, xa, ya,
			xb-xa, yb-ya, RECTFMT_RGB);

	} else {
		SetAPen(pw->RPort, BGPEN(pw->p));
		SetDrMd(pw->RPort, JAM1);
		RectFill(pw->RPort, xa, ya, xb, yb);
	}
}

void PM_RectFill(struct PM_Window *pw, int xa, int ya, int xb, int yb)
{
    SetDrMd(pw->RPort, JAM1);
    RectFill(pw->RPort, xa, ya, xb, yb);
}

void PM_Move(struct PM_Window *pw, int x, int y)
{
    Move(pw->RPort, x, y);
}

void PM_Pixel(struct PM_Window *pw, int x, int y)
{
    WritePixel(pw->RPort, x, y);
}

void PM_Draw(struct PM_Window *pw, int x, int y)
{
    Draw(pw->RPort, x, y);
}

void PM_DrawImage(struct PM_Window *pw, struct Image *img, int x, int y, struct DrawInfo *dri, ULONG state)
{
    DrawImageState(pw->RPort, img, x, y, state, dri);
}

//
// Ghost an item
//

void PM_Ghost(struct PM_Window *w, int x, int y, int xb, int yb, int pen)
{
        static UWORD      ghostpat[] = { 0x2222, 0x8888 };

        SetAPen(w->RPort, pen);
        SetAfPt(w->RPort, ghostpat, 1);
        PM_RectFill(w, x, y, xb, yb);
        SetAfPt(w->RPort, NULL, 0);
}

//
// Render a ColourBox
//

void ColourBox(struct PM_Window *w, int xa, int ya, int xb, int yb, int pen, int shine, int shade, BOOL selected)
{
    if(selected) {
        PM_DrawBox(w, xa, ya, xb, yb, shade, shine);
    } else {
        PM_DrawBox(w, xa, ya, xb, yb, shine, shade);
    }

    SetAPen(w->RPort, pen);
    PM_RectFill(w, xa+1, ya+1, xb-1, yb-1);
}

//
// This is used by the other functions.
//

void PM_DI_SetTextPen(struct PM_Window *a, struct PopupMenu *pm)
{
        SetAPen(a->RPort, TEXT(a->p));
        if(pm->Flags&NPM_FILLTEXT) SetAPen(a->RPort, FILL(a->p));
        if(pm->Flags&NPM_SHADOWTEXT) SetAPen(a->RPort, SHADOW(a->p));
        if(pm->Flags&NPM_SHINETEXT) SetAPen(a->RPort, SHINE(a->p));
        if(pm->Flags&NPM_HILITETEXT) SetAPen(a->RPort, HILITE(a->p));
        if(pm->Flags&NPM_CUSTOMPEN) SetAPen(a->RPort, pm->TextPen);
}

ULONG PM_RenderCheckMark(struct PM_Window *a, struct PopupMenu *pm, BOOL Selected)
{
        ULONG xoff=0;

        if(pm->Flags&NPM_CHECKIT) { // Should this item have a checkmark?

                if(pm->Flags&NPM_CHECKED) {
			xoff=PM_Image_Draw(a,
				pm->Exclude?PMIMG_EXCLUDE:PMIMG_CHECKMARK,
				pm->Left+PM_Prefs->pmp_XSpace+1,
				a->p->DrawInfo,
				Selected?IDS_SELECTED:IDS_INACTIVESELECTED,
				pm) + PM_Prefs->pmp_Intermediate;
		} else {
			/*
			xoff=PM_Image_Draw(a,
				pm->Exclude?PMIMG_EXCLUDE:PMIMG_CHECKMARK,
				pm->Left+PM_Prefs->pmp_XSpace+1,
				a->p->DrawInfo,
				Selected?IDS_NORMAL:IDS_INACTIVENORMAL,
				pm);
			*/
			xoff = PM_Image_Width(a->p, pm->Exclude?PMIMG_EXCLUDE:PMIMG_CHECKMARK, pm) + PM_Prefs->pmp_Intermediate;
		}

        }

        return xoff+PM_Prefs->pmp_XSpace;
}

int PM_NewDrawItem(struct PM_Window *a, struct PopupMenu *pm, BOOL Selected, BOOL Disabled)
{
	ULONG style, xoff;
	STRPTR pmtitle=pm->Title;

	if(pm->Flags&NPM_HIDDEN) return 0;

	if(GET_TXTMODE(pm)==NPX_TXTLOCALE)
		pmtitle=(STRPTR)CallHook(a->p->LocaleHook, (Object *)pm, (Object *)pm->TitleID);

	if(Selected) {
		SetAPen(a->RPort, FILL(a->p));
		PM_RectFill(a, pm->Left, pm->Top, pm->Left+pm->Width, pm->Top+pm->Height);

		// --- Test to see how it looks... --- //
		//PM_DrawBg(a, pm->Left, pm->Top, pm->Left+pm->Width, pm->Top+pm->Height);
		//PM_DrawShadow(a, pm->Left, pm->Top, pm->Left+pm->Width, pm->Top+pm->Height);
		// --- //

	} else {
		PM_DrawBg(a, pm->Left, pm->Top, pm->Left+pm->Width, pm->Top+pm->Height);
	}

	if(pm->Flags&NPM_GROUP) {
		struct PopupMenu *pmp=pm->Sub;

		if(pmp) do {
			PM_NewDrawItem(a, pmp, Selected, Disabled);
			pmp=pmp->Next;
		} while(pmp);
		return 0;
	}


	if(pm->Flags&NPM_HBAR_BIT) {
		PM_ShortSeparator(a, pm);
		return TRUE;
	} else if(pm->Flags&NPM_WIDE_BAR_BIT) {
		PM_WideSeparator(a, pm);
		return TRUE;
	}

	if(!Selected) { // Use RastPort Pen
		PM_DI_SetTextPen(a, pm);
	}

	/* Text style and font */

	SetFont(a->RPort, a->p->MenuFont);
	SetDrMd(a->RPort, JAM1);

	style = 0;

	if(pm->Flags&NPM_UNDERLINEDTEXT) style|=FSF_UNDERLINED;
	if(pm->Flags&NPM_BOLDTEXT) style|=FSF_BOLD;
	if(pm->Flags&NPM_ITALICTEXT) style|=FSF_ITALIC;

	SetSoftStyle(a->RPort, style, ~0);

        xoff=PM_RenderCheckMark(a, pm, Selected);

	if(pm->Flags&NPM_COLOURBOX) {
		if(pm->Flags&NPM_CHECKIT) {
			if(pm->CommKey) {
				int x1=(a->Width-3*a->RPort->Font->tf_XSize-PM_Prefs->pmp_XSpace-a->p->BorderWidth)-a->RPort->Font->tf_XSize;

				x1-=a->RPort->Font->tf_XSize*3+PM_Prefs->pmp_Intermediate;

				ColourBox(a, x1, pm->Top+PM_Prefs->pmp_YSpace+1, x1+3*a->RPort->Font->tf_XSize, pm->Top+pm->Height-PM_Prefs->pmp_YSpace-1, pm->CBox, SHINE(a->p), SHADOW(a->p), Selected);
			} else {
				int x1 = (pm->Width + pm->Left - 3*a->RPort->Font->tf_XSize - PM_Prefs->pmp_XSpace - a->p->BorderWidth);
				ColourBox(a, x1, pm->Top + PM_Prefs->pmp_YSpace + 1,
				x1 + 3*a->RPort->Font->tf_XSize, pm->Top + pm->Height - PM_Prefs->pmp_YSpace - 1,
				pm->CBox, SHINE(a->p), SHADOW(a->p), Selected);
			}
		} else {
			ColourBox(a, pm->Left+PM_Prefs->pmp_XSpace, pm->Top+PM_Prefs->pmp_YSpace+1, pm->Left+3*a->RPort->Font->tf_XSize, pm->Top+pm->Height-PM_Prefs->pmp_YSpace-1, pm->CBox, SHINE(a->p), SHADOW(a->p), Selected);
			xoff+=PM_Prefs->pmp_XSpace+3*a->RPort->Font->tf_XSize;
		}
	}

	if(!a->p->PullDown) {
		if(pm->Sub) {
			PM_Image_Draw(a,
				PMIMG_SUBMENU,
				-PM_Prefs->pmp_XSpace,
				a->p->DrawInfo,
				Selected?IDS_NORMAL:IDS_INACTIVENORMAL,
				pm);
		}
	}

        if(Selected) {
            if(pm->Flags&NPM_ISIMAGE) {
                    if(pm->Images[1]) {
                    if(pm->Flags&NPM_CENTERED) {
                    PM_DrawImage(a, pm->Images[1],
                        pm->Left+PM_Prefs->pmp_XSpace+ (pm->Width/2-pm->Images[1]->Width/2)>xoff?pm->Width/2-pm->Images[1]->Width/2:xoff,
                        YPosImage_(a, pm, pm->Images[1]), a->p->DrawInfo, IDS_SELECTED);
                } else {
                    PM_DrawImage(a, pm->Images[1], pm->Left+PM_Prefs->pmp_XSpace+xoff+a->IconColumn, YPosImage_(a, pm, pm->Images[1]), a->p->DrawInfo, IDS_SELECTED);
                }
            }
        } else {
            if(pm->Images[1]) {
                WORD ixoff;
                ixoff=a->IconColumn/2-pm->Images[1]->Width/2;
                        PM_DrawImage(a, pm->Images[1], pm->Left+PM_Prefs->pmp_XSpace+ixoff, YPosImage_(a, pm, pm->Images[1]), a->p->DrawInfo, IDS_SELECTED);
            }
        }
        } else {
            if(pm->Flags&NPM_ISIMAGE) {
                    if(pm->Images[0]) {
                    if(pm->Flags&NPM_CENTERED) {
                    PM_DrawImage(a, pm->Images[0],
                        pm->Left+PM_Prefs->pmp_XSpace+ (pm->Width/2-pm->Images[0]->Width/2)>xoff?pm->Width/2-pm->Images[0]->Width/2:xoff,
                        YPosImage_(a, pm, pm->Images[0]), a->p->DrawInfo, IDS_SELECTED);
                } else {
                    PM_DrawImage(a, pm->Images[0], pm->Left+PM_Prefs->pmp_XSpace+xoff+a->IconColumn, YPosImage_(a, pm, pm->Images[0]), a->p->DrawInfo, IDS_NORMAL);
                }
            }
        } else {
            if(pm->Images[0]) {
                WORD ixoff;
                ixoff=a->IconColumn/2-pm->Images[0]->Width/2;
                PM_DrawImage(a, pm->Images[0], pm->Left+PM_Prefs->pmp_XSpace+ixoff, YPosImage_(a, pm, pm->Images[0]), a->p->DrawInfo, IDS_NORMAL);
            }
        }
        }

	if(a->IconColumn) {
		xoff+=a->IconColumn+PM_Prefs->pmp_Intermediate;
	}

        if(pm->Flags&NPM_CENTERED) {
                int offs, tw;

                tw=TextLength(a->RPort,pmtitle,strlen(pmtitle));
                offs=pm->Width/2-tw/2;

                if(xoff<offs) xoff=offs;
        }

    if((pm->Flags&NPM_DISABLED || Disabled) && !PM_Prefs->pmp_SeparatorBar) {
                SetAPen(a->RPort, BGSHINE(a->p));
                PM_Move(a, pm->Left+xoff+1, YPosText(a, pm)+1);
                if(pmtitle) Text(a->RPort, pmtitle, strlen(pmtitle));
        } else if(pm->Flags&NPM_SHADOWED) {
                SetAPen(a->RPort, TEXTSHADOW(a->p));
                PM_Move(a, pm->Left+xoff+1, YPosText(a, pm)+1);
                if(pmtitle) Text(a->RPort, pmtitle, strlen(pmtitle));
        } else if(pm->Flags&NPM_OUTLINED) {
        if(pmtitle) {
                    SetAPen(a->RPort, TEXTOUTLINE(a->p));
                    PM_Move(a, pm->Left+xoff-1, YPosText(a, pm)-1);
                    Text(a->RPort, pmtitle, strlen(pmtitle));
                    PM_Move(a, pm->Left+xoff+1, YPosText(a, pm)+1);
                    Text(a->RPort, pmtitle, strlen(pmtitle));
                    PM_Move(a, pm->Left+xoff+1, YPosText(a, pm)-1);
                    Text(a->RPort, pmtitle, strlen(pmtitle));
                    PM_Move(a, pm->Left+xoff-1, YPosText(a, pm)+1);
                    Text(a->RPort, pmtitle, strlen(pmtitle));
                    PM_Move(a, pm->Left+xoff-1, YPosText(a, pm)+1);
                    Text(a->RPort, pmtitle, strlen(pmtitle));
                    PM_Move(a, pm->Left+xoff+1, YPosText(a, pm));
                    Text(a->RPort, pmtitle, strlen(pmtitle));
                    PM_Move(a, pm->Left+xoff-1, YPosText(a, pm));
                    Text(a->RPort, pmtitle, strlen(pmtitle));
                    PM_Move(a, pm->Left+xoff, YPosText(a, pm)+1);
                    Text(a->RPort, pmtitle, strlen(pmtitle));
                    PM_Move(a, pm->Left+xoff, YPosText(a, pm)-1);
                    Text(a->RPort, pmtitle, strlen(pmtitle));
        }
        } else if(pm->Flags&NPM_EMBOSSED) {
        if(pmtitle) {
                    SetAPen(a->RPort, SHINE(a->p));
                    PM_Move(a, pm->Left+xoff-1, YPosText(a, pm)-1);
                    Text(a->RPort, pmtitle, strlen(pmtitle));
                    PM_Move(a, pm->Left+xoff-1, YPosText(a, pm));
                    Text(a->RPort, pmtitle, strlen(pmtitle));
                    PM_Move(a, pm->Left+xoff, YPosText(a, pm)-1);
                    Text(a->RPort, pmtitle, strlen(pmtitle));
                    SetAPen(a->RPort, SHADOW(a->p));
                    PM_Move(a, pm->Left+xoff+1, YPosText(a, pm)+1);
                    Text(a->RPort, pmtitle, strlen(pmtitle));
                    PM_Move(a, pm->Left+xoff+1, YPosText(a, pm));
                    Text(a->RPort, pmtitle, strlen(pmtitle));
                    PM_Move(a, pm->Left+xoff, YPosText(a, pm)+1);
                    Text(a->RPort, pmtitle, strlen(pmtitle));
        }
        }

        if(!Selected) {
                PM_DI_SetTextPen(a, pm);
        } else {
        SetAPen(a->RPort, FTPEN(a->p));
        }

        if(pm->Flags&NPM_ISSELECTED) {
                if(pm->Flags&NPM_CHECKIT) {
                    if((!(pm->Flags&NPM_CHECKED) && pm->Flags&NPM_INITIAL_CHECKED) ||
                       ((pm->Flags&NPM_CHECKED) && !(pm->Flags&NPM_INITIAL_CHECKED))) {
                        SetAPen(a->RPort, HILITE(a->p));
                    }
                } else {
                    SetAPen(a->RPort, HILITE(a->p));
                }
        }

        if(pm->CommKey) {
                char x[2];

		PM_Image_Draw(a,
			PMIMG_AMIGAKEY,
			-PM_Prefs->pmp_XSpace-a->RPort->Font->tf_XSize-PM_Prefs->pmp_Intermediate,
			a->p->DrawInfo,
			Selected?IDS_NORMAL:IDS_INACTIVENORMAL,
			pm);

                x[0]=pm->CommKey;
                PM_Move(a, XPosLastCol(a), YPosText(a,pm));
                Text(a->RPort, x, 1);
        }

        PM_Move(a, pm->Left+xoff, YPosText(a, pm));
	if((pm->Flags&NPM_DISABLED || Disabled) && !PM_Prefs->pmp_SeparatorBar) SetAPen(a->RPort, BGSHADOW(a->p));   // Set text colour to 'bgminus' if disabled
        if(pmtitle) Text(a->RPort, pmtitle, strlen(pmtitle));

    if(PM_Prefs->pmp_SelItemBorder && Selected) {
        if(PM_Prefs->pmp_MenuBorder==MAGIC_FRAME) {
            if(PM_Prefs->pmp_SelItemBorder==1) /* Raised */
                PM_DrawBoxMM2(a, pm->Left, pm->Top, pm->Left+pm->Width, pm->Top+pm->Height, SHINE(a->p), SHADOW(a->p), HALF(a->p));
            else
                PM_DrawBoxMM2(a, pm->Left, pm->Top, pm->Left+pm->Width, pm->Top+pm->Height, SHADOW(a->p), SHINE(a->p), HALF(a->p));
        } else {
            if(PM_Prefs->pmp_SelItemBorder==1) /* Raised */
                PM_DrawBox(a, pm->Left, pm->Top, pm->Left+pm->Width, pm->Top+pm->Height, SHINE(a->p), SHADOW(a->p));
            else
                PM_DrawBox(a, pm->Left, pm->Top, pm->Left+pm->Width, pm->Top+pm->Height, SHADOW(a->p), SHINE(a->p));
        }
    }

    if(pm->Flags&NPM_DISABLED && PM_Prefs->pmp_SeparatorBar) {
        PM_Ghost(a, pm->Left, pm->Top, pm->Left+pm->Width, pm->Top+pm->Height, SHADOW(a->p));
    }

        return 0;
}

//
// Draw a horizontal bar
//

void PM_Separator(struct PM_Window *w, int x, int y, int x2, int shine, int shadow)
{
        PM_Move(w, x, y-1);
        SetAPen(w->RPort, shadow);
        PM_Draw(w, x2, y-1);
        PM_Move(w, x, y);
        SetAPen(w->RPort, shine);
        PM_Draw(w, x2, y);
}

void PM_XENSeparator(struct PM_Window *w, int x, int y, int x2, int shine, int shadow, int bgplus, int bgminus)
{
        PM_Move(w, x+1, y-2);
        SetAPen(w->RPort, bgminus);
        PM_Draw(w, x2-1, y-2);

        PM_Move(w, x, y-1);
        SetAPen(w->RPort, shadow);
        PM_Draw(w, x2, y-1);

        PM_Move(w, x, y);
        SetAPen(w->RPort, shine);
        PM_Draw(w, x2, y);

        PM_Move(w, x+1, y+1);
        SetAPen(w->RPort, bgplus);
        PM_Draw(w, x2-1, y+1);
}

void PM_XENSeparatorHalf(struct PM_Window *w, int x, int y, int x2, int shine, int shadow, int bgplus, int bgminus, int half)
{
        PM_Move(w, x+1, y-2);
        SetAPen(w->RPort, bgminus);
        PM_Draw(w, x2-1, y-2);

        PM_Move(w, x, y-1);
        SetAPen(w->RPort, shadow);
        PM_Draw(w, x2, y-1);

        PM_Move(w, x, y);
        SetAPen(w->RPort, shine);
        PM_Draw(w, x2, y);

        PM_Move(w, x+1, y+1);
        SetAPen(w->RPort, bgplus);
        PM_Draw(w, x2-1, y+1);
}

void PM_NewSeparator(struct PM_Window *w, int x, int y, int x2, int shine, int shadow)
{
        PM_Move(w, x, y);
        SetAPen(w->RPort, shadow);
        PM_Draw(w, x, y-1);
        PM_Draw(w, x2-1, y-1);
        PM_Move(w, x+1, y);
        SetAPen(w->RPort, shine);
        PM_Draw(w, x2, y);
        PM_Draw(w, x2, y-1);
}

void PM_OldSeparator(struct PM_Window *w, int x, int y, int x2, int shine, int shadow)
{
        PM_Move(w, x, y-1);
        SetAPen(w->RPort, shadow);
        SetDrPt(w->RPort, 0xbbbb);
        PM_Draw(w, x2, y-1);

        PM_Move(w, x, y);
        SetAPen(w->RPort, shadow);
        SetDrPt(w->RPort, 0xeeee);
        PM_Draw(w, x2, y);

        SetDrPt(w->RPort, 0xffff);
}

void PM_ShortSeparator(struct PM_Window *a, struct PopupMenu *pm)
{
    if(!PM_Prefs->pmp_SeparatorBar) {
        if(PM_Prefs->pmp_MenuBorder==MAGIC_FRAME)
            PM_NewSeparator(a, BarLeft(a, pm), BarTop(a, pm), BarRight(a, pm), BGSHINE(a->p), BGSHADOW(a->p));
        else PM_NewSeparator(a, BarLeft(a, pm), BarTop(a, pm), BarRight(a, pm), SHINE(a->p), SHADOW(a->p));
    } else PM_OldSeparator(a, BarLeft(a, pm), BarTop(a, pm), BarRight(a, pm), SHINE(a->p), SHADOW(a->p));
}

void PM_WideSeparator(struct PM_Window *a, struct PopupMenu *pm)
{
    if(!PM_Prefs->pmp_SeparatorBar) {
        if(PM_Prefs->pmp_MenuBorder==MAGIC_FRAME) {
            PM_XENSeparatorHalf(a, 0, BarTop(a, pm), a->Width-2, SHINE(a->p), SHADOW(a->p), BGSHINE(a->p), BGSHADOW(a->p), HALF(a->p));
        } else if(PM_Prefs->pmp_MenuBorder==DROPBOX_FRAME) {
                PM_Move(a, a->p->BorderWidth-1, BarTop(a, pm)-2);
                SetAPen(a->RPort, SHINE(a->p));
            PM_Draw(a, a->Width-a->p->BorderWidth, BarTop(a, pm)-2);
            PM_Move(a, a->p->BorderWidth-1, BarTop(a, pm)+1);
            SetAPen(a->RPort, SHADOW(a->p));
            PM_Draw(a, a->Width-a->p->BorderWidth, BarTop(a, pm)+1);

            SetAPen(a->RPort, BGPEN(a->p));
            PM_Move(a, a->p->BorderWidth-1, BarTop(a, pm)-1);
            PM_Draw(a, a->p->BorderWidth-1, BarTop(a, pm));
            PM_Move(a, a->Width-a->p->BorderWidth, BarTop(a, pm)-1);
            PM_Draw(a, a->Width-a->p->BorderWidth, BarTop(a, pm));
        } else {
            PM_Separator(a, a->p->BorderWidth-1, BarTop(a, pm), a->Width-a->p->BorderWidth-1, SHINE(a->p), SHADOW(a->p));
        }
    } else {
        SetAPen(a->RPort, SHADOW(a->p));
        PM_Move(a, a->p->BorderWidth, BarTop(a, pm));
        PM_Draw(a, a->Width-a->p->BorderWidth, BarTop(a, pm));
        PM_Move(a, a->p->BorderWidth, BarTop(a, pm)+1);
        PM_Draw(a, a->Width-a->p->BorderWidth, BarTop(a, pm)+1);
    }
}

//
// Draw a bevel box
//

void PM_DrawBoxMM2(struct PM_Window *w, int x1, int y1, int x2, int y2, int shine, int shadow, int halfshine)
{
    SetAPen(w->RPort, halfshine);
    PM_Pixel(w, x1, y2);
    PM_Pixel(w, x2, y1);
        SetAPen(w->RPort, shine);
        PM_Move(w, x1, y2-1);
        PM_Draw(w, x1, y1);
        PM_Draw(w, x2-1, y1);
        SetAPen(w->RPort, shadow);
        PM_Move(w, x1+1, y2);
        PM_Draw(w, x2, y2);
        PM_Draw(w, x2, y1+1);
}

void PM_DrawXENBoxMM2(struct PM_Window *w, int x1, int y1, int x2, int y2, int shine, int shadow, int bgplus, int bgminus, int halfshine)
{
        SetAPen(w->RPort, shine);
        PM_Move(w, x1, y2);
        PM_Draw(w, x1, y1);
        PM_Draw(w, x2, y1);
        SetAPen(w->RPort, bgplus);
        PM_Move(w, x1+1, y2-1);
        PM_Draw(w, x1+1, y1+1);
        PM_Draw(w, x2-1, y1+1);
        SetAPen(w->RPort, shadow);
        PM_Move(w, x1, y2);
        PM_Draw(w, x2, y2);
        PM_Draw(w, x2, y1);
        SetAPen(w->RPort, bgminus);
        PM_Move(w, x1+1, y2-1);
        PM_Draw(w, x2-1, y2-1);
        PM_Draw(w, x2-1, y1+1);
    SetAPen(w->RPort, halfshine);
    PM_Pixel(w, x1, y2);
    PM_Pixel(w, x2, y1);
}

void PM_DrawBox(struct PM_Window *w, int x1, int y1, int x2, int y2, int shine, int shadow)
{
        SetAPen(w->RPort, shine);
        PM_Move(w, x1, y2-1);
        PM_Draw(w, x1, y1);
        PM_Draw(w, x2-1, y1);
        SetAPen(w->RPort, shadow);
        PM_Move(w, x1, y2);
        PM_Draw(w, x2, y2);
        PM_Draw(w, x2, y1);

}

void PM_DrawIBox(struct PM_Window *w, int x1, int y1, int x2, int y2, int shine, int shadow)
{
        SetAPen(w->RPort, shadow);
        PM_Move(w, x1, y2);
        PM_Draw(w, x1, y1);
        PM_Draw(w, x2, y1);

        PM_Move(w, x1, y2);
        PM_Draw(w, x2, y2);
        PM_Draw(w, x2, y1);

    PM_Move(w, x1+1, y1);
    PM_Draw(w, x1+1, y2);

    PM_Move(w, x2-1, y1);
    PM_Draw(w, x2-1, y2);
}

void PM_DrawXENBox(struct PM_Window *w, int x1, int y1, int x2, int y2, int shine, int shadow, int bgplus, int bgminus)
{
        SetAPen(w->RPort, shine);
        PM_Move(w, x1, y2);
        PM_Draw(w, x1, y1);
        PM_Draw(w, x2, y1);
        SetAPen(w->RPort, bgplus);
        PM_Move(w, x1+1, y2-1);
        PM_Draw(w, x1+1, y1+1);
        PM_Draw(w, x2-1, y1+1);
        SetAPen(w->RPort, shadow);
        PM_Move(w, x1, y2);
        PM_Draw(w, x2, y2);
        PM_Draw(w, x2, y1);
        SetAPen(w->RPort, bgminus);
        PM_Move(w, x1+1, y2-1);
        PM_Draw(w, x2-1, y2-1);
        PM_Draw(w, x2-1, y1+1);
}

void PM_DrawDBLBox(struct PM_Window *w, int x1, int y1, int x2, int y2, int shine, int shadow)
{
        PM_DrawBox(w, x1, y1, x2, y2, shine, shadow);
        PM_DrawBox(w, x1+1, y1+1, x2-1, y2-1, shine, shadow);
}

void PM_DrawDropBox(struct PM_Window *w, int x1, int y1, int x2, int y2, int shine, int shadow)
{
        SetAPen(w->RPort, shine);
        PM_Move(w, x1, y2-1);
        PM_Draw(w, x1, y1);
        PM_Draw(w, x2-1, y1);
        PM_Move(w, x1+4, y2-3);
        PM_Draw(w, x2-3, y2-3);
        PM_Draw(w, x2-3, y1+4);
        SetAPen(w->RPort, shadow);
        PM_Move(w, x1+3, y2-3);
        PM_Draw(w, x1+3, y1+3);
        PM_Draw(w, x2-4, y1+3);
        PM_Move(w, x1, y2);
        PM_Draw(w, x2, y2);
        PM_Draw(w, x2, y1);
}

void PM_DrawPrefBox(struct PM_Root *p, struct PM_Window *w, int x1, int y1, int x2, int y2)
{
        switch(PM_Prefs->pmp_MenuBorder) {
                case BUTTON_FRAME:
                        PM_DrawBox(w, x1, y1, x2, y2, SHINE(p), SHADOW(p));
                        break;
                case MAGIC_FRAME:
                        PM_DrawXENBoxMM2(w, x1, y1, x2, y2, SHINE(p), SHADOW(p), BGSHINE(p), BGSHADOW(p), HALF(p));
                        break;
                case THICK_BUTTON_FRAME:
                        PM_DrawXENBox(w, x1, y1, x2, y2, SHINE(p), SHADOW(p), SHINE(p), SHADOW(p));
                        break;
                case DOUBLE_FRAME:
                        PM_DrawDBLBox(w, x1, y1, x2, y2, SHINE(p), SHADOW(p));
                        break;
        case DROPBOX_FRAME:
                        PM_DrawDropBox(w, x1, y1, x2, y2, SHINE(p), SHADOW(p));
            break;
        case INTUI_FRAME:
            PM_DrawIBox(w, x1, y1, x2, y2, 0, SHADOW(p));
            break;
        }
}

