//
// pm.c - popupmenu.library
//
// popupmenu.library and the PopupMenu package is
// Copyright ©1996 - 2002 Henrik Isaksson
// All Rights Reserved.
//

#include "pmpriv.h"

#include "pminput.h"

#ifndef __AROS__
#include "newgui.h"
#endif

// struct PM_Root	*p = NULL;

// PM_HandleShadow
//
// Interface to the shadow handling algortihms.
// This will add the current menu rect to the menu topology map, that
// keeps track of the depth arrengement of the menus. This way shadows
// can be mapped to the height differences of various parts of the menu
// structure so that the size of each shadow segment correlates to the
// distance between the background and the menu that cast the shadow.
// Each shadow rectangle computed, is added to a list of shadowed
// regions that will prevent shadows cast by submenus to overlap any of
// the shadows previoulsy drawn.
// When all this is done, the shadow is finally rendered by calls to
// PM_DrawShadow(), for each rectangular segment of the shadow cast by
// this menu.
//

void PM_HandleShadow(struct PM_Window *a)
{
    PMSR *worknode;
    PMSR *nextnode;
    PMSRList *delta;
    int depth=a->MenuLevel;
    int l=a->Wnd->LeftEdge, t=a->Wnd->TopEdge;
    int w=a->Width;
    int h=a->Height;

    if(!a->Shadowmap || !a->Topographic)
	return;	// Shadows not used

    if((delta=PM_InitShadowList())) {

	//
	// The three following calls will create shadows for a lightsource
	// positioned somewhere near the top left corner of the screen.
	//
	// +----------+
	// |          |--+
	// |          |  |
	// |          |1 |
	// |          |  |
	// |          |  |
	// +-+--------+--+
	//   |   2    |3 |
	//   +--------+--+
	//

	// Shadow 1
        PM_MapShadow(a->Topographic, delta,
            l+w, t, l+w, t+h,
            depth, PMSHADOW_HORIZ|PMSHADOW_TOP);

	// Shadow 2
        PM_MapShadow(a->Topographic, delta,
            l, t+h, l+w, t+h,
            depth, PMSHADOW_VERT|PMSHADOW_LEFT);

	// Shadow 3
        PM_MapShadow(a->Topographic, delta,
            l+w, t+h, l+w, t+h,
            depth, PMSHADOW_HORIZ|PMSHADOW_VERT);

	//
	// The list "delta" represents the additional shadows necessary to
	// achieve the desired result.
	//
	
        PM_AddShadow(a->Shadowmap, delta);

        PM_SubMenuRect(a->Shadowmap, l, t, w, h);

        PM_AddTopographicRegion(a->Topographic,
            l, t, l+w, h+t, depth+1);

        worknode = (PMSR *)(delta->mlh_Head);
        while((nextnode = (PMSR *)PM_NextNode(worknode))) {
            PM_DrawShadow(a,	  worknode->Left-a->Wnd->LeftEdge,
                                  worknode->Top-a->Wnd->TopEdge,
                                  worknode->Right-a->Wnd->LeftEdge-1,
                                  worknode->Bottom-a->Wnd->TopEdge-1);
            worknode=nextnode;
        }

        PM_FreeShadowList(delta);
    }
}

// PM_RenderMenu
void PM_RenderMenu(struct PM_Window *a, BOOL MenuDisable, BOOL refresh)
{
	if(a->te.RPort && !refresh) {
		/* Off screen buffer present - draw into it instead */
		/* (Unless it's a menu refresh) */
		a->RPort = a->te.RPort;
	}

	SetDrMd(a->RPort, JAM1);


	// Clear the background

	PM_DrawBg(a, 0, 0, a->Width-1, a->Height-1);

	// Draw a frame around the menu

	if(a->p->PullDown && a->FirstTime)
		PM_DrawBox(a, 0, 0, a->Width-1, a->Height-1, SHINE(a->p), SHADOW(a->p));
	else
		PM_DrawPrefBox(a->p, a, 0, 0, a->Width-1, a->Height-1);

/*******************************************************

	{
		struct Hook *menurenderhook = NULL;
		GetGUIAttrs(NULL, a->p->DrawInfo, GUIA_MenuRenderHook, &menurenderhook, TAG_DONE); // V50
		if(menurenderhook) {
			struct MenuRenderMsg rendermsg;
			rendermsg.mrm_MethodID	= MR_MENUPANEL;
			rendermsg.mrm_RastPort	= a->RPort;
			rendermsg.mrm_DrawInfo	= a->p->DrawInfo;
			rendermsg.mrm_Bounds	= ;
			rendermsg.mrm_State	= 0;
			rendermsg.mrm_Window	= a->Wnd;
			rendermsg.mrm_Flags	= MRF_POPUP; // | MRF_TRANSPARENT
			CallHook(menurenderhook, (Object *)&rendermsg);
		} else {
		}		
	}
*******************************************************/


	// Set Menu Font

	SetFont(a->RPort, a->p->MenuFont);

	// Draw the items

	PM_NewDrawItem(a, &a->PM, FALSE, MenuDisable);

	// Dither the menu, if disabled and if Old Style is selected

	if(MenuDisable && PM_Prefs->pmp_SeparatorBar)
		PM_Ghost(a, 0, 0, a->Width, a->Height, SHADOW(a->p));

	/* Back to on-screen rendering */
	a->RPort = a->Wnd->RPort;
}

