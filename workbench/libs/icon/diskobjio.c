/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    Read an icon from an .info file
*/

#ifdef __MORPHOS__
#undef __NOLIBBASE__
#endif

/****************************************************************************************/

#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include <exec/memory.h>
#include <aros/bigendianio.h>
#include <aros/asmcall.h>
#include <aros/macros.h>
#include <workbench/workbench.h>

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/arossupport.h>
#include <proto/intuition.h>

#include "icon_intern.h"

#   include <aros/debug.h>

/****************************************************************************************/

AROS_UFP3S(ULONG, ProcessClearMem,
    AROS_UFPA(struct Hook *,   hook, A0),
    AROS_UFPA(struct Hook *,   streamhook, A2),
    AROS_UFPA(struct SDData *, data, A1)
);

AROS_UFP3S(ULONG, ProcessCheckFileType,
    AROS_UFPA(struct Hook *,   hook, A0),
    AROS_UFPA(struct Hook *,   streamhook, A2),
    AROS_UFPA(struct SDData *, data, A1)
);

AROS_UFP3S(ULONG, ProcessOldDrawerData,
    AROS_UFPA(struct Hook *,   hook, A0),
    AROS_UFPA(struct Hook *,   streamhook, A2),
    AROS_UFPA(struct SDData *, data, A1)
);

AROS_UFP3S(ULONG, ProcessNewDrawerData,
    AROS_UFPA(struct Hook *,   hook, A0),
    AROS_UFPA(struct Hook *,   streamhook, A2),
    AROS_UFPA(struct SDData *, data, A1)
);

AROS_UFP3S(ULONG, ProcessGadgetRender,
    AROS_UFPA(struct Hook *,   hook, A0),
    AROS_UFPA(struct Hook *,    streamhook, A2),
    AROS_UFPA(struct SDData *, data, A1)
);

AROS_UFP3S(ULONG, ProcessSelectRender,
    AROS_UFPA(struct Hook *,   hook, A0),
    AROS_UFPA(struct Hook *,   streamhook, A2),
    AROS_UFPA(struct SDData *, data, A1)
);

AROS_UFP3S(ULONG, ProcessDefaultTool,
    AROS_UFPA(struct Hook *,   hook, A0),
    AROS_UFPA(struct Hook *,   streamhook, A2),
    AROS_UFPA(struct SDData *, data, A1)
);

AROS_UFP3S(ULONG, ProcessToolWindow,
    AROS_UFPA(struct Hook *,   hook, A0),
    AROS_UFPA(struct Hook *,   streamhook, A2),
    AROS_UFPA(struct SDData *, data, A1)
);

AROS_UFP3S(ULONG, ProcessToolTypes,
    AROS_UFPA(struct Hook *,   hook, A0),
    AROS_UFPA(struct Hook *,   streamhook, A2),
    AROS_UFPA(struct SDData *, data, A1)
);

AROS_UFP3S(ULONG, ProcessFlagPtr,
    AROS_UFPA(struct Hook *,   hook, A0),
    AROS_UFPA(struct Hook *,   streamhook, A2),
    AROS_UFPA(struct SDData *, data, A1)
);

AROS_UFP3S(ULONG, ProcessIcon35,
    AROS_UFPA(struct Hook *,   hook, A0),
    AROS_UFPA(struct Hook *,   streamhook, A2),
    AROS_UFPA(struct SDData *, data, A1)
);

static const struct Hook ProcessClearMemHook =
{
    { NULL, NULL}, AROS_ASMSYMNAME(ProcessClearMem), NULL, NULL
},
ProcessCheckFileTypeHook =
{
    { NULL, NULL}, AROS_ASMSYMNAME(ProcessCheckFileType), NULL, NULL
},
ProcessOldDrawerDataHook =
{
    { NULL, NULL}, AROS_ASMSYMNAME(ProcessOldDrawerData), NULL, NULL
},
ProcessNewDrawerDataHook =
{
    { NULL, NULL}, AROS_ASMSYMNAME(ProcessNewDrawerData), NULL, NULL
},
ProcessGadgetRenderHook =
{
    { NULL, NULL}, AROS_ASMSYMNAME(ProcessGadgetRender), NULL, NULL
},
ProcessSelectRenderHook =
{
    { NULL, NULL}, AROS_ASMSYMNAME(ProcessSelectRender), NULL, NULL
},
ProcessFlagPtrHook =
{
    { NULL, NULL}, AROS_ASMSYMNAME(ProcessFlagPtr), NULL, NULL
},
ProcessDefaultToolHook =
{
    { NULL, NULL}, AROS_ASMSYMNAME(ProcessDefaultTool), NULL, NULL
},
ProcessToolWindowHook =
{
    { NULL, NULL}, AROS_ASMSYMNAME(ProcessToolWindow), NULL, NULL
},
ProcessToolTypesHook =
{
    { NULL, NULL}, AROS_ASMSYMNAME(ProcessToolTypes), NULL, NULL
},
ProcessIcon35Hook =
{
    { NULL, NULL}, AROS_ASMSYMNAME(ProcessIcon35), NULL, NULL
};

