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
/* #include <intuition/intuitionbase.h>
#include <intuition/intuition.h>
#include <graphics/gfxbase.h>
#include <graphics/rastport.h> */

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/aros.h>
#include <proto/intuition.h>

static AROS_UFH3(ULONG, ProcessDrawerData,
    AROS_UFHA(struct Hook *,   hook, A0),
    AROS_UFHA(BPTR,            file, A2),
    AROS_UFHA(struct SDData *, data, A1)
);
static AROS_UFH3(ULONG, ProcessGadgetRender,
    AROS_UFHA(struct Hook *,   hook, A0),
    AROS_UFHA(BPTR,            file, A2),
    AROS_UFHA(struct SDData *, data, A1)
);
static AROS_UFH3(ULONG, ProcessSelectRender,
    AROS_UFHA(struct Hook *,   hook, A0),
    AROS_UFHA(BPTR,            file, A2),
    AROS_UFHA(struct SDData *, data, A1)
);
static AROS_UFH3(ULONG, ProcessDefaultTool,
    AROS_UFHA(struct Hook *,   hook, A0),
    AROS_UFHA(BPTR,            file, A2),
    AROS_UFHA(struct SDData *, data, A1)
);
static AROS_UFH3(ULONG, ProcessToolTypes,
    AROS_UFHA(struct Hook *,   hook, A0),
    AROS_UFHA(BPTR,            file, A2),
    AROS_UFHA(struct SDData *, data, A1)
);
static AROS_UFH3(ULONG, ProcessFlagPtr,
    AROS_UFHA(struct Hook *,   hook, A0),
    AROS_UFHA(BPTR,            file, A2),
    AROS_UFHA(struct SDData *, data, A1)
);

static const struct Hook ProcessDrawerDataHook =
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
static const IPTR GadgetDesc[] =
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
static const IPTR DiskObjectDesc[] =
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
static const IPTR ImageDesc[] =
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

const IPTR IconDesc[] =
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

/*    if (!WriteStruct (icon, dobj, IconDesc))

		FreeStruct (dobj, DiskObjectDesc); */

#define DO(x)       ((struct DiskObject *)x)

static AROS_UFH3(ULONG, ProcessDrawerData,
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

static struct Image * ReadImage (BPTR file)
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

static int WriteImage (BPTR file, struct Image * image)
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

static void FreeImage (struct Image * image)
{
    ULONG size;

    /* Get size in bytes */
    size = ((image->Width + 15) >> 3) * image->Height * image->Depth;

    if (size)
	FreeMem (image->ImageData, size);

    FreeStruct (image, ImageDesc);
} /* FreeImage */

static AROS_UFH3(ULONG, ProcessGadgetRender,
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

static AROS_UFH3(ULONG, ProcessSelectRender,
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

static AROS_UFH3(ULONG, ProcessFlagPtr,
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

static STRPTR ReadIconString (BPTR file)
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

static int WriteIconString (BPTR file, STRPTR str)
{
    ULONG len;

    len = strlen (str) + 1;

    if (!WriteLong (file, len))
	return FALSE;

    return FWrite (file, str, len, 1) != EOF;
} /* WriteIconString */

static AROS_UFH3(ULONG, ProcessDefaultTool,
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

static AROS_UFH3(ULONG, ProcessToolTypes,
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