void SelectItem(struct PM_Window *c, BOOL mdis)
{
    if(!(c->Selected->Flags & NPM_DISABLED) && !mdis) {
        if(c->Selected->Flags & NPM_CHECKIT) {
            if(c->Selected->Flags & NPM_CHECKED) {
                if(!(c->Selected->Flags & NPM_NOTOGGLE)) {
                    if(c->Selected->Exclude) {
                        PM_AlterState(c->p->PM, c->Selected->Exclude, PMACT_DESELECT);
                    }
                    c->Selected->Flags&=~NPM_CHECKED;
                    c->Selected->Flags|=NPM_ISSELECTED;
                    if(c->Selected->AutoSetPtr) *c->Selected->AutoSetPtr=FALSE;
                    if(c->Selected->Exclude) {
                        PM_RenderMenu(c, FALSE, TRUE);
                    } else {
                        PM_NewDrawItem(c, c->Selected, TRUE, FALSE);
                    }
                }
            } else {
                if(c->Selected->Exclude) {
                    PM_AlterState(c->p->PM, c->Selected->Exclude, PMACT_SELECT);
                }
                c->Selected->Flags|=NPM_CHECKED|NPM_ISSELECTED;
                if(c->Selected->AutoSetPtr) *c->Selected->AutoSetPtr=TRUE;
                if(c->Selected->Exclude) {
                    PM_RenderMenu(c, FALSE, TRUE);
                } else {
                    PM_NewDrawItem(c, c->Selected, TRUE, FALSE);
                }
            }
        } else {
            if(c->Selected->Flags & NPM_ISSELECTED) c->Selected->Flags&=~NPM_ISSELECTED;
            else c->Selected->Flags|=NPM_ISSELECTED;
            PM_NewDrawItem(c, c->Selected, TRUE, FALSE);
        }

        c->p->ReturnCode=c->Selected->UserData;
        c->p->ReturnID=c->Selected->ID;
    }
}

//
// PM_MBHit
//
// Called when a mouse button changes state.
//
BOOL PM_MBHit(struct PM_Window *c, struct PM_InpMsg *msg, BOOL mdis)
{
    UBYTE   whattodo=0;

    if(PM_Prefs->pmp_Sticky) {
        if(((msg->Code&IECODE_LBUTTON) || (msg->Code&IECODE_RBUTTON))) {
            if((msg->Code&IECODE_UP_PREFIX) &&
                ((msg->Qual & (IEQUALIFIER_LSHIFT|IEQUALIFIER_RSHIFT)) ||
                (msg->Qual & (IEQUALIFIER_LEFTBUTTON|IEQUALIFIER_RBUTTON)))) {
                    if(c->p->DoMultiSel) {
                        whattodo=3;
                        c->p->DoneMulti=TRUE;
                    } else {
                        return FALSE;
                    }
            } else {
                if(!(msg->Code&IECODE_UP_PREFIX)) {
                    c->StickyFlag=TRUE;
                    return FALSE;
                } else {
                    if(c->StickyFlag) {
                        if(!c->p->DoneMulti) {
                            whattodo=1;
                        } else {
                            if(c->Prev) c->Prev->Running=0L;
                            c->Running=0L;
                            return TRUE;
                        }
                    }
                }
            }
        } else {
            return FALSE;
        }
    } else {
        if(((msg->Code&IECODE_LBUTTON) || (msg->Code&IECODE_RBUTTON))) {
            if((msg->Code&IECODE_UP_PREFIX) &&
                ((msg->Qual & (IEQUALIFIER_LSHIFT|IEQUALIFIER_RSHIFT)) ||
                (msg->Qual & (IEQUALIFIER_LEFTBUTTON|IEQUALIFIER_RBUTTON)))) {
                    if(c->p->DoMultiSel) {
                        whattodo=3;
                        c->p->DoneMulti=TRUE;
                    } else {
                        return FALSE;
                    }
            } else {
                if(msg->Code&IECODE_UP_PREFIX) {
                    if(!c->p->DoneMulti) {
                        whattodo=1;
                    } else {
                        if(c->Prev) c->Prev->Running=0L;
                        c->Running=0L;
                        return TRUE;
                    }
                }
            }
        } else return FALSE;
    }

    if(whattodo & 1) { // Select an item
        if(c->Selected) {
            SelectItem(c, mdis);
        }
    }

    if(whattodo==1) {   // Close if multiselect not requested
        if(c->Prev) c->Prev->Running=0L;
        c->Running=0L;
    }

    if(whattodo==0) return FALSE;
    else return TRUE;
}

// PM_InsideItemBox
struct PopupMenu *PM_InsideItemBox(struct PM_Window *c, struct PopupMenu *pm, ULONG mx, ULONG my)
{
	PopupMenu *tmppm;

    while(pm) {
        if(pm->Sub && (pm->Flags&NPM_GROUP)) {
            tmppm=PM_InsideItemBox(c, pm->Sub, mx, my);
            if(tmppm) return tmppm;
        } else if(!(pm->Flags&NPM_NOSELECT)) {
            if(c->p->PullDown && c->FirstTime) {
                if(my>=c->Wnd->TopEdge &&
                   my<=c->Wnd->TopEdge+pm->Top+pm->Height &&
                   mx>=c->Wnd->LeftEdge+pm->Left &&
                   mx<=c->Wnd->LeftEdge+pm->Left+pm->Width) {
                    return pm;
                }
            } else {
                if(my>=c->Wnd->TopEdge+pm->Top &&
                   my<=c->Wnd->TopEdge+pm->Top+pm->Height &&
                   mx>=c->Wnd->LeftEdge+pm->Left &&
                   mx<=c->Wnd->LeftEdge+pm->Left+pm->Width) {
                    return pm;
                }
            }
        }
        pm=pm->Next;
    }

    return NULL;
}

//
// PM_RedrawPrevSel
//
// Redraws the previously selected item.
//
void PM_RedrawPrevSel(struct PM_Window *c)
{
        if(c->PrevSel) {
                PM_NewDrawItem(c, c->PrevSel, FALSE, FALSE);
        }
}