/****************************************************************************************/

#undef O
#define O(x)    (offsetof (struct Gadget,x))

/****************************************************************************************/

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

/****************************************************************************************/

#undef O
#define O(x)    (offsetof (struct DiskObject,x))

/****************************************************************************************/

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

/****************************************************************************************/

#undef O
#define O(x)    (offsetof (struct Image,x))

/****************************************************************************************/

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

/****************************************************************************************/

const IPTR IconDesc[] =
{
    sizeof (struct NativeIcon),
    SDM_SPECIAL(0,&ProcessClearMemHook),
    SDM_STRUCT(0,DiskObjectDesc),
    SDM_SPECIAL(0,&ProcessCheckFileTypeHook),
    SDM_SPECIAL(0,&ProcessOldDrawerDataHook),
    SDM_SPECIAL(0,&ProcessGadgetRenderHook),
    SDM_SPECIAL(0,&ProcessSelectRenderHook),
    SDM_SPECIAL(0,&ProcessDefaultToolHook),
    SDM_SPECIAL(0,&ProcessToolTypesHook),
    SDM_SPECIAL(0,&ProcessToolWindowHook),
    SDM_SPECIAL(0,&ProcessNewDrawerDataHook),
    SDM_SPECIAL(0,&ProcessIcon35Hook),
    SDM_END
};

/****************************************************************************************/

#undef O
#define O(x)    (offsetof (struct NewWindow,x))

/****************************************************************************************/

const IPTR NewWindowDesc[] =
{
    sizeof (struct NewWindow),
    SDM_WORD(O(LeftEdge)),
    SDM_WORD(O(TopEdge)),
    SDM_WORD(O(Width)),
    SDM_WORD(O(Height)),
    SDM_UBYTE(O(DetailPen)),
    SDM_UBYTE(O(BlockPen)),
    SDM_ULONG(O(IDCMPFlags)),
    SDM_ULONG(O(Flags)),
    SDM_IGNORE(4+4+4+4+4), /* FirstGadget
	    +CheckMark
	    +Title
	    +Screen
	    +BitMap
	    */
    SDM_WORD(O(MinWidth)),
    SDM_WORD(O(MinHeight)),
    SDM_UWORD(O(MaxWidth)),
    SDM_UWORD(O(MaxHeight)),
    SDM_UWORD(O(Type)),
    SDM_END
};

/****************************************************************************************/

#undef O
#define O(x)    (offsetof (struct DrawerData,x))

/****************************************************************************************/

const IPTR OldDrawerDataDesc[] =
{
    sizeof (struct DrawerData),
    SDM_STRUCT(O(dd_NewWindow),NewWindowDesc),
    SDM_LONG(O(dd_CurrentX)),
    SDM_LONG(O(dd_CurrentY)),
    SDM_END
};

/****************************************************************************************/

AROS_UFH3(LONG, dosstreamhook,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(BPTR,          fh,   A2),
    AROS_UFHA(ULONG       *, msg,  A1)
)
{
    AROS_USERFUNC_INIT

    LONG rc = 0;

    switch (*msg)
    {
    case BEIO_READ:
	rc = FGetC (fh);
#if 0
kprintf ("dsh: Read: %02X\n", rc);
#endif

	break;

    case BEIO_WRITE:
	rc = FPutC (fh, ((struct BEIOM_Write *)msg)->Data);
	break;

    case BEIO_IGNORE:
	Flush (fh);

	rc = Seek (fh, ((struct BEIOM_Ignore *)msg)->Count, OFFSET_CURRENT);
#if 0
kprintf ("dsh: Skip %d\n", ((struct BEIOM_Ignore *)msg)->Count);
#endif
	break;

    }

    return rc;

    AROS_USERFUNC_EXIT
} /* dosstreamhook */

