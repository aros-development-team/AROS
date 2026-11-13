#include <intuition/intuition.h>
#include <libraries/gadtools.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/gadtools.h>
#include <proto/alib.h>

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <math.h>

struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
struct Library *GadToolsBase;

static struct Screen *scr;
static struct DrawInfo *dri;
static struct Window *win;
static struct Menu *menus;
static APTR vi;

static struct NewMenu nm[] =
{
    {NM_TITLE, "Menu Title 1" },
     {NM_ITEM, "Item 1", "A"},
     {NM_ITEM, "Item 2", "B"},
     {NM_ITEM, "Item 3", "C"},
     {NM_ITEM, NM_BARLABEL },
     {NM_ITEM, "Item 4" },
      {NM_SUB, "Subitem 1", "!"},
      {NM_SUB, "Subitem 2", "O"},
      {NM_SUB, NM_BARLABEL},
      {NM_SUB, "Subitem 3", "Q"},
     {NM_ITEM, "Item 5" },
     {NM_ITEM, NM_BARLABEL},
     {NM_ITEM, "Quit" },
    {NM_TITLE, "Menu Title 2"},
     {NM_ITEM, "One", "1", CHECKIT | CHECKED | MENUTOGGLE},
     {NM_ITEM, "Two", "2"},
     {NM_ITEM, "Three", "3"},
    {NM_TITLE, "Menus with Commandstrings"},
     {NM_ITEM, "Operation 1", "CTRL 1", NM_COMMANDSTRING},
     {NM_ITEM, "Operation 3", "CTRL 2", NM_COMMANDSTRING},
     {NM_ITEM, "Help", "ALT 3", NM_COMMANDSTRING},
     {NM_ITEM, NM_BARLABEL},
     {NM_ITEM, "Cool", "4", CHECKIT | CHECKED, 32 + 64 + 128},
     {NM_ITEM, "Great", "5", CHECKIT, 16 + 64 + 128},
     {NM_ITEM, "Nice", "6", CHECKIT, 16 + 32 + 128},
     {NM_ITEM, "Bad", "7", CHECKIT, 16 + 32 + 64},
    {NM_TITLE, "Disabled", NULL, NM_MENUDISABLED},
     {NM_ITEM, "Item 1"},
     {NM_ITEM, "Item 2"},
     {NM_ITEM, "Item 3"},
     {NM_ITEM, NM_BARLABEL},
     {NM_ITEM, "Sub"},
      {NM_SUB, "Sub 1"},
      {NM_SUB, "Sub 2"},
      {NM_SUB, "Sub 3"},
    {NM_END}
};

static void Cleanup(char *msg)
{
    WORD rc;
    
    if (msg)
    {
	printf("MenuDemo: %s\n",msg);
	rc = RETURN_WARN;
    } else {
	rc = RETURN_OK;
    }
        
    if (win)
    {
        ClearMenuStrip(win);
        CloseWindow(win);
    }
    
    if (menus) FreeMenus(menus);
    
    if (vi) FreeVisualInfo(vi);
    if (dri) FreeScreenDrawInfo(scr,dri);
    if (scr) UnlockPubScreen(0,scr);
    
    if (GadToolsBase) CloseLibrary(GadToolsBase);
    if (GfxBase) CloseLibrary((struct Library *)GfxBase);
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    
    exit (rc);
}

static void OpenLibs(void)
{
    if (!(IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library",39)))
    {
	Cleanup("Can't open intuition.library V39!");
    }
    
    if (!(GfxBase = (struct GfxBase *)OpenLibrary("graphics.library",39)))
    {
	Cleanup("Can't open graphics.library V39!");
    }
    
    if (!(GadToolsBase = OpenLibrary("gadtools.library",39)))
    {
	Cleanup("Can't open gadtools.library V39!");
    }    
}

static void GetVisual(void)
{
    if (!(scr = LockPubScreen(0)))
    {
	Cleanup("Can't lock screen!");
    }
   
    if (!(dri = GetScreenDrawInfo(scr)))
    {
	Cleanup("Can't get drawinfo!");
    }
    
    if (!(vi = GetVisualInfo(scr,0)))
    {
	Cleanup("Can't get visual info!");
    }
}

static void MakeMenus(void)
{
    menus = CreateMenusA(nm, NULL);
    if (!menus) Cleanup("Can't create menus!");
    
    if (!LayoutMenusA(menus, vi, NULL)) Cleanup("Can't layout menus!");
}

static void MakeWin(void)
{
    win = OpenWindowTags(0,WA_Left,scr->MouseX,
			   WA_Top,scr->MouseY,
			   WA_Width,200,
			   WA_Height,scr->WBorTop + scr->Font->ta_YSize + 1,
			   WA_AutoAdjust,TRUE,
			   WA_Title,(IPTR)"MenuDemo",
			   WA_CloseGadget,TRUE,
			   WA_DepthGadget,TRUE,
			   WA_DragBar,TRUE,
			   WA_Activate,TRUE,
			   WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_MENUPICK,
			   TAG_DONE);

    if (!win) Cleanup("Can't open window!");
    
    SetMenuStrip(win, menus);
    
    ScreenToFront(win->WScreen);
}

static void HandleAll(void)
{
    struct IntuiMessage *msg;
    struct MenuItem *item;
    UWORD men;
    
    BOOL quitme = FALSE;
    
    while(!quitme)
    {
	WaitPort(win->UserPort);
	while ((msg = (struct IntuiMessage *)GetMsg(win->UserPort)))
	{
	    switch(msg->Class)
	    {
	    case IDCMP_CLOSEWINDOW:
		quitme = TRUE;
		break;
	
	    case IDCMP_MENUPICK:
	        printf("\nMENUPICK: ------------------------\n\n");
		men = msg->Code;
		
		while(men != MENUNULL)
		{
		    printf("MENU %d, ITEM %d, SUBITEM %d\n", MENUNUM(men), ITEMNUM(men), SUBNUM(men));
		
		    if ((item = ItemAddress(menus, men)))
		    {
		        men = item->NextSelect;
		    } else {
		        men = MENUNULL;
		    }
		}
	        break;
					
	    } /* switch(msg->Class) */
	    
	    ReplyMsg((struct Message *)msg);
	    
	} /* while ((msg = (struct IntuiMessage *)GetMsg(win->UserPort))) */
	
    } /* while(!quitme) */
}

int main(void)
{
    OpenLibs();
    GetVisual();
    MakeMenus();
    MakeWin();
    HandleAll();
    Cleanup(0);
    return 0;
}