//
// PM_MMove
//
// Called on mouse moves
//
BOOL PM_MMove(struct PM_Window *c, struct PM_InpMsg *msg,
        ULONG wleft, ULONG wtop, ULONG mx, ULONG my, BOOL MenuDisable)
{
        if(c->Running && my>=c->Wnd->TopEdge && my<=c->Wnd->TopEdge+c->Height && mx>=c->Wnd->LeftEdge && mx<=c->Wnd->LeftEdge+c->Width) {
                c->PrevSel=c->Selected;

	        c->Selected=PM_InsideItemBox(c, &c->PM, mx, my);

        	if((c->PrevSel!=c->Selected)) {
                	c->p->Subtimer=0;
	                if(c->PrevSel) {
        	                if(!MenuDisable) PM_RedrawPrevSel(c);
                	        c->PrevSel=0L;
	                }
        	        if(c->Selected) {
                	        if(!MenuDisable) PM_NewDrawItem(c, c->Selected, TRUE, MenuDisable);
	                }
		}
        } else {
                if(c->Selected) {
                        if(!MenuDisable) PM_NewDrawItem(c, c->Selected, FALSE, MenuDisable);
                        c->Selected=0L;
                }
                return FALSE;
        }
        return TRUE;
}

//
// PM_OpenPopupWindow
//
// 1) Opens a window at the appropriate position
// 2) If the window has been opened before, it will be resized to fit
//    changes in menu size
// 3) Renders the menu.
// 4) Performs transition effects (TE).
// 5) Renders shadows.
//
BOOL PM_OpenPopupWindow(struct PM_Window *a)
{
	ULONG	shadows = TRUE;

#ifndef __AROS__
	if( IntuitionBase->LibNode.lib_Version >= 50 ) {
		GetGUIAttrs(NULL, a->p->DrawInfo, GUIA_MenuDropShadows, &shadows, TAG_DONE); // V50 !!
	}
#endif

	PM_LayoutMenu(a);

	if(!shadows) {
		a->p->ShadowHeight=0;
	        a->p->ShadowWidth=0;
	}

	if(a->FirstTime) {
        	// The first menu may be affected by some special parameters
		// set through PM_OpenPopupMenu()
        	if(a->p->MenuWidth>a->Width) a->Width=a->p->MenuWidth;
	        if(a->p->MenuHeight>a->Height) a->Height=a->p->MenuHeight;
        	if(a->p->MenuRight) a->Wnd->MouseX=a->p->MenuRight-a->Width;
	        if(a->p->MenuBottom) a->Wnd->MouseY=a->p->MenuBottom-a->Height;
        	if(a->p->MenuCenter) {
	        	a->Wnd->MouseX=(a->p->RootWnd->WScreen->Width/2)-a->Width/2;
        		a->Wnd->MouseY=(a->p->RootWnd->WScreen->Height/2)-a->Height/2;
	        }
	}

	if(a->FirstTime && a->p->PullDown) {
		// A pulldown menu will be horizontally laid out
        	if(!a->Wnd)
	            PM_OpenWindow(a, a->MenuX, a->MenuY, a->Width+1, a->Height+1, a->p->RootWnd->WScreen);
        	else
	            PM_ResizeWindow(a, a->MenuX, a->MenuY, a->Width+1, a->Height+1);
	} else {
	        if(!a->Wnd) {
			WORD mx, my;

			mx = a->MenuX;
			my = a->MenuY;

	            if(mx+a->Width > a->p->RootWnd->WScreen->Width) {
			// If we're too close to the right edge, open menus to the left hereafter.
                	a->ReverseDirection=TRUE;
	            }
        	    if(a->AltXPos-a->Width < 0) {
			// If we're too close to the left edge, open menus to the right (default).
	                a->ReverseDirection=FALSE;
        	    }

	            if(!a->ReverseDirection) {
        	        PM_OpenWindow(a, mx, my, a->Width+(a->p->ShadowWidth+a->MenuLevel*2)+1, a->Height+(a->p->ShadowHeight+a->MenuLevel*2)+1, a->p->RootWnd->WScreen);
	            } else {
        	        PM_OpenWindow(a, a->AltXPos-a->Width, my, a->Width+(a->p->ShadowWidth+a->MenuLevel*2)+1, a->Height+(a->p->ShadowHeight+a->MenuLevel*2)+1, a->p->RootWnd->WScreen);
	            }
        	} else {
	            PM_ResizeWindow(a, SCREENMOUSEPOS(a->p), a->Width+1, a->Height+1);
        	}
	}

	if(a->Wnd) {
	        PM_RenderMenu(a, a->MenuDisabled, FALSE);

		if(a->te.BMap) {
			// If there is a TE (transition effects) bitmap.

			// Currently, only an "animation" effect is implemented.

			int i;
			int w, h;
			int dw, dh;

			w = a->Wnd->Width-1;
			h = a->Wnd->Height-1;

			if(!(a->p->PullDown && a->FirstTime)) {
				w-=a->p->ShadowWidth+a->MenuLevel*2;
				h-=a->p->ShadowHeight+a->MenuLevel*2;
			}

			dw = w/10;
			dh = h/10;

			for(i=1;i<11;i++) {
				//Forbid();
				BltBitMap(a->te.BMap, 0, 0, a->Wnd->WScreen->RastPort.BitMap, a->Wnd->LeftEdge, a->Wnd->TopEdge, dw*i, dh*i, 0xc0, 0xff, 0L);
				WaitBlit();
				//Permit();
				WaitTOF();
			}

			//Forbid();
			BltBitMap(a->te.BMap, 0, 0, a->Wnd->WScreen->RastPort.BitMap, a->Wnd->LeftEdge, a->Wnd->TopEdge, w, h, 0xc0, 0xff, 0L);
			WaitBlit();
			//Permit();
		}

		// Handle shadows
		if(shadows) {
			if(!(a->p->PullDown && a->FirstTime)) {
				PM_HandleShadow(a);
			}
		}

        	return TRUE;
	}
	return FALSE;
}

struct PM_Window *PM_SetupSubWindow(struct PM_Window *parent, struct PM_Root *p, struct PopupMenu *pm)
{
	struct PM_Window *newwin;

