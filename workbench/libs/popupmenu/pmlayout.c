//
// PopupMenu
// ©1996-2000 Henrik Isaksson
//
// Menu Item Layout
//

#include "pmpriv.h"

UWORD PM_ItemHeight(struct PM_Window *a, struct PopupMenu *pm)
{
        UWORD r,fonty=a->p->MenuFont->tf_YSize;

	if(pm->Flags&NPM_HBAR_BIT) {
		r=(UWORD)(PM_Prefs->pmp_YSpace*2+5);
		pm->Flags|=NPM_FIXEDSIZE;
	} else if(pm->Flags&NPM_WIDE_BAR_BIT) {
		r=(UWORD)(PM_Prefs->pmp_YOffset*2+5);
		pm->Flags|=NPM_FIXEDSIZE;
	} else {
                r=(UWORD)(fonty+1);

                if(pm->Images[0]) { if(pm->Images[0]->Height>r) r=(UWORD)pm->Images[0]->Height+1; }
                if(pm->Images[1]) { if(pm->Images[1]->Height>r) r=(UWORD)pm->Images[1]->Height+1; }

                if(pm->Sub) {
			UBYTE x=PM_Image_Height(a->p, PMIMG_SUBMENU, pm);
			if(x>r) r=x;
                } else if(pm->Flags&NPM_CHECKIT) {
			UBYTE x=PM_Image_Height(a->p, pm->Exclude?PMIMG_EXCLUDE:PMIMG_CHECKMARK, pm);
			if(x>r) r=x;
                }
		if(pm->CommKey!=0) {
			UBYTE x=PM_Image_Height(a->p, PMIMG_AMIGAKEY, pm);
			if(x>r) r=x;
		}

		r+=(UWORD)PM_Prefs->pmp_YSpace*2; // *2
	}

        return r;
}

UWORD PM_ItemWidth(struct PM_Window *a, struct PopupMenu *pm)
{
        UWORD tmp=0,img=0,icn=0,fontx;
	struct RastPort tmprp;

        fontx=a->p->MenuFont->tf_XSize;
        CopyMem(a->p->RootWnd->RPort, &tmprp, sizeof(struct RastPort));
        tmprp.Font=a->p->MenuFont;

        if(pm->Title) {
        if(GET_TXTMODE(pm)==NPX_TXTBOOPSI) {
        } else {
            STRPTR title=pm->Title;

            if(GET_TXTMODE(pm)==NPX_TXTLOCALE) title=(STRPTR)CallHook(a->p->LocaleHook, (Object *)pm, pm->TitleID, 123);

            if(title) tmp=TextLength(&tmprp,title,strlen(title));
        }
    }

        if(pm->Flags&NPM_CHECKIT) {
		tmp+=PM_Image_Width(a->p, pm->Exclude?PMIMG_EXCLUDE:PMIMG_CHECKMARK, pm);
		tmp+=PM_Prefs->pmp_Intermediate;
        } else if(pm->Sub && (a->p->PullDown==0) && (pm->Layout==0)) {
		tmp+=PM_Image_Width(a->p, PMIMG_SUBMENU, pm);
		tmp+=PM_Prefs->pmp_Intermediate;
        }

        if(pm->CommKey!=0) {
		tmp+=PM_Image_Width(a->p, PMIMG_AMIGAKEY, pm);
		tmp+=PM_Prefs->pmp_Intermediate+fontx*4;
        }

    if(pm->Flags&NPM_ISIMAGE) {
            if(pm->Images[0]) img=pm->Images[0]->Width;
            if(pm->Images[1]) if(pm->Images[1]->Width>img) img=pm->Images[1]->Width;

            if(img>tmp) tmp=img;
    } else {
        struct Image *img=pm->Images[0];

        // Hitta den största

        if(!img) { img=pm->Images[1]; }
        else if(pm->Images[1]) {
            if(img->Width<pm->Images[1]->Width) img=pm->Images[1];
        }
        if(img) {
                    tmp+=PM_Prefs->pmp_Intermediate;

                    icn=img->Width+PM_Prefs->pmp_Intermediate;

                if(icn>a->IconColumn) a->IconColumn=icn;
            }
    }

    if(pm->Flags&NPM_COLOURBOX) {
        if(!tmp) {  // If there's no other stuff in the item, no intermediate spacing is req'd
            tmp+=fontx*3;
            tmp-=PM_Prefs->pmp_XSpace+2;    // Must be like this for some unknow reason...
        } else {
            tmp+=PM_Prefs->pmp_Intermediate+fontx*3;
        }
    }

    tmp+=PM_Prefs->pmp_XSpace*2;    // "Horizontal spacing"

    tmp+=2;             // Item border

        return (UWORD)(tmp);
}