/****************************************************************************************/

#define DO(x)       ((struct DiskObject *)x)

/****************************************************************************************/

AROS_UFH3S(ULONG, ProcessClearMem,
    AROS_UFHA(struct Hook *,   hook, A0),
    AROS_UFHA(struct Hook *,   streamhook, A2),
    AROS_UFHA(struct SDData *, data, A1)
)
{
    AROS_USERFUNC_INIT
    
    if (data->sdd_Mode == SDV_SPECIALMODE_READ)
    {
    	memset(data->sdd_Dest, 0, sizeof(struct NativeIcon));
	
	NATIVEICON(DO(data->sdd_Dest))->iconbase = (struct IconBase *)streamhook->h_Data;
    }
    
    return TRUE;
    
    AROS_USERFUNC_EXIT
    
}

/****************************************************************************************/

AROS_UFH3S(ULONG, ProcessCheckFileType,
    AROS_UFHA(struct Hook *,   hook, A0),
    AROS_UFHA(struct Hook *,   streamhook, A2),
    AROS_UFHA(struct SDData *, data, A1)
)
{
    AROS_USERFUNC_INIT
    
    if (data->sdd_Mode == SDV_SPECIALMODE_READ)
    {
    	if ((DO(data->sdd_Dest)->do_Magic != WB_DISKMAGIC))
	{
	    return FALSE;
	}
    }
    
    return TRUE;
    
    AROS_USERFUNC_EXIT
    
}

/****************************************************************************************/

AROS_UFH3S(ULONG, ProcessOldDrawerData,
    AROS_UFHA(struct Hook *,   hook, A0),
    AROS_UFHA(struct Hook *,   streamhook, A2),
    AROS_UFHA(struct SDData *, data, A1)
)
{
    AROS_USERFUNC_INIT
#if 0
kprintf ("ProcessOldDrawerData\n");
#endif

/*    if (DO(data->sdd_Dest)->do_Type == WBDRAWER)
*/
    /* sba: all icons which have do_DrawerData set actually contain
     * also the drawer data */

    if (DO(data->sdd_Dest)->do_DrawerData)
    {
	switch (data->sdd_Mode)
	{
	case SDV_SPECIALMODE_READ:
	    return ReadStruct (streamhook
		, (APTR *)&(DO(data->sdd_Dest)->do_DrawerData)
		, data->sdd_Stream
		, OldDrawerDataDesc
	    );

	case SDV_SPECIALMODE_WRITE:
	    return WriteStruct (streamhook
		, DO(data->sdd_Dest)->do_DrawerData
		, data->sdd_Stream
		, OldDrawerDataDesc
	    );

	case SDV_SPECIALMODE_FREE:
	    FreeStruct (DO(data->sdd_Dest)->do_DrawerData
		, OldDrawerDataDesc
	    );
	    break;
	}
    }

    return TRUE;

    AROS_USERFUNC_EXIT
} /* ProcessOldDrawerData */

/****************************************************************************************/

static struct Image * ReadImage (struct Hook * streamhook, BPTR file)
{
    struct Image * image;
    ULONG	   size;
    ULONG	   t;

    if (!ReadStruct (streamhook, (APTR *)&image, file, ImageDesc))
	return NULL;

    /* Size of imagedata in bytes */
    size = ((image->Width + 15) >> 4) * image->Height * image->Depth * 2;

#if 0
kprintf ("ReadImage: %dx%dx%d (%d bytes)\n"
    , image->Width
    , image->Height
    , image->Depth
    , size
);
#endif

    if (size)
    {
	if (!(image->ImageData = AllocMem (size, MEMF_CHIP)) )
	{
	    FreeStruct (image, ImageDesc);
	    return NULL;
	}

	size >>= 1;

	for (t=0; t<size; t++)
	{
	    UWORD data;

	    if (!ReadWord (streamhook, &data, file))
		break;

	    image->ImageData[t] = AROS_WORD2BE(data);
    	}

	if (t != size)
	{
	    FreeStruct (image, ImageDesc);
	    return NULL;
	}
    }

    return image;
} /* ReadImage */

/****************************************************************************************/

static int WriteImage (struct Hook * streamhook, BPTR file,
	struct Image * image)
{
    ULONG size;
    ULONG t;

    if (!WriteStruct (streamhook, image, file, ImageDesc) )
	return FALSE;

