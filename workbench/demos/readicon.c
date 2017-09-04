/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Read an icon from an .info file
    Lang: english
*/
#include <stdio.h>

#include <aros/bigendianio.h>
#include <graphics/gfxbase.h>
#include <workbench/workbench.h>
#include <workbench/icon.h>
#include <devices/inputevent.h>

#include <proto/icon.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>

static const char version[] __attribute__((used)) = "$VER: readicon 41.2 (5.9.1997)\n";

#define IM(x)       ((struct Image *)x)

struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
struct Library * IconBase;

void DoWindow (struct DiskObject * dobj)
{
    struct Window   * win = NULL;
    struct RastPort * rp;
    struct IntuiMessage * im;
    int cont;

    GfxBase=(struct GfxBase *)OpenLibrary(GRAPHICSNAME,39);
    IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",39);

    if (!GfxBase)
    {
	printf ("Couldn't open %s\n", GRAPHICSNAME);
	goto end;
    }

    if (!IntuitionBase)
    {
	printf ("Couldn't open intuition.library\n");
	goto end;
    }

    win = OpenWindowTags (NULL
	, WA_Title,	    (IPTR)"Show an icon"
	, WA_DragBar,       TRUE
	, WA_CloseGadget,   TRUE
	, WA_DepthGadget,   TRUE
	, WA_Activate,      TRUE
	, WA_Left,	    100
	, WA_Top,	    100
	, WA_InnerWidth,    dobj->do_Gadget.Width * 3 + 40
	, WA_InnerHeight,   dobj->do_Gadget.Height + 20
	, WA_IDCMP,	    IDCMP_RAWKEY | IDCMP_CLOSEWINDOW
	, WA_SimpleRefresh, TRUE
	, TAG_END
    );

    if (!win)
	goto end;

    rp = win->RPort;

    dobj->do_Gadget.LeftEdge = win->BorderLeft + dobj->do_Gadget.Width * 2 + 30;
    dobj->do_Gadget.TopEdge  = win->BorderTop + 10;

    AddGadget(win, &dobj->do_Gadget, -1);
    RefreshGList(&dobj->do_Gadget, win, NULL, 1);
    
    DrawImage (rp, dobj->do_Gadget.GadgetRender, win->BorderLeft + 10, win->BorderTop + 10);
    DrawImage (rp, dobj->do_Gadget.SelectRender, win->BorderLeft + 20 + dobj->do_Gadget.Width, win->BorderTop + 10);

    cont = 1;

    printf ("Press a key to exit\n");

    while (cont)
    {
	if ((im = (struct IntuiMessage *)GetMsg (win->UserPort)))
	{
	    /* D("Got msg\n"); */
	    printf("Got msg %lx\n", (long)im->Class);
	    
	    switch (im->Class)
	    {
	    case IDCMP_CLOSEWINDOW:
	    	cont = FALSE;
    	    	break;
		
	    case IDCMP_RAWKEY:
	    	if (!(im->Code & IECODE_UP_PREFIX))
		{
		    cont = FALSE;
		}
		break;

	    }

	    ReplyMsg ((struct Message *)im);
	}
	else
	{
	    /* D("Waiting\n"); */
	    Wait (1L << win->UserPort->mp_SigBit);
	}
    }

end:
    if (win)
	CloseWindow (win);

    if (GfxBase)
	CloseLibrary ((struct Library *)GfxBase);

    if (IntuitionBase)
	CloseLibrary ((struct Library *)IntuitionBase);

    return;
} /* DoWindow */

int main (int argc, char ** argv)
{
    struct RDArgs     * rda;
    struct DiskObject * dobj;

    STRPTR arg;
    int    rc;
    int    t;

    rc = 0;

    IconBase = OpenLibrary (ICONNAME, 39);

    if (!IconBase)
    {
	printf ("Couldn't open %s\n", ICONNAME);
	return RETURN_FAIL;
    }

    rda = ReadArgs ("IconFile/A", (IPTR *)&arg, NULL);

    if (rda)
    {
	if (!(dobj = GetDiskObject (arg)) )
	{
	    printf ("Cannot open icon for %s: ", arg);
	    PrintFault (IoErr(), "");
	    rc = 10;
	}
	else
	{
	    /* hexdump (dobj, 0L, sizeof (struct DiskObject)); */

	    printf ("Some information about the icon:\n"
		"Magic = %d\n"
		"Version = %d\n"
		"Type = %d\n"
		"Gadget: %dx%d+%d+%d Flags=%x Act=%x Type=%d\n"
		"Stack = %ld\n"
		, dobj->do_Magic
		, dobj->do_Version
		, dobj->do_Type
		, dobj->do_Gadget.Width
		, dobj->do_Gadget.Height
		, dobj->do_Gadget.LeftEdge
		, dobj->do_Gadget.TopEdge
		, dobj->do_Gadget.Flags
		, dobj->do_Gadget.Activation
		, dobj->do_Gadget.GadgetType
		, (long)dobj->do_StackSize
	    );

	    if (dobj->do_Gadget.GadgetRender)
	    {
		printf ("GImage: %dx%d+%d+%d\n"
		    , IM(dobj->do_Gadget.GadgetRender)->Width
		    , IM(dobj->do_Gadget.GadgetRender)->Height
		    , IM(dobj->do_Gadget.GadgetRender)->LeftEdge
		    , IM(dobj->do_Gadget.GadgetRender)->TopEdge
		);

		/* hexdump (IM(dobj->do_Gadget.GadgetRender)->ImageData
		    , 0L
		    , 720
		); */
	    }
	    else
	    {
		printf ("GImage: none\n");
	    }

	    if (dobj->do_Gadget.SelectRender)
	    {
		printf ("SImage: %dx%d+%d+%d\n"
		    , IM(dobj->do_Gadget.SelectRender)->Width
		    , IM(dobj->do_Gadget.SelectRender)->Height
		    , IM(dobj->do_Gadget.SelectRender)->LeftEdge
		    , IM(dobj->do_Gadget.SelectRender)->TopEdge
		);

		/* hexdump (IM(dobj->do_Gadget.SelectRender)->ImageData
		    , 0L
		    , 720
		); */
	    }
	    else
	    {
		printf ("SImage: none\n");
	    }

	    printf ("DefaultTool: %s\n", dobj->do_DefaultTool);

	    printf ("ToolTypes:\n");

	    if (dobj->do_ToolTypes)
		for (t=0; dobj->do_ToolTypes[t]; t++)
		    printf ("TT %d: %s\n", t, dobj->do_ToolTypes[t]);
	    else
		printf ("--- none ---\n");

	    if (!PutDiskObject ("readicon", dobj))
		PrintFault (IoErr(), "Writing of icon to readicon.info failed");

	    DoWindow (dobj);

	    FreeDiskObject (dobj);
	}

	FreeArgs (rda);
    }
    else
    {
        PrintFault(IoErr(), argv[0]);
	rc = 10;
    }
    
    return rc;
} /* main */

