/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Read an icon from an .info file
    Lang: english
*/

#include <stdio.h>
#include <stddef.h>

#include <exec/memory.h>
#include <aros/structdesc.h>
#include <aros/asmcall.h>
#include <aros/debug.h>
#include <workbench/workbench.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/intuition.h>
#include <graphics/gfxbase.h>
#include <graphics/rastport.h>

#include <clib/alib_protos.h>
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include <clib/aros_protos.h>
#include <clib/intuition_protos.h>

AROS_UFH3(ULONG, ProcessDrawerData,
    AROS_UFHA(struct Hook *,   hook, A0),
    AROS_UFHA(BPTR,            file, A2),
    AROS_UFHA(struct SDData *, data, A1)
);
AROS_UFH3(ULONG, ProcessGadgetRender,
    AROS_UFHA(struct Hook *,   hook, A0),
    AROS_UFHA(BPTR,            file, A2),
    AROS_UFHA(struct SDData *, data, A1)
);
AROS_UFH3(ULONG, ProcessSelectRender,
    AROS_UFHA(struct Hook *,   hook, A0),
    AROS_UFHA(BPTR,            file, A2),
    AROS_UFHA(struct SDData *, data, A1)
);
AROS_UFH3(ULONG, ProcessDefaultTool,
    AROS_UFHA(struct Hook *,   hook, A0),
    AROS_UFHA(BPTR,            file, A2),
    AROS_UFHA(struct SDData *, data, A1)
);
AROS_UFH3(ULONG, ProcessToolTypes,
    AROS_UFHA(struct Hook *,   hook, A0),
    AROS_UFHA(BPTR,            file, A2),
    AROS_UFHA(struct SDData *, data, A1)
);
AROS_UFH3(ULONG, ProcessFlagPtr,
    AROS_UFHA(struct Hook *,   hook, A0),
    AROS_UFHA(BPTR,            file, A2),
    AROS_UFHA(struct SDData *, data, A1)
);

struct Hook ProcessDrawerDataHook =
{
    { NULL, NULL}, ProcessDrawerData, NULL, NULL
},
ProcessGadgetRenderHook =
{
    { NULL, NULL}, ProcessGadgetRender, NULL, NULL
},
ProcessSelectRenderHook =
{
    { NULL, NULL}, ProcessSelectRender, NULL, NULL
},
ProcessFlagPtrHook =
{
    { NULL, NULL}, ProcessFlagPtr, NULL, NULL
},
ProcessDefaultToolHook =
{
    { NULL, NULL}, ProcessDefaultTool, NULL, NULL
},
ProcessToolTypesHook =
{
    { NULL, NULL}, ProcessToolTypes, NULL, NULL
};

#undef O
#define O(x)    (offsetof (struct Gadget,x))
IPTR GadgetDesc[] =
{
    sizeof (struct Gadget),
    SDM_IGNORE(4), /* NextGadget */
    SDM_WORD(O(LeftEdge)),
    SDM_WORD(O(TopEdge)),
    SDM_WORD(O(Width)),
    SDM_WORD(O(Height)),
    SDM_UWORD(O(Flags)),
    SDM_UWORD(O(Activation)),
    SDM_UWORD(O(GadgetType)),
    SDM_SPECIAL(O(GadgetRender),&ProcessFlagPtrHook),
    SDM_SPECIAL(O(SelectRender),&ProcessFlagPtrHook),
    SDM_SPECIAL(O(GadgetText),&ProcessFlagPtrHook),
    SDM_LONG(O(MutualExclude)),
    SDM_SPECIAL(O(SpecialInfo),&ProcessFlagPtrHook),
    SDM_UWORD(O(GadgetID)),
    SDM_ULONG(O(UserData)),
    SDM_END
};

#undef O
#define O(x)    (offsetof (struct DiskObject,x))
IPTR DiskObjectDesc[] =
{
    sizeof (struct DiskObject),
    SDM_UWORD(O(do_Magic)),
    SDM_UWORD(O(do_Version)),
    SDM_STRUCT(O(do_Gadget),GadgetDesc),
    SDM_UBYTE(O(do_Type)),
    SDM_IGNORE(1), /* Pad */
    SDM_SPECIAL(O(do_DefaultTool),&ProcessFlagPtrHook),
    SDM_SPECIAL(O(do_ToolTypes),&ProcessFlagPtrHook),
    SDM_LONG(O(do_CurrentX)),
    SDM_LONG(O(do_CurrentY)),
    SDM_SPECIAL(O(do_DrawerData),&ProcessFlagPtrHook),
    SDM_SPECIAL(O(do_ToolWindow),&ProcessFlagPtrHook),
    SDM_LONG(O(do_StackSize)),
    SDM_END
};