    /* Get size in words */
    size = ((image->Width + 15) >> 4) * image->Height * image->Depth;

#if 0
kprintf ("WriteImage: %dx%dx%d (%d bytes)\n"
    , image->Width
    , image->Height
    , image->Depth
    , size*2
);
#endif

    for (t=0; t<size; t++)
    {
    	UWORD data = image->ImageData[t];

	if (!WriteWord (streamhook, AROS_WORD2BE(data), file))
	    break;
    }

    return (t == size);
} /* WriteImage */

/****************************************************************************************/

static void FreeImage (struct Image * image)
{
    ULONG size;

    /* Get size in bytes */
    size = ((image->Width + 15) >> 4) * image->Height * image->Depth * 2;

    if (size)
	FreeMem (image->ImageData, size);

    FreeStruct (image, ImageDesc);
} /* FreeImage */

/****************************************************************************************/

AROS_UFH3S(ULONG, ProcessGadgetRender,
    AROS_UFHA(struct Hook *,   hook, A0),
    AROS_UFHA(struct Hook *,   streamhook, A2),
    AROS_UFHA(struct SDData *, data, A1)
)
{
    AROS_USERFUNC_INIT

    struct Image * image;

#if 0
kprintf ("ProcessGadgetRender\n");
#endif

    switch (data->sdd_Mode)
    {
    case SDV_SPECIALMODE_READ:
	image = ReadImage (streamhook, data->sdd_Stream);

	if (!image)
	    return FALSE;

	DO(data->sdd_Dest)->do_Gadget.GadgetRender = image;

	break;

    case SDV_SPECIALMODE_WRITE:
	image = DO(data->sdd_Dest)->do_Gadget.GadgetRender;

	return WriteImage (streamhook, data->sdd_Stream, image);

    case SDV_SPECIALMODE_FREE:
	image = DO(data->sdd_Dest)->do_Gadget.GadgetRender;

	FreeImage (image);

	break;
    }

    return TRUE;

    AROS_USERFUNC_EXIT
} /* ProcessGadgetRender */

/****************************************************************************************/

AROS_UFH3S(ULONG, ProcessSelectRender,
    AROS_UFHA(struct Hook *,   hook, A0),
    AROS_UFHA(struct Hook *,   streamhook, A2),
    AROS_UFHA(struct SDData *, data, A1)
)
{
    AROS_USERFUNC_INIT

    struct Image * image;

#if 0
kprintf ("ProcessSelectRender\n");
#endif

    if (DO(data->sdd_Dest)->do_Gadget.Flags & GFLG_GADGHIMAGE)
    {
	switch (data->sdd_Mode)
	{
	case SDV_SPECIALMODE_READ:
	    image = ReadImage (streamhook, data->sdd_Stream);

	    if (!image)
		return FALSE;

	    DO(data->sdd_Dest)->do_Gadget.SelectRender = image;

	    break;

	case SDV_SPECIALMODE_WRITE:
	    image = DO(data->sdd_Dest)->do_Gadget.SelectRender;

	    return WriteImage (streamhook, data->sdd_Stream, image);

	case SDV_SPECIALMODE_FREE:
	    image = DO(data->sdd_Dest)->do_Gadget.SelectRender;

	    FreeImage (image);

	    break;
	}
    }

    return TRUE;

    AROS_USERFUNC_EXIT
} /* ProcessSelectRender */

/****************************************************************************************/

AROS_UFH3S(ULONG, ProcessFlagPtr,
    AROS_UFHA(struct Hook *,   hook, A0),
    AROS_UFHA(struct Hook *,   streamhook, A2),
    AROS_UFHA(struct SDData *, data, A1)
)
{
    AROS_USERFUNC_INIT

    LONG ptr;

    switch (data->sdd_Mode)
    {
    case SDV_SPECIALMODE_READ:
	if (FRead (data->sdd_Stream, &ptr, 1, 4) != 4)
	    return FALSE;

#if 0
kprintf ("ProcessFlagPtr: %08lx %ld\n", ptr);
#endif

	*((APTR *)data->sdd_Dest) = (APTR)(ptr != 0L);

	break;

    case SDV_SPECIALMODE_WRITE:
	if (*((APTR *)data->sdd_Dest))
	    ptr = 0xABADCAFEL;
	else
	    ptr = 0L;

	if (FWrite (data->sdd_Stream, &ptr, 1, 4) != 4)
	    return FALSE;

	break;

    case SDV_SPECIALMODE_FREE:
	break;
    }


    return TRUE;

    AROS_USERFUNC_EXIT
} /* ProcessFlagPtr */