	newwin=PM_Mem_Alloc(sizeof(struct PM_Window));
	if(newwin) {
		if(parent) {
			parent->WasSelected = parent->Selected;
			newwin->ReverseDirection = parent->ReverseDirection;
			newwin->Topographic = PM_CopyList(parent->Topographic);
			newwin->Shadowmap = PM_CopyList(parent->Shadowmap);

			if((parent->Selected->Flags & NPM_DISABLED) || parent->MenuDisabled)
				newwin->MenuDisabled = TRUE;
		} else {
			newwin->ReverseDirection = 0;
			if(PM_Prefs->pmp_Flags&1) {
				newwin->Topographic = PM_InitTopographicList();
				newwin->Shadowmap = PM_InitShadowList();
				if(newwin->Topographic)
					PM_AddTopographicRegion(newwin->Topographic, 0, 0, 5000, 5000, 0);
			}
		}

		newwin->PM.Sub = pm;
		newwin->PM.Flags = NPM_NOSELECT|NPM_GROUP;
		newwin->PM.Layout = PML_Vertical;
		newwin->PM.Image = NULL;

		newwin->Running = TRUE;
		newwin->Prev = parent;
		newwin->p = p;

		if(parent) {
			if(p->PullDown && parent->FirstTime) {
				newwin->MenuX = parent->WasSelected->Left + parent->Wnd->LeftEdge - 1;
				newwin->MenuY = parent->Height + 1 + parent->Wnd->TopEdge - 1;
				newwin->AltXPos = parent->WasSelected->Left + parent ->Wnd->LeftEdge;
			} else {
				newwin->MenuX = (int)(parent->WasSelected->Width - parent->WasSelected->Width / 3) + parent->Wnd->LeftEdge + parent->WasSelected->Left;

				if(newwin->MenuX & 1)		// Prevents interference in dithered shadows
					newwin->MenuX += 1;

				newwin->MenuY = YAPosSelBar(parent, parent->WasSelected) - 5 + parent->Wnd->TopEdge;
				newwin->AltXPos = (int)(parent->WasSelected->Width / 3) + parent->Wnd->LeftEdge + parent->WasSelected->Left;
			}

			newwin->MenuLevel = parent->MenuLevel + 1;
			if(newwin->MenuLevel>4) newwin->MenuLevel = 4;

			if(parent->SubMenuParent->SubConstruct) {
				newwin->PM.Sub = (struct PopupMenu *)CallHook(parent->SubMenuParent->SubConstruct, (Object *)parent->Selected, parent);
			}
		} else {
			newwin->AltXPos = newwin->MenuX = p->Scr->MouseX;
			newwin->MenuY = p->Scr->MouseY;
			newwin->MenuLevel = 0;
			newwin->FirstTime = TRUE;
			return newwin;
		}

		if(newwin->PM.Sub)
			return newwin;

		// Constructor hook cancelled the operation, we must tidy up a bit...

		PM_FreeSubWindow(parent, newwin);
	}
	return NULL;
}

void PM_FreeSubWindow(struct PM_Window *parent, struct PM_Window *window)
{
	if(window) {
		if(parent) {
			if(parent->SubMenuParent->SubDestruct) {
				CallHook(parent->SubMenuParent->SubDestruct, (Object *)parent->SubMenuParent);
			}
		}

		if(window->Topographic)
			PM_FreeTopographicList(window->Topographic);
		if(window->Shadowmap)
			PM_FreeShadowList(window->Shadowmap);

		PM_CloseWindow(window);
		PM_Mem_Free(window);
	}
	if(parent)
		parent->SubMenuToOpen = 0L;
}

void PM_SelectNext(struct PM_Window *a)
{
        BOOL found=FALSE;
        a->Selected=PM_FindNextSelectable(a, a->PM.Sub, &found);
        if(a->Selected==NULL) a->Selected=PM_FindFirstSelectable(a->PM.Sub);
}

void PM_SelectPrev(struct PM_Window *a)
{
        BOOL found=FALSE;
        a->Selected=PM_FindPrevSelectable(a, a->PM.Sub, &found);
        if(!a->Selected)
                a->Selected=PM_FindLastSelectable(a->PM.Sub);
}