#undef O
#define O(x)    (offsetof (struct Image,x))
IPTR ImageDesc[] =
{
    sizeof (struct Image),
    SDM_WORD(O(LeftEdge)),
    SDM_WORD(O(TopEdge)),
    SDM_WORD(O(Width)),
    SDM_WORD(O(Height)),
    SDM_WORD(O(Depth)),
    SDM_SPECIAL(O(ImageData),&ProcessFlagPtrHook),
    SDM_UBYTE(O(PlanePick)),
    SDM_UBYTE(O(PlaneOnOff)),
    SDM_SPECIAL(O(NextImage),&ProcessFlagPtrHook),
    SDM_END
};

IPTR IconDesc[] =
{
    sizeof (struct DiskObject),
    SDM_STRUCT(0,DiskObjectDesc),
    SDM_SPECIAL(0,&ProcessDrawerDataHook),
    SDM_SPECIAL(0,&ProcessGadgetRenderHook),
    SDM_SPECIAL(0,&ProcessSelectRenderHook),
    SDM_SPECIAL(0,&ProcessDefaultToolHook),
    SDM_SPECIAL(0,&ProcessToolTypesHook),
    SDM_END
};

#define IM(x)       ((struct Image *)x)

struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;

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
	bug ("Couldn't open intuition.library\n");
	goto end;
    }

    dobj->do_Gadget.LeftEdge = dobj->do_Gadget.Width * 2 + 30;
    dobj->do_Gadget.TopEdge  = 10;

    win = OpenWindowTags (NULL
	, WA_Title,	    "Show an icon"
	, WA_Left,	    100
	, WA_Top,	    100
	, WA_Width,	    dobj->do_Gadget.Width * 3 + 40
	, WA_Height,	    dobj->do_Gadget.Height + 20
	, WA_IDCMP,	    IDCMP_RAWKEY
	, WA_SimpleRefresh, TRUE
	, WA_Gadgets,	    &dobj->do_Gadget
	, TAG_END
    );
    D(bug("OpenWindow win=%p\n", win));

    if (!win)
	goto end;

    rp = win->RPort;

    DrawImage (rp, dobj->do_Gadget.GadgetRender, 10, 10);
    DrawImage (rp, dobj->do_Gadget.SelectRender, 20 + dobj->do_Gadget.Width, 10);

    cont = 1;

    printf ("Press a key to exit\n");
    Flush (Output ());

    while (cont)
    {
	if ((im = (struct IntuiMessage *)GetMsg (win->UserPort)))
	{
	    /* D("Got msg\n"); */
	    switch (im->Class)
	    {
	    case IDCMP_RAWKEY:
		cont = FALSE;
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
    {
	D(bug("CloseWindow (%p)\n", win));
	CloseWindow (win);
    }

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
    BPTR   icon;
    int    rc;
    int    t;

    rc = 0;

    rda = ReadArgs ("IconFile/A", (IPTR *)&arg, NULL);

    if (rda)
    {
	if (!(icon = Open (arg, MODE_OLDFILE)) )
	{
	    printf ("Cannot open file %s for reading: ", arg);
	    PrintFault (IoErr(), "");
	    rc = 10;
	}
	else
	{
	    if (!ReadStruct (icon, (APTR *)&dobj, IconDesc))
	    {
		printf ("Error reading file %s: ", arg);
		PrintFault (IoErr(), "");
		rc = 10;
	    }
	    else
	    {
		Close (icon);

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
		    , dobj->do_StackSize
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

		for (t=0; dobj->do_ToolTypes[t]; t++)
		    printf ("TT %d: %s\n", t, dobj->do_ToolTypes[t]);

		if (!(icon = Open ("readicon.info", MODE_NEWFILE)) )
		    PrintFault (IoErr(), "Cannot write icon to readicon.info");
		else
		{
		    if (!WriteStruct (icon, dobj, IconDesc))
			PrintFault (IoErr(), "Writing of icon to readicon.info failed");

		    Close (icon);
		}

		DoWindow (dobj);

		FreeStruct (dobj, DiskObjectDesc);
	    }
	}

	FreeArgs (rda);
    }
    else
	rc = 10;

    return rc;
} /* main */

#define DO(x)       ((struct DiskObject *)x)

AROS_UFH3(ULONG, ProcessDrawerData,
    AROS_UFHA(struct Hook *,   hook, A0),
    AROS_UFHA(BPTR,            file, A2),
    AROS_UFHA(struct SDData *, data, A1)
)
{
/* kprintf ("ProcessDrawerData\n"); */
    if (DO(data->sdd_Dest)->do_Type == WBDRAWER)
    {
	switch (data->sdd_Mode)
	{
	case SDV_SPECIALMODE_READ:
	    Flush (file);

	    return Seek (file, DRAWERDATAFILESIZE, OFFSET_CURRENT) != EOF;

	case SDV_SPECIALMODE_WRITE:
	    return Write (file
		, DO(data->sdd_Dest)->do_DrawerData
		, DRAWERDATAFILESIZE
	    ) != EOF;

	case SDV_SPECIALMODE_FREE:
	    break;
	}
    }

    return TRUE;
} /* ProcessDrawerData */

struct Image * ReadImage (BPTR file)
{
    struct Image * image;
    ULONG	   size;
    ULONG	   t;

    if (!ReadStruct (file, (APTR *)&image, ImageDesc))
	return NULL;

    /* Size of imagedata in bytes */
    size = ((image->Width + 15) >> 3) * image->Height * image->Depth;

/* kprintf ("ReadImage: %dx%dx%d (%d bytes)\n"
    , image->Width
    , image->Height
    , image->Depth
    , size
); */

    if (size)
    {
	if (!(image->ImageData = AllocMem (size, MEMF_CHIP)) )
	{
	    FreeStruct (image, ImageDesc);
	    return NULL;
	}

	size >>= 1; /* Get size in words */

	for (t=0; t<size; t++)
	    if (!ReadWord (file, &image->ImageData[t]))
		break;

	if (t != size)
	{
	    FreeStruct (image, ImageDesc);
	    return NULL;
	}
    }

    return image;
} /* ReadImage */

int WriteImage (BPTR file, struct Image * image)
{
    ULONG size;
    ULONG t;

    if (!WriteStruct (file, image, ImageDesc) )
	return FALSE;

    /* Get size in words */
    size = ((image->Width + 15) >> 4) * image->Height * image->Depth;

    for (t=0; t<size; t++)
	if (!WriteWord (file, image->ImageData[t]))
	    break;

    return (t == size);
} /* WriteImage */

void FreeImage (struct Image * image)
{
    ULONG size;

    /* Get size in bytes */
    size = ((image->Width + 15) >> 3) * image->Height * image->Depth;

    if (size)
	FreeMem (image->ImageData, size);

    FreeStruct (image, ImageDesc);
} /* FreeImage */

AROS_UFH3(ULONG, ProcessGadgetRender,
    AROS_UFHA(struct Hook *,   hook, A0),
    AROS_UFHA(BPTR,            file, A2),
    AROS_UFHA(struct SDData *, data, A1)
)
{
    struct Image * image;
/* kprintf ("ProcessGadgetRender\n"); */

    switch (data->sdd_Mode)
    {
    case SDV_SPECIALMODE_READ:
	image = ReadImage (file);

	if (!image)
	    return FALSE;

	DO(data->sdd_Dest)->do_Gadget.GadgetRender = image;

	break;

    case SDV_SPECIALMODE_WRITE:
	image = DO(data->sdd_Dest)->do_Gadget.GadgetRender;

	return WriteImage (file, image);

    case SDV_SPECIALMODE_FREE:
	image = DO(data->sdd_Dest)->do_Gadget.GadgetRender;

	FreeImage (image);

	break;
    }

    return TRUE;
} /* ProcessGadgetRender */

AROS_UFH3(ULONG, ProcessSelectRender,
    AROS_UFHA(struct Hook *,   hook, A0),
    AROS_UFHA(BPTR,            file, A2),
    AROS_UFHA(struct SDData *, data, A1)
)
{
    struct Image * image;
/* kprintf ("ProcessSelectRender\n"); */

    if (DO(data->sdd_Dest)->do_Gadget.Flags & GFLG_GADGHIMAGE)
    {
	switch (data->sdd_Mode)
	{
	case SDV_SPECIALMODE_READ:
	    image = ReadImage (file);

	    if (!image)
		return FALSE;

	    DO(data->sdd_Dest)->do_Gadget.SelectRender = image;

	    break;

	case SDV_SPECIALMODE_WRITE:
	    image = DO(data->sdd_Dest)->do_Gadget.SelectRender;

	    return WriteImage (file, image);

	case SDV_SPECIALMODE_FREE:
	    image = DO(data->sdd_Dest)->do_Gadget.SelectRender;

	    FreeImage (image);

	    break;
	}
    }

    return TRUE;
} /* ProcessSelectRender */

AROS_UFH3(ULONG, ProcessFlagPtr,
    AROS_UFHA(struct Hook *,   hook, A0),
    AROS_UFHA(BPTR,            file, A2),
    AROS_UFHA(struct SDData *, data, A1)
)
{
    LONG ptr;

    switch (data->sdd_Mode)
    {
    case SDV_SPECIALMODE_READ:
	if (FRead (file, &ptr, 4, 1) == EOF)
	    return FALSE;

	*((APTR *)data->sdd_Dest) = (APTR)(ptr != 0L);

	break;

    case SDV_SPECIALMODE_WRITE:
	if (*((APTR *)data->sdd_Dest))
	    ptr = 0xABADCAFEL;
	else
	    ptr = 0L;

	if (FWrite (file, &ptr, 4, 1) == EOF)
	    return FALSE;

	break;

    case SDV_SPECIALMODE_FREE:
	break;
    }


    return TRUE;
} /* ProcessFlagPtr */

STRPTR ReadIconString (BPTR file)
{
    ULONG  len;
    STRPTR str;

    if (!ReadLong (file, &len))
	return NULL;

    str = AllocMem (len, MEMF_ANY);

    if (!str)
	return NULL;

    if (FRead (file, str, len, 1) == EOF)
    {
	FreeMem (str, len);
	return NULL;
    }

    return str;
} /* ReadIconString */

int WriteIconString (BPTR file, STRPTR str)
{
    ULONG len;

    len = strlen (str) + 1;

    if (!WriteLong (file, len))
	return FALSE;

    return FWrite (file, str, len, 1) != EOF;
} /* WriteIconString */

AROS_UFH3(ULONG, ProcessDefaultTool,
    AROS_UFHA(struct Hook *,   hook, A0),
    AROS_UFHA(BPTR,            file, A2),
    AROS_UFHA(struct SDData *, data, A1)
)
{
    STRPTR str;

    if (DO(data->sdd_Dest)->do_DefaultTool)
    {
	switch (data->sdd_Mode)
	{
	case SDV_SPECIALMODE_READ:
	    str = ReadIconString (file);

	    if (!str)
		return FALSE;

	    DO(data->sdd_Dest)->do_DefaultTool = str;

	    break;

	case SDV_SPECIALMODE_WRITE: {
	    str = DO(data->sdd_Dest)->do_DefaultTool;

	    WriteIconString (file, str);

	    break; }

	case SDV_SPECIALMODE_FREE:
	    str = DO(data->sdd_Dest)->do_DefaultTool;

	    FreeMem (str, strlen (str)+1);

	    break;
	}
    }

    return TRUE;
} /* ProcessDefaultTool */

AROS_UFH3(ULONG, ProcessToolTypes,
    AROS_UFHA(struct Hook *,   hook, A0),
    AROS_UFHA(BPTR,            file, A2),
    AROS_UFHA(struct SDData *, data, A1)
)
{
    if (DO(data->sdd_Dest)->do_ToolTypes)
    {
	ULONG	 t;
	ULONG	 count;
	STRPTR * ttarray;

	switch (data->sdd_Mode)
	{
	case SDV_SPECIALMODE_READ:
	    /* Read size of ToolTypes array (each entry is 4 bytes and the
	       last is 0L */
	    if (!ReadLong (file, &count))
		return FALSE;

	    count = (count >> 2) - 1; /* How many entries */

	    ttarray = AllocMem ((count+1)*sizeof(STRPTR), MEMF_ANY);

	    for (t=0; t<count; t++)
	    {
		ttarray[t] = ReadIconString (file);

		if (!ttarray[t])
		{
		    ULONG i;

		    for (i=0; i<t; i++)
			FreeMem (ttarray[t], strlen (ttarray[t])+1);

		    FreeMem (ttarray, (count+1)*sizeof(STRPTR));

		    return FALSE;
		}
	    }

	    ttarray[t] = NULL;

	    DO(data->sdd_Dest)->do_ToolTypes = (char **)ttarray;

	    break;

	case SDV_SPECIALMODE_WRITE: {
	    ULONG size;

	    ttarray = (STRPTR *)DO(data->sdd_Dest)->do_ToolTypes;

	    for (count=0; ttarray[count]; count++);

	    size = (count+1)*4;

	    if (!WriteLong (file, size))
		return FALSE;

	    for (t=0; t<count; t++)
	    {
		if (!WriteIconString (file, ttarray[t]))
		    return FALSE;
	    }

	    break; }

	case SDV_SPECIALMODE_FREE:
	    ttarray = (STRPTR *)DO(data->sdd_Dest)->do_ToolTypes;

	    for (t=0; ttarray[t]; t++)
		FreeMem (ttarray[t], strlen (ttarray[t])+1);

	    FreeMem (ttarray, (t+1)*sizeof(STRPTR));

	    break;
	}
    }

    return TRUE;
} /* ProcessToolTypes */