/****************************************************************************************/

static STRPTR ReadIconString (struct Hook * streamhook, BPTR file)
{
    ULONG  len;
    STRPTR str;

    if (!ReadLong (streamhook, &len, file))
	return NULL;

    str = AllocMem (len, MEMF_ANY);

    if (!str)
	return NULL;

    if (FRead (file, str, len, 1) == EOF)
    {
	FreeMem (str, len);
	return NULL;
    }

#if 0
kprintf ("ReadIconString: \"%s\"\n", str);
#endif

    return str;
} /* ReadIconString */

/****************************************************************************************/

static int WriteIconString (struct Hook * streamhook, BPTR file, STRPTR str)
{
    ULONG len;

    len = strlen (str) + 1;

    if (!WriteLong (streamhook, len, file))
	return FALSE;

    return FWrite (file, str, len, 1) != EOF;
} /* WriteIconString */

/****************************************************************************************/

AROS_UFH3S(ULONG, ProcessDefaultTool,
    AROS_UFHA(struct Hook *,   hook, A0),
    AROS_UFHA(struct Hook *,   streamhook, A2),
    AROS_UFHA(struct SDData *, data, A1)
)
{
    AROS_USERFUNC_INIT

    STRPTR str;

#if 0
kprintf ("ProcessDefaultTool\n");
#endif

    if (DO(data->sdd_Dest)->do_DefaultTool)
    {
	switch (data->sdd_Mode)
	{
	case SDV_SPECIALMODE_READ:
	    str = ReadIconString (streamhook, data->sdd_Stream);

	    if (!str)
		return FALSE;

	    DO(data->sdd_Dest)->do_DefaultTool = str;

	    break;

	case SDV_SPECIALMODE_WRITE: {
	    str = DO(data->sdd_Dest)->do_DefaultTool;

	    WriteIconString (streamhook, data->sdd_Stream, str);

	    break; }

	case SDV_SPECIALMODE_FREE:
	    str = DO(data->sdd_Dest)->do_DefaultTool;

	    FreeMem (str, strlen (str)+1);

	    break;
	}
    }

    return TRUE;

    AROS_USERFUNC_EXIT
} /* ProcessDefaultTool */

/****************************************************************************************/

AROS_UFH3S(ULONG, ProcessToolWindow,
    AROS_UFHA(struct Hook *,   hook, A0),
    AROS_UFHA(struct Hook *,   streamhook, A2),
    AROS_UFHA(struct SDData *, data, A1)
)
{
    AROS_USERFUNC_INIT

    STRPTR str;

#if 0
kprintf ("ProcessToolWindow\n");
#endif

    if (DO(data->sdd_Dest)->do_ToolWindow)
    {
	switch (data->sdd_Mode)
	{
	case SDV_SPECIALMODE_READ:
	    str = ReadIconString (streamhook, data->sdd_Stream);

	    if (!str)
		return FALSE;

	    DO(data->sdd_Dest)->do_ToolWindow = str;

	    break;

	case SDV_SPECIALMODE_WRITE: {
	    str = DO(data->sdd_Dest)->do_ToolWindow;

	    WriteIconString (streamhook, data->sdd_Stream, str);

	    break; }

	case SDV_SPECIALMODE_FREE:
	    str = DO(data->sdd_Dest)->do_ToolWindow;

	    FreeMem (str, strlen (str)+1);

	    break;
	}
    }

    return TRUE;

    AROS_USERFUNC_EXIT
} /* ProcessToolWindow */

/****************************************************************************************/