void PM_CalcItemSize(struct PM_Window *a, struct PopupMenu *pm)
{
    UWORD tmpw, tmph;
    struct PopupMenu *p;

    pm->Width=0;
    pm->Height=0;

    if(pm->Flags & NPM_HIDDEN) {
        return;
    }

    if(pm->Flags & NPM_GROUP) {
        //
        // Calculate minimum size for each item.
        // Calculate total size for the group.
        //

        p=pm->Sub;
        tmpw=1;
        tmph=1;
        if(p) do {
            PM_CalcItemSize(a, p);

            if(!(p->Flags & NPM_HIDDEN)) {
                if(pm->Layout==PML_Vertical) {
                    tmph+=p->Height;
                    if(p->Width>tmpw) tmpw=p->Width;
                }

                if(pm->Layout==PML_Horizontal) {
                    tmpw+=p->Width;
                    if(p->Height>tmph) tmph=p->Height;
                }
            
            }

            p=p->Next;
        } while(p);
    } else {
	struct PopupMenu *tmppm;

        BOOL patched=FALSE;

        tmppm=pm;

        if(pm->Next) {
            if((tmppm->Flags&NPM_NOSELECT) && (tmppm->Flags&NPM_SHADOWED) && !(tmppm->Flags&NPM_DISABLED) && (tmppm->Next->Flags&NPM_WIDE_BAR)) {
                //PATCH(TP_CENTER, NPM_CENTERED);

                PATCH(PMP_TITLE_UNDERLINE, NPM_UNDERLINEDTEXT);
                PATCH(PMP_TITLE_BOLD, NPM_BOLDTEXT);
                PATCH(PMP_TITLE_SHADOW, NPM_SHADOWED);
                PATCH(PMP_TITLE_EMBOSS, NPM_EMBOSSED);
                PATCH(PMP_TITLE_OUTLINE, NPM_OUTLINED);

                //PATCH(TP_SHINE, NPM_SHINETEXT);
                //PATCH(TP_SHADOW, NPM_SHADOWTEXT);
                //PATCH(TP_HILITE, NPM_HILITETEXT);
                    
                patched=TRUE;
            }
	}
        

        if(!patched) {
            //TPATCH(TP_CENTER, NPM_CENTERED);

            TPATCH(PMP_TEXT_UNDERLINE, NPM_UNDERLINEDTEXT);
            TPATCH(PMP_TEXT_BOLD, NPM_BOLDTEXT);
            TPATCH(PMP_TEXT_SHADOW, NPM_SHADOWED);
            TPATCH(PMP_TEXT_EMBOSS, NPM_EMBOSSED);
            TPATCH(PMP_TEXT_OUTLINE, NPM_OUTLINED);

            //TPATCH(TP_SHINE, NPM_SHINETEXT);
            //TPATCH(TP_SHADOW, NPM_SHADOWTEXT);
            //TPATCH(TP_HILITE, NPM_HILITETEXT);
        }
	
        tmph=PM_ItemHeight(a, pm);
        tmpw=PM_ItemWidth(a, pm);
    }

    pm->Height=tmph;
    pm->Width=tmpw;
}