APTR PM_DoPopup(struct PM_Window *a)
{
    if(PM_OpenPopupWindow(a)) {
        BOOL halt;
        BOOL keymode=FALSE;

        while(a->Running && (a->p->TimeOut<2)) {
            struct PM_InpMsg *msg;
            ULONG s;

            s=Wait(1L << a->p->pmh->port->mp_SigBit | 1L << a->p->tport->mp_SigBit);

            if(s & (1L << a->p->tport->mp_SigBit)) {
                if(GetMsg(a->p->tport)) {
                    a->p->treq->tr_node.io_Command = TR_ADDREQUEST;
                    a->p->treq->tr_time.tv_secs=0;
                    a->p->treq->tr_time.tv_micro=200000;
                    SendIO((struct IORequest *)a->p->treq);
                    a->p->TimeOut++;
                }
            }

            halt=FALSE;

            if(s & (1L << a->p->pmh->port->mp_SigBit)) {
                while((msg=(struct PM_InpMsg *)GetMsg(a->p->pmh->port))) {
                    switch(msg->Kind) {
                        case PM_MSG_RAWMOUSE:
                            keymode=FALSE;
                            if(msg->Code!=IECODE_NOBUTTON) {
                                PM_MBHit(a, msg, a->MenuDisabled);
                                break;
                            } else {
                                if(!PM_MMove(a, msg, 0, 0, a->Wnd->WScreen->MouseX, a->Wnd->WScreen->MouseY, a->MenuDisabled)) {
                                    if(PM_InsideWindows(a->Wnd->WScreen->MouseX, a->Wnd->WScreen->MouseY, a)) {
                                        a->Running=0L;
                                        if(a->Prev) a->Prev->Running=TRUE;
                                    }
                                }
                            }

                        case PM_MSG_TIMER:
                            if(a->p->TimeOut>0) a->p->TimeOut--;
                            if(msg->Kind==PM_MSG_TIMER || a->p->Subtimer==0) {
                                a->p->Subtimer++;
                                if(a->Selected && (a->p->Subtimer > PM_Prefs->pmp_SubMenuDelay) && !keymode) {
                                    if(a->Selected->Sub || a->Selected->SubConstruct) {
                                        a->SubMenuToOpen=a->Selected->Sub;
                                        a->SubMenuParent=a->Selected;

                                        halt=TRUE;  // stop before we get too many timer msgs
                                    }
                                }
                            }
                            break;

                        case PM_MSG_DOWN:
                            keymode=TRUE;
                            a->PrevSel=a->Selected;
                            PM_RedrawPrevSel(a);
                            PM_SelectNext(a);
                            if(a->Selected) PM_NewDrawItem(a, a->Selected, TRUE, FALSE);
                            break;

                        case PM_MSG_UP:
                            keymode=TRUE;
                            a->PrevSel=a->Selected;
                            PM_RedrawPrevSel(a);
                            PM_SelectPrev(a);
                            if(a->Selected) PM_NewDrawItem(a, a->Selected, TRUE, FALSE);
                            break;

                        case PM_MSG_MULTISELECT:
                        case PM_MSG_SELECT:
                            if(a->Selected) {
                                if(!a->Selected->Sub && !a->Selected->SubConstruct) {
                                    SelectItem(a, FALSE);
                                    if(msg->Kind==PM_MSG_SELECT) {
                                        a->Running=0;
                                        if(a->Prev) a->Prev->Running=FALSE;
                                    }
                                }
                            }
                        case PM_MSG_OPENSUB:
                            if(a->Selected) {
                                if(a->Selected->Sub || a->Selected->SubConstruct) {
                                    a->SubMenuToOpen=a->Selected->Sub;
                                    a->SubMenuParent=a->Selected;

                                    halt=TRUE;  // stop before we get too many timer msgs
                                }
                            }
                            break;

                        case PM_MSG_CLOSESUB:
                            a->Running=0;
                            if(a->Prev) a->Prev->Running=TRUE;
                            halt=TRUE;
                            break;

                        case PM_MSG_TERMINATE:
                            a->Running=0L;
                            halt=TRUE;
                            break;
                    }
                    PM_Mem_Free(msg);

                    if(halt) break;
                }
            }

            if(a->SubMenuToOpen) {
                struct Task *tsk;
                tsk=FindTask(NULL);
                if((UBYTE *)tsk->tc_SPReg<((UBYTE *)tsk->tc_SPLower)+1024) {
                    DisplayBeep(NULL);
	    #if 0
                } else if((a->NextWindow = PM_SetupSubWindow(a, p, a->SubMenuToOpen))) {
	    #else
	    	#warning "CHECKME: trying to avoid global p"
                } else if((a->NextWindow = PM_SetupSubWindow(a, a->p, a->SubMenuToOpen))) {
	    #endif
                    PM_DoPopup(a->NextWindow);

                    if(a->Prev) a->Prev->Running = FALSE;

                    if(!keymode) {
                        if(!PM_MMove(a, 0, 0, 0, a->Wnd->WScreen->MouseX, a->Wnd->WScreen->MouseY, a->MenuDisabled)) {
                            if(PM_InsideWindows(a->Wnd->WScreen->MouseX, a->Wnd->WScreen->MouseY, a)) {
                                a->Running = 0L;
                                if(a->Prev) a->Prev->Running = TRUE;
                            }
                        }
                    }

                    PM_FreeSubWindow(a, a->NextWindow);
		    a->NextWindow = NULL;
                }
                a->SubMenuToOpen=NULL;
            }
        }
        PM_CloseWindow(a);
    }

    return 0L;
}

//
// Popup a "hint" window, and close it at first mouse move
//
APTR PM_DoHint(struct PM_Window *a)
{
    if(PM_OpenPopupWindow(a)) {
        while(a->Running && (a->p->TimeOut<2)) {
            struct PM_InpMsg *msg;
            ULONG s;

            s=Wait(1L << a->p->pmh->port->mp_SigBit | 1L << a->p->tport->mp_SigBit);

            if(s & (1L << a->p->tport->mp_SigBit)) {
                if(GetMsg(a->p->tport)) {
                    a->p->treq->tr_node.io_Command = TR_ADDREQUEST;
                    a->p->treq->tr_time.tv_secs=0;
                    a->p->treq->tr_time.tv_micro=200000;
                    SendIO((struct IORequest *)a->p->treq);
                    a->p->TimeOut++;
                }
            }

            if(s & (1L << a->p->pmh->port->mp_SigBit)) {
                while((msg=(struct PM_InpMsg *)GetMsg(a->p->pmh->port))) {
                    switch(msg->Kind) {
                        case PM_MSG_TIMER:
                            if(a->p->TimeOut>0) a->p->TimeOut--;
                            break;
                        default:
                            a->Running=0L;
                    }
                    PM_Mem_Free(msg);
                }
            }

        }
        PM_CloseWindow(a);
    }
    return 0L;
}

APTR __saveds ASM PM_OBSOLETEFilterIMsgA()
{
    return NULL;
}