AROS_UFH3S(ULONG, ProcessToolTypes,
    AROS_UFHA(struct Hook *,   hook, A0),
    AROS_UFHA(struct Hook *,   streamhook, A2),
    AROS_UFHA(struct SDData *, data, A1)
)
{
    AROS_USERFUNC_INIT

#if 0
kprintf ("ProcessToolTypes\n");
#endif

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
	    if (!ReadLong (streamhook, &count, data->sdd_Stream))
		return FALSE;

	    count = (count >> 2) - 1; /* How many entries */

	    ttarray = AllocMem ((count+1)*sizeof(STRPTR), MEMF_ANY);

#if 0
kprintf ("Read %d tooltypes (tt=%p)\n", count, ttarray);
#endif

	    for (t=0; t<count; t++)
	    {
		ttarray[t] = ReadIconString (streamhook, data->sdd_Stream);
#if 0
kprintf ("String %d=%p=%s\n", t, ttarray[t], ttarray[t]);
#endif

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

#if 0
kprintf ("Write %d tooltypes (%p)\n", count, ttarray);
#endif

	    size = (count+1)*4;

	    if (!WriteLong (streamhook, size, data->sdd_Stream))
		return FALSE;

	    for (t=0; t<count; t++)
	    {
#if 0
kprintf ("String %d=%p=%s\n", t, ttarray[t], ttarray[t]);
#endif
		if (!WriteIconString (streamhook, data->sdd_Stream, ttarray[t]))
		    return FALSE;
	    }

	    break; }

	case SDV_SPECIALMODE_FREE:
	    ttarray = (STRPTR *)DO(data->sdd_Dest)->do_ToolTypes;

#if 0
kprintf ("Free tooltypes (%p)\n", count, ttarray);
#endif

	    for (t=0; ttarray[t]; t++)
	    {
#if 0
kprintf ("String %d=%p=%s\n", t, ttarray[t], ttarray[t]);
#endif
		FreeMem (ttarray[t], strlen (ttarray[t])+1);
	    }

	    FreeMem (ttarray, (t+1)*sizeof(STRPTR));

	    break;
	}
    }
#if 0
    else
	kprintf ("No tool types\n");
#endif

    return TRUE;

    AROS_USERFUNC_EXIT
} /* ProcessToolTypes */

/****************************************************************************************/

AROS_UFH3S(ULONG, ProcessNewDrawerData,
    AROS_UFHA(struct Hook *,   hook, A0),
    AROS_UFHA(struct Hook *,   streamhook, A2),
    AROS_UFHA(struct SDData *, data, A1)
)
{
    AROS_USERFUNC_INIT

#if 0
kprintf ("ProcessNewDrawerData\n");
#endif

    if (DO(data->sdd_Dest)->do_DrawerData &&
    	((LONG)DO(data->sdd_Dest)->do_Gadget.UserData > 0) &&
	((LONG)DO(data->sdd_Dest)->do_Gadget.UserData <= WB_DISKREVISION))
    {
	switch (data->sdd_Mode)
	{
	case SDV_SPECIALMODE_READ:
	    if (!ReadLong(streamhook, &DO(data->sdd_Dest)->do_DrawerData->dd_Flags, data->sdd_Stream))
		return FALSE;

    	    if (!ReadWord(streamhook, &DO(data->sdd_Dest)->do_DrawerData->dd_ViewModes, data->sdd_Stream))
	    	return FALSE;
		
	    break;

	case SDV_SPECIALMODE_WRITE: 
	    if (!WriteLong(streamhook, DO(data->sdd_Dest)->do_DrawerData->dd_Flags, data->sdd_Stream))
		return FALSE;

    	    if (!WriteWord(streamhook, DO(data->sdd_Dest)->do_DrawerData->dd_ViewModes, data->sdd_Stream))
	    	return FALSE;
	    break;
	}
    }

    return TRUE;

    AROS_USERFUNC_EXIT
} /* ProcessNewDrawerData */

/****************************************************************************************/

#define IconBase ((IconBase_T *)(streamhook->h_Data))

/****************************************************************************************/

AROS_UFH3S(ULONG, ProcessIcon35,
    AROS_UFHA(struct Hook *,   hook, A0),
    AROS_UFHA(struct Hook *,   streamhook, A2),
    AROS_UFHA(struct SDData *, data, A1)
)
{
    AROS_USERFUNC_INIT
    
    IPTR retval = TRUE;

#if 0
kprintf ("ProcessIcon35\n");
#endif
    
    switch (data->sdd_Mode)
    {
    	case SDV_SPECIALMODE_READ:
	    ReadIcon35(NATIVEICON(DO(data->sdd_Dest)), streamhook, data->sdd_Stream, IconBase);
    	    break;	    

    	case SDV_SPECIALMODE_FREE:
	    FreeIcon35(NATIVEICON(DO(data->sdd_Dest)), (IconBase_T *)NATIVEICON(DO(data->sdd_Dest))->iconbase);
	    break;

    	case SDV_SPECIALMODE_WRITE:
	    retval = WriteIcon35(NATIVEICON(DO(data->sdd_Dest)), streamhook, data->sdd_Stream, IconBase);
    	    break;	    

    }
    
    return retval;
    
    AROS_USERFUNC_EXIT
} /* ProcessIcon35 */

/****************************************************************************************/

