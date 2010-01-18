/*
    Copyright � 2002-2010, The AROS Development Team. All rights reserved.
    Copyright � 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <aros/debug.h>
#include <aros/macros.h>
#include <exec/memory.h>
#include <graphics/rastport.h>
#include <intuition/pointerclass.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include "intuition_intern.h"
#include "../graphics/graphics_private.h"

void MySetPointerPos(struct IntuitionBase *IntuitionBase, int x, int y)
{
    struct IntScreen *scr = GetPrivScreen(IntuitionBase->ActiveScreen);

    IntuitionBase->MouseX = x;
    IntuitionBase->MouseY = y;

    if (scr && scr->Pointer)
    {
        D(bug("MoveSprite Viewport 0x%lx %ld %ld\n",&scr->Screen.ViewPort,x + scr->Pointer->xoffset - scr->Screen.LeftEdge,y + scr->Pointer->yoffset - scr->Screen.TopEdge));
        D(bug("MoveSprite data 0x%lx, height %ld, x %ld, y %ld, num %ld, wordwidth, 0x%lx, flags 0x%lx\n",
                scr->Pointer->sprite->es_SimpleSprite.posctldata,
                scr->Pointer->sprite->es_SimpleSprite.height,
                scr->Pointer->sprite->es_SimpleSprite.x,
                scr->Pointer->sprite->es_SimpleSprite.y,
                scr->Pointer->sprite->es_SimpleSprite.num,
                scr->Pointer->sprite->es_wordwidth,
                scr->Pointer->sprite->es_flags));
        MoveSprite(&scr->Screen.ViewPort, &scr->Pointer->sprite->es_SimpleSprite,
                   x + scr->Pointer->xoffset,
                   y + scr->Pointer->yoffset);
        D(bug("MoveSprite data 0x%lx, height %ld, x %ld, y %ld, num %ld, wordwidth, 0x%lx, flags 0x%lx\n",
                scr->Pointer->sprite->es_SimpleSprite.posctldata,
                scr->Pointer->sprite->es_SimpleSprite.height,
                scr->Pointer->sprite->es_SimpleSprite.x,
                scr->Pointer->sprite->es_SimpleSprite.y,
                scr->Pointer->sprite->es_SimpleSprite.num,
                scr->Pointer->sprite->es_wordwidth,
                scr->Pointer->sprite->es_flags));
    }
}

#ifdef __MORPHOS__

/* TODO: there are no such functions in MorphOS/AmigaOS, may be we should make them private too? */

struct RastPort *MyCreateRastPort(struct IntuitionBase *IntuitionBase)
{
    struct RastPort *newrp = AllocMem(sizeof(*newrp), MEMF_PUBLIC);
    
    if (newrp)
    {
        InitRastPort(newrp);
    }
    
    return newrp;
}

struct RastPort *MyCloneRastPort(struct IntuitionBase *IntuitionBase, struct RastPort *rp)
{
    struct RastPort *newrp = NULL;

    if (rp)
    {
        newrp = AllocMem(sizeof(*newrp), MEMF_PUBLIC);
        if (newrp)
        {
            // *newrp = *rp;

            memcpy(newrp,rp,sizeof(struct RastPort));
        }
    }

    return newrp;
}

void MyFreeRastPort(struct IntuitionBase *IntuitionBase, struct RastPort *rp)
{
    FreeMem(rp, sizeof(*rp));
}

BOOL IsLayerHiddenBySibling(struct Layer *layer, BOOL xx)
{
    struct Window *win = layer->Window;
    
    /* skip requesters attached to the same window. */
    while (layer->front && layer->front->Window == win)
    {
        layer = layer->front;
    }

    /* jDc: we need to care for layers that are on
    ** front of our layer, but don't cover it*/

    if (layer->front)
    {
        struct Layer *lay;
	
        for (lay = layer->front; lay; lay = lay->front)
        {
            struct Window *lwin = lay->Window;
	    
            if (lwin && win)
            {
                if (lwin->LeftEdge > win->LeftEdge + win->Width - 1) continue;
                if (lwin->LeftEdge + lwin->Width - 1 < win->LeftEdge) continue;
                if (lwin->TopEdge > win->TopEdge + win->Height - 1) continue;
                if (lwin->TopEdge + lwin->Height - 1 < win->TopEdge) continue;
                return TRUE;
            }
        }
        return NULL;
	
    } else return NULL;
}

#endif

struct TextFont *SafeReopenFont(struct IntuitionBase *IntuitionBase,
                                struct TextFont **fontptr)
{
    struct TextFont *ret = NULL, *font;

    /* Atomically lock the font before, so it can't go away
     */
    Forbid();

    font = *fontptr;
    if (font)
    {
        struct TextAttr ta;

        font->tf_Accessors++;
        Permit();

        /* Now really open it
         */
        ta.ta_Name  = font->tf_Message.mn_Node.ln_Name;
        ta.ta_YSize = font->tf_YSize;
        ta.ta_Style = font->tf_Style;
        ta.ta_Flags = font->tf_Flags;

        ret = OpenFont(&ta);

        /* Unlock it
         */
        Forbid();
        font->tf_Accessors--;
    }