//
// Filter the IntuiMessage structure for command key sequences, and
// mouse events for intuition menu replacement mode.
//
APTR __saveds ASM PM_FilterIMsgA(register __a0 struct Window *w GNUCREG(a0),
    register __a1 struct PopupMenu *pm GNUCREG(a1),
    register __a2 struct IntuiMessage *im GNUCREG(a2),
    register __a3 struct TagItem *tags GNUCREG(a3))
{
        struct TagItem          *tstate;
        struct TagItem          *tag;
        struct Hook             *MenuHandler=NULL;
        BOOL                    autpd = FALSE, rawkey = FALSE;

	if(!pm) return 0L;
	if(!im) return 0L;

        tstate = tags;
        while((tag=NextTagItem(&tstate))) {
                switch(tag->ti_Tag) {
                        case PM_MenuHandler:
                                MenuHandler=(struct Hook *)tag->ti_Data;
                                break;
                        case PM_AutoPullDown:
                                autpd=TRUE;
                                break;
			case PM_RawKey:
				rawkey = (BOOL)tag->ti_Data;
				break;
                }
        }

        if(im->Class==IDCMP_MOUSEBUTTONS) {
                if(autpd) {
                        if(im->Code==MENUDOWN || im->Code==MENUUP) {
                                struct TagItem  opmtags[6];

                                opmtags[0].ti_Tag=PM_Menu;              opmtags[0].ti_Data=(ULONG)pm;
                                opmtags[1].ti_Tag=PM_Code;              opmtags[1].ti_Data=im->Code;
                                opmtags[2].ti_Tag=PM_PullDown;          opmtags[2].ti_Data=TRUE;
                                if(MenuHandler) {
                                        opmtags[3].ti_Tag=PM_MenuHandler;       opmtags[3].ti_Data=(ULONG)MenuHandler;
                                } else {
                                        opmtags[3].ti_Tag=PM_Dummy;             opmtags[3].ti_Data=0;
                                }
                                opmtags[4].ti_Tag=TAG_MORE;             opmtags[4].ti_Data=(ULONG)tags;
                                opmtags[5].ti_Tag=TAG_DONE;             opmtags[5].ti_Data=0;

                                return PM_OpenPopupMenuA(w, opmtags);
                        }
                }
        } else if(im->Class==IDCMP_VANILLAKEY && !rawkey) {
                if(im->Qualifier&IEQUALIFIER_RCOMMAND) {
                        struct PopupMenu *p;

                        p=PM_FindItemCommKey(pm, im->Code);
                        if(p) {
                                if(!(p->Flags & NPM_DISABLED)) {
                                        if(p->Flags & NPM_CHECKIT) {
                                                if(p->Flags & NPM_CHECKED) {
                                                        p->Flags&=~NPM_CHECKED;
                                                        if(p->Exclude)
                                                                PM_AlterState(pm, p->Exclude, PMACT_DESELECT);
                                                        if(p->AutoSetPtr) *p->AutoSetPtr=FALSE;
                                                } else {
                                                        if(p->Exclude)
                                                                PM_AlterState(pm, p->Exclude, PMACT_SELECT);
                                                        p->Flags|=NPM_CHECKED;
                                                        if(p->AutoSetPtr) *p->AutoSetPtr=TRUE;
                                                }
                                        }
        	                        if(MenuHandler) CallHook(MenuHandler, (Object *)p);
	                                return p->UserData;
                                }
                        }
                }
        } else if(im->Class==IDCMP_RAWKEY && rawkey) {
                if(im->Qualifier&IEQUALIFIER_RCOMMAND) {
                        struct PopupMenu *p;

                        p=PM_FindItemCommKey(pm, im->Code);
                        if(p) {
                                if(!(p->Flags & NPM_DISABLED)) {
                                        if(p->Flags & NPM_CHECKIT) {
                                                if(p->Flags & NPM_CHECKED) {
                                                        p->Flags&=~NPM_CHECKED;
                                                        if(p->Exclude)
                                                                PM_AlterState(pm, p->Exclude, PMACT_DESELECT);
                                                        if(p->AutoSetPtr) *p->AutoSetPtr=FALSE;
                                                } else {
                                                        if(p->Exclude)
                                                                PM_AlterState(pm, p->Exclude, PMACT_SELECT);
                                                        p->Flags|=NPM_CHECKED;
                                                        if(p->AutoSetPtr) *p->AutoSetPtr=TRUE;
                                                }
                                        }
        	                        if(MenuHandler) CallHook(MenuHandler, (Object *)p);
	                                return p->UserData;
                                }
                        }
                }
	}
        return NULL;
}

//
// Call MenuHandler for each selected item
//
void PM_DoSelected(struct PM_Root *p, struct PopupMenu *pm)
{
        struct PopupMenu *z=pm;

        while(z) {
                if(z->Flags&NPM_ISSELECTED) {
                        if(p->DoMultiSel) CallHook(p->MenuHandler, (Object *)z, 0);
                        z->Flags&=~NPM_ISSELECTED;
                        if(z->Flags&NPM_CHECKED)
                            z->Flags|=NPM_INITIAL_CHECKED;
                        else
                            z->Flags&=~NPM_INITIAL_CHECKED;
                }
                if(z->Flags&NPM_CHECKED)
                        z->Flags|=NPM_INITIAL_CHECKED;
                else
                        z->Flags&=~NPM_INITIAL_CHECKED;

                if(z->Sub) PM_DoSelected(p, z->Sub);
                z=z->Next;
        }
}

/// PM_OpenPopupMenuA
struct Window FakeWnd;