void PM_LayoutGroup(struct PM_Window *a, struct PopupMenu *pm)
{
    UWORD tw, t, l, tsz, tmp;
    struct PopupMenu *p;
    BOOL restart;

    // Is this a group? If not, return.

    if(!(pm->Flags & NPM_GROUP))
        return;

    // Set up group offset

    l=pm->Left;
    t=pm->Top;

    switch(pm->Layout) {
        case PML_Vertical:
            //
            // Step 1. Calculate total weight.
            // Hidden items and fixed size items are to be excluded from
            // the size distribution, so they must be excluded from weight
            // calculation aswell.
            //
            tw=0;
            p=pm->Sub;
            while(p) {
                p->Flags&=~NPM_MINSIZE;
                if(!(p->Flags & NPM_HIDDEN) && !(p->Flags&NPM_FIXEDSIZE)) {
                    tw++;
                }
                p=p->Next;
            }

            //kprintf("\n\nLAYOUT: Start. Maximum vertical size %ld\n", pm->Height);

            //kprintf("LAYOUT: Initial vertical weight %ld\n", tw);

            //
            // Step 2. Find the items whose size is larger than what they
            // would normally get if the available size was distributed
            // equally.
            //

            do {
                restart=FALSE;
                
                // First we have to exclude fixed size items, and items that
                // require more space than what equal distribution would
                // result in.
                tsz=pm->Height;
                p=pm->Sub;
                while(p) {
                    if(!(p->Flags & NPM_HIDDEN) && (p->Flags&NPM_FIXEDSIZE || p->Flags&NPM_MINSIZE))
                        tsz-=p->Height;
                    p=p->Next;
                }

                // Then we scan through the other items for those who require
                // more space, and mark them with NPM_MINSIZE for exclusion
                // in the loop above.
                // If one, or more such items are found, we must repeat these
                // steps once more, until there are no more such items left.
                // (Until all remaining items can share the available space
                // equally)
                p=pm->Sub;
                while(p) {
                    if(!(p->Flags & NPM_HIDDEN)) {
                        if(!(p->Flags & NPM_FIXEDSIZE) && !(p->Flags & NPM_MINSIZE)) {
                            if(tw) tmp=tsz/tw;
                            else tmp=0;

                            if(tmp<p->Height) {
                                p->Flags|=NPM_MINSIZE;
                                //kprintf("LAYOUT: Removed item %s (%ld > %ld)\n", p->Title, p->Height, tmp);
                                tw--;
                                restart=TRUE;
                                //break;    // optimization, will require less looping
                            }
                        }
                    }

                    p=p->Next;
                }
            } while(restart);

            //kprintf("LAYOUT: Remaining vertical size %ld\n", tsz);
            //kprintf("LAYOUT: Remaining vertical weight %ld\n", tw);

            //
            // Step 3. Distribute the remaining size among the remaining items
            // and position all items.
            //

            p=pm->Sub;
            while(p) {

                p->Width=pm->Width;

                if(!(p->Flags & NPM_HIDDEN) && !(p->Flags&NPM_FIXEDSIZE) && !(p->Flags&NPM_MINSIZE)) {
                    if(tw) {
                        p->Height=tsz/tw;   // Assign height

                        tsz-=p->Height;     // This will avoid leaving an empty space at the end of a group,
                        tw--;           // due to precision limitations when dividing integers.
                    }
                }

                p->Left=l;  // Set item's
                p->Top=t;   // position

                PM_LayoutGroup(a, p);   // If this item is a group, layout will be handled here.

                t+=p->Height;
                p=p->Next;
            }
            break;
        case PML_Horizontal:
            //
            // See PML_Vertical for explanations.
            //
            tw=0;
            p=pm->Sub;
            while(p) {
                p->Flags&=~NPM_MINSIZE;
                if(!(p->Flags & NPM_HIDDEN) && !(p->Flags&NPM_FIXEDSIZE)) {
                    tw++;
                }
                p=p->Next;
            }

            do {
                restart=FALSE;
                
                tsz=pm->Width;
                p=pm->Sub;
                while(p) {
                    if(!(p->Flags & NPM_HIDDEN) && (p->Flags&NPM_FIXEDSIZE || p->Flags&NPM_MINSIZE))
                        tsz-=p->Width;
                    p=p->Next;
                }

                p=pm->Sub;
                while(p) {
                    if(!(p->Flags & NPM_HIDDEN)) {
                        if(!(p->Flags & NPM_FIXEDSIZE) && !(p->Flags & NPM_MINSIZE)) {
                            if(tw) tmp=tsz/tw;
                            else tmp=0;

                            if(tmp<p->Width) {
                                p->Flags|=NPM_MINSIZE;
                                tw--;
                                restart=TRUE;
                            }
                        }
                    }

                    p=p->Next;
                }
            } while(restart);

            p=pm->Sub;
            while(p) {

                p->Height=pm->Height;

                if(!(p->Flags & NPM_HIDDEN) && !(p->Flags&NPM_FIXEDSIZE) && !(p->Flags&NPM_MINSIZE)) {
                    if(tw) {
                        p->Width=tsz/tw;    // Assign height

                        tsz-=p->Width;      // This will avoid leaving an empty space at the end of a group,
                        tw--;           // due to precision limitations when dividing integers.
                    }
                }

                p->Left=l;  // Set item's
                p->Top=t;   // position

                PM_LayoutGroup(a, p);   // If this item is a group, layout will be handled here.

                l+=p->Width;
                p=p->Next;
            }
            break;
        case PML_Table:
            break;
    }
}

void PM_LayoutMenu(struct PM_Window *a)
{
    BOOL wb=PM_Prefs->pmp_PulldownPos==PMP_PD_WINDOWBAR;


    // Calculate Item and Group sizes

    PM_CalcItemSize(a, &a->PM);
    a->PM.Width+=a->IconColumn;

    // Set offsets

    a->PM.Left=PM_Prefs->pmp_XOffset+a->p->BorderWidth;
        a->PM.Top=PM_Prefs->pmp_YOffset+a->p->BorderHeight;

    // Adjust for pulldowns

    if(a->p->PullDown) {

        if(a->p->RootWnd->BorderTop<10) wb=FALSE;

        a->PM.Left=1;
            a->PM.Top=1;

        if(wb) {
            a->PM.Height=a->p->RootWnd->BorderTop - 3;
        }

        if(!wb) {
                a->PM.Height=a->p->RootWnd->WScreen->BarHeight - 2;
        }
    }

    // Layout groups

    PM_LayoutGroup(a, &a->PM);

    // Set size of window

    a->Height=a->PM.Height + (PM_Prefs->pmp_YOffset+a->p->BorderHeight)*2+1;
    a->Width=a->PM.Width + (PM_Prefs->pmp_XOffset+a->p->BorderWidth)*2+1;

    // Adjust for pulldowns

    if(a->p->PullDown) {

        if(wb) {
            a->Height=a->p->RootWnd->BorderTop;
        }

        if(!wb) {
                a->Height=a->p->RootWnd->WScreen->BarHeight+1;
        }

            if(wb) {
                a->Width+=2;        // 2 * borderwidth
            if(a->Width<a->p->RootWnd->Width-1) {
                a->Width=a->p->RootWnd->Width-1;
            }
        } else {
                a->Width=a->p->RootWnd->WScreen->Width-1;
        }
    }
}