    Permit();

    return ret;
}

Object *MakePointerFromData(struct IntuitionBase *IntuitionBase,
                            UWORD *source, int xOffset, int yOffset, int width, int height)
{
    struct TagItem pointertags[] = {
        {POINTERA_BitMap      , (IPTR)source},
        {POINTERA_XOffset     , xOffset      },
        {POINTERA_YOffset     , yOffset      },
	{SPRITEA_OldDataFormat, TRUE	     },
	{SPRITEA_Width	      , width	     },
	{SPRITEA_OutputHeight , height	     },
        {TAG_DONE                            }
    };

    return NewObjectA(GetPrivIBase(IntuitionBase)->pointerclass, NULL, pointertags);
}

Object *MakePointerFromPrefs(struct IntuitionBase *IntuitionBase, struct Preferences *prefs)
{
    SetPointerColors(IntuitionBase);
    
    return MakePointerFromData(IntuitionBase, prefs->PointerMatrix, prefs->XOffset, prefs->YOffset, 16, 16);
}

void InstallPointer(struct IntuitionBase *IntuitionBase, Object **old, Object *pointer)
{
    struct IntScreen 	*scr;
    struct Window   	*win;
    struct SharedPointer *oldpointer;
    struct SharedPointer *newpointer;
    
    ULONG lock = LockIBase(0);

    GetAttr(POINTERA_SharedPointer, *old, (IPTR *)&oldpointer);
    GetAttr(POINTERA_SharedPointer, pointer, (IPTR *)&newpointer);

    for (scr = GetPrivScreen(IntuitionBase->FirstScreen); scr; scr = GetPrivScreen(scr->Screen.NextScreen))
    {
        for (win = scr->Screen.FirstWindow; win; win = win->NextWindow)
        {
            if (((struct IntWindow *)win)->pointer == *old)
            {
                win->XOffset = newpointer->xoffset;
                win->YOffset = newpointer->yoffset;
            }
        }

        if (scr->Pointer == oldpointer)
        {
            DEBUG_POINTER(dprintf("InstallPointer: scr 0x%lx pointer 0x%lx sprite 0x%lx\n",
                                  scr, pointer, newpointer->sprite));
            if (ChangeExtSprite(&scr->Screen.ViewPort,
                                oldpointer->sprite, newpointer->sprite,
				CSTAG_XOffset, newpointer->xoffset,
				CSTAG_YOffset, newpointer->yoffset,
				TAG_DONE))
            {
                ObtainSharedPointer(newpointer, IntuitionBase);
                ReleaseSharedPointer(oldpointer, IntuitionBase);
                scr->Pointer = newpointer;
            }
            else
            {
                DEBUG_POINTER(dprintf("InstallPointer: can't change pointer.\n"));
            }
        }
    }

    DisposeObject(*old);
    *old = pointer;

    UnlockIBase(lock);
}

void SetPointerColors(struct IntuitionBase *IntuitionBase)
{
    UWORD   	  *p;
    int     	   k;
    ULONG   	   lock = LockIBase(0);
  //struct Screen *scr;

    DEBUG_POINTER(dprintf("SetPointerColors()\n");)

    p = &GetPrivIBase(IntuitionBase)->ActivePreferences->color17;

    DEBUG_POINTER(dprintf("color17 %04lx color18 %04lx color19 %04lx color20 %04lx\n",p[0],p[1],p[2],p[3]);)

    if (IntuitionBase->ActiveScreen)
    {
        for (k = 0; k < 3; ++k, ++p)
        {
            SetRGB4(&IntuitionBase->ActiveScreen->ViewPort,
                    k + 17, *p >> 8, (*p >> 4) & 0xf, *p & 0xf);
        }
    }

    UnlockIBase(lock);

    DEBUG_POINTER(dprintf("SetPointerColors() done\n");)
}


struct SharedPointer *CreateSharedPointer(struct ExtSprite *sprite, int x, int y,
                    struct IntuitionBase *IntuitionBase)
{
    struct SharedPointer *pointer;

    pointer = AllocMem(sizeof(*pointer), MEMF_PUBLIC);
    if (pointer)
    {
        pointer->sprite = sprite;
        pointer->xoffset = x;
        pointer->yoffset = y;
        pointer->ref_count = 1;
    }

    return pointer;
}

void ObtainSharedPointer(struct SharedPointer *pointer,
                         struct IntuitionBase *IntuitionBase)
{
    ULONG lock = LockIBase(0);
    ++pointer->ref_count;
    UnlockIBase(lock);
}

void ReleaseSharedPointer(struct SharedPointer *pointer,
                          struct IntuitionBase *IntuitionBase)
{
    ULONG lock = LockIBase(0);
    if (--pointer->ref_count == 0)
    {
        FreeSpriteData(pointer->sprite);
        FreeMem(pointer, sizeof(*pointer));
    }
    UnlockIBase(lock);
}