APTR __saveds ASM PM_OpenPopupMenuA(register __a1 struct Window *prevwnd GNUCREG(a1),
    register __a2 struct TagItem *tags GNUCREG(a2))
{
        APTR			ret = 0L;
        BOOL			shut_down = FALSE;
        struct TagItem          *tstate;
        struct TagItem          *tag;
#if 1
#warning "trying to get rid of global p"
    	struct PM_Root	    	*p;
#endif
	
	if(!prevwnd) {
        	prevwnd=&FakeWnd;
        	FakeWnd.LeftEdge=0;
        	FakeWnd.TopEdge=0;
        	FakeWnd.WScreen=IntuitionBase->ActiveScreen;
		FakeWnd.RPort=&FakeWnd.WScreen->RastPort;
    	}

        p=PM_AllocPMRoot(prevwnd);
        if(p) {
                p->RootWnd = prevwnd;
		p->Scr = prevwnd->WScreen;

                p->pmh=PM_InstallHandler(127);

                if(p->pmh) {

			PM_Prefs_Load(PMP_PATH);

			p->RootMenu = PM_SetupSubWindow(NULL, p, NULL);
            		p->RButton=TRUE; // default

                        tstate = tags;
                        while((tag = NextTagItem(&tstate)) && !shut_down) {
                                switch(tag->ti_Tag) {
                                    case PM_ForceFont:
                                        p->MenuFont=(struct TextFont *)tag->ti_Data;
                                        break;
                                    case PM_LocaleHook:
					p->LocaleHook=(struct Hook *)tag->ti_Data;
					break;
                                    case PM_Code:
                                        if(tag->ti_Data & IECODE_RBUTTON) p->RButton=TRUE;
                                        if(tag->ti_Data & IECODE_LBUTTON) p->LButton=TRUE;
                                        if((tag->ti_Data & IECODE_UP_PREFIX)) {
                                        	shut_down=TRUE;
                                        }
                                        break;
                                    case PM_UseLMB:
                                        if(tag->ti_Data) p->LButton = TRUE;
                                        p->RButton = FALSE;
                                        break;
                                    case PM_CenterScreen:
                                        p->MenuCenter = tag->ti_Data;
                                        break;
                                    case PM_Right:
                                        p->MenuRight = tag->ti_Data + prevwnd->LeftEdge;
                                        break;
                                    case PM_Bottom:
                                        p->MenuBottom = tag->ti_Data + prevwnd->TopEdge;
                                        break;
                                    case PM_MinWidth:
                                        p->MenuWidth = tag->ti_Data;
                                        break;
                                    case PM_MinHeight:
                                        p->MenuHeight = tag->ti_Data;
                                        break;
                                    case PM_Left:
                                        p->RootMenu->MenuX = tag->ti_Data+(tag->ti_Data&1L?1:0)+prevwnd->LeftEdge;
                                        break;
                                    case PM_Top:
                                        p->RootMenu->MenuY = tag->ti_Data+prevwnd->TopEdge;
                                        break;
                                    case PM_Menu:
                                        if(tag->ti_Data) {
                                        	p->PM = (struct PopupMenu *)tag->ti_Data;
                                        }
                                        break;
                                    case PM_PullDown:
                                        if(PM_Prefs->pmp_PulldownPos == PMP_PD_MOUSE) { // Popup pulldowns?
                                            p->PullDown = tag->ti_Data;
                                        } /*else if(PM_Prefs->Popup == 2) { // pos dependent
                                            if(PM_Prefs->WinBar) {
                                                if(prevwnd->LeftEdge<prevwnd->WScreen->MouseX &&
                                                   prevwnd->TopEdge<prevwnd->WScreen->MouseY &&
                                                   prevwnd->LeftEdge+prevwnd->Width>prevwnd->WScreen->MouseX &&
                                                   prevwnd->TopEdge+prevwnd->BorderTop>prevwnd->WScreen->MouseY) {
                                   			p->PullDown = tag->ti_Data;
                                                }
                                            } else if(prevwnd->WScreen->MouseY < prevwnd->WScreen->BarHeight) {
                                                        p->PullDown = tag->ti_Data;
                                            }
                                        }*/

                                        if(p->PullDown) {   // Put pulldowns at screen top-left
                                        	p->RootMenu->MenuX = 0;
                                        	p->RootMenu->MenuY = 0;
                                        	p->RootMenu->MenuLevel = -1;   // Right shadow-size
                                        	p->RootMenu->PM.Layout = PML_Horizontal;
                                        } else {
                                        	p->RootMenu->PM.Layout=PML_Vertical;
                                        }
                                        break;
                                    case PM_MenuHandler:
                                        p->MenuHandler = (struct Hook *)tag->ti_Data;
                                        p->DoMultiSel = TRUE;
                                        break;
                                    case PM_HintBox:
                                        p->HintBox = TRUE;
                                        break;
                                }
                        }

                        if(!shut_down) {

		                switch(PM_Prefs->pmp_MenuBorder) {
		                    case BUTTON_FRAME:
                		        p->BorderWidth=p->BorderHeight=1;
		                        break;
                		    case MAGIC_FRAME:
		                    case THICK_BUTTON_FRAME:
                		    case DOUBLE_FRAME:
		                        p->BorderWidth=p->BorderHeight=2;
                		        break;
		                    case INTUI_FRAME:
                		        p->BorderWidth=2;
		                        p->BorderHeight=1;
                		        break;
		                    case DROPBOX_FRAME:
                		        p->BorderWidth=p->BorderHeight=4;
		                        break;
				}

                                if(p->DrawInfo) {
					if(!p->MenuFont) p->MenuFont=p->DrawInfo->dri_Font;
                                }

				if(!p->MenuFont) p->MenuFont=(struct TextFont *)prevwnd->WScreen->Font;

                                p->RootMenu->PM.Sub = p->PM;

				PM_Image_Allocate(p);

    	    	    	#ifdef __AROS__
				p->treq=EZCreateTimer(UNIT_VBLANK);
			#else
				p->treq=EZCreateTimer(0);
			#endif
                                if(p->treq) {
                                    p->treq->tr_time.tv_secs=0;
                                        p->treq->tr_time.tv_micro=200000;

                                        p->tport=p->treq->tr_node.io_Message.mn_ReplyPort;

                                        p->treq->tr_node.io_Command=TR_ADDREQUEST;
                                        SendIO((struct IORequest *)p->treq);
                                }

				if(p->HintBox)
					PM_DoHint(p->RootMenu);
				else
					PM_DoPopup(p->RootMenu);

                                if(p->treq) {
					AbortIO((struct IORequest *)p->treq);
					WaitIO((struct IORequest *)p->treq);
					EZDeleteTimer(p->treq);
                                }

                        } /* if(!shut_down) */

                        if(p->DrawInfo) FreeScreenDrawInfo(prevwnd->WScreen, p->DrawInfo);

                        ret = p->ReturnCode;

                        Delay(2);

			PM_FreeSubWindow(NULL, p->RootMenu);
	                PM_RemoveHandler(p->pmh);
                }

		PM_DoSelected(p, p->PM);
		PM_Image_Free(p);
		PM_Mem_Free(p);

                return ret;
        } else DisplayBeep(NULL);

        return 0L;
}
///

/// PM_InsertMenuItemA
//
// Function to add an item to a menu
//

LONG __saveds ASM PM_InsertMenuItemA(register __a0 struct PopupMenu *base GNUCREG(a0),
    register __a1 struct TagItem *tags GNUCREG(a1))
{
    struct TagItem *tag, *tstate;
    LONG count=0;
    ULONG method=0; // insertion method
    struct PopupMenu *pointer, *pm;

    if(!base) return 0;

    tstate = tags;
    while((tag=NextTagItem(&tstate))) {
        switch(tag->ti_Tag) {
            case PM_Insert_Before:
            case PM_Insert_After:
            case PM_Insert_First:
            case PM_Insert_Last:
            case PM_InsertSub_Last:
            case PM_InsertSub_First:
            case PM_InsertSub_Sorted:
                method=tag->ti_Tag;
                pointer=(struct PopupMenu *)tag->ti_Data;
                break;
            case PM_Insert_AfterID:
                method=tag->ti_Tag;
                pointer=PM_FindID(base, tag->ti_Data);
                break;
            case PM_Insert_BeforeID:
                method=tag->ti_Tag;
                pointer=(struct PopupMenu *)PM_FindBeforeID(base, tag->ti_Data);
                break;
            case PM_Insert_Item:
                if(tag->ti_Data && pointer) {

                    switch(method) {
                        case PM_Insert_Last:
                            pm=PM_FindLast(base);
                            if(pm) pm->Next=(struct PopupMenu *)tag->ti_Data;
                            break;
                        case PM_Insert_First:   // Relies on first itm being hdr (invis itm)
                            pm=base->Next;
                            base->Next=(struct PopupMenu *)tag->ti_Data;
                            base->Next->Next=pm;
                            break;
                        case PM_Insert_Before:
                            pm=PM_FindBefore(base, pointer);
                            if(pm) {
                                pm->Next=(struct PopupMenu *)tag->ti_Data;
                                pm->Next->Next=pointer;
                            }
                            break;
                        case PM_Insert_BeforeID:
                        case PM_Insert_After:
                        case PM_Insert_AfterID:
                            pm=pointer->Next;
                            pointer->Next=(struct PopupMenu *)tag->ti_Data;
                            pointer->Next->Next=pm;
                            break;
                        case PM_InsertSub_Last:
                            pm=PM_FindLast(pointer);
                            if(pm) pm->Next=(struct PopupMenu *)tag->ti_Data;
                            break;
                        case PM_InsertSub_First:
                            pm=pointer->Sub;
                            pointer->Sub=(struct PopupMenu *)tag->ti_Data;
                            pointer->Sub->Next=pm;
                            break;
                        case PM_InsertSub_Sorted:
                            pm=PM_FindSortedInsertPoint(base, pointer);
                            if(pm) {
                                pm->Next=(struct PopupMenu *)tag->ti_Data;
                                pm->Next->Next=pointer;
                            }
                            break;
                    }

                    count++;
                }
                break;
        }
    }

    return count;
}
///

/// PM_RemoveMenuItem
//
// Function to remove an item
//

struct PopupMenu * __saveds ASM PM_RemoveMenuItem(register __a0 struct PopupMenu *base GNUCREG(a0),
    register __a1 struct PopupMenu *item GNUCREG(a1))
{
    struct PopupMenu *pm, *prev=NULL;

    pm=base;

    if(pm) do {
        if(pm==item) {
            if(prev) {
                prev->Next=item->Next;
                item->Next=NULL;
                return item;
            } else {
                return NULL;
            }
        }
        prev=pm;
        pm=pm->Next;
    } while(pm);

    return NULL;
}
///

/// PM_AbortHook
BOOL __saveds ASM PM_AbortHook(register __a0 APTR handle GNUCREG(a0))
{
    struct PM_Window *a=(struct PM_Window *)handle;
    int mx=a->Wnd->WScreen->MouseX-a->Wnd->LeftEdge;
    int my=a->Wnd->WScreen->MouseY-a->Wnd->TopEdge;

    if(mx>=a->Selected->Left && mx<=a->Selected->Left+a->Selected->Width &&
       my>=a->Selected->Top && my<=a->Selected->Top+a->Selected->Height) {

        if(a->NextWindow) {
            if(a->Selected->Sub) {
                ULONG m;
                a->NextWindow->PM.Sub=a->Selected->Sub;
                CurrentTime(&a->p->Secs, &m);
                if(m-a->p->Micros>60000)    /* Update only every 60th millisecond */
                    PM_OpenPopupWindow(a->NextWindow);
                a->p->Micros=m;
            }
        }

        return FALSE;
    }
    return TRUE;
}
///

/// PM_GetVersion

char version[256];

#ifdef __AROS__
#include "popupmenu_libdefs.h"
#define _LibID VERSION_STRING
#else
extern char _LibID[];
#endif

STRPTR ASM __saveds PM_GetVersion(void)
{
    strcpy(version, _LibID);

    if(CyberGfx) PM_StrCat(version, "\nGraphics card environment found and utilized.");
    if(V40Gfx) PM_StrCat(version, "\nOS3.1 graphics.library found and utilized.");

    return version;
}
///

LONG ASM __saveds PM_LayoutMenuA(register __a0 struct Window *w GNUCREG(a0),
    register __a1 struct PopupMenu *pm GNUCREG(a1),
    register __a2 struct TagItem *tags GNUCREG(a2))
{
    return 0;
}

LONG ASM __saveds PM_RESERVED1()
{
    return 0;
}

