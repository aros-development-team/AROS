/******************************************************************************
 *
 * COPYRIGHT: Unless otherwise noted, all files are Copyright (c) 1992-1999
 * Amiga, Inc.  All rights reserved.
 *
 * DISCLAIMER: This software is provided "as is".  No representations or
 * warranties are made with respect to the accuracy, reliability, performance,
 * currentness, or operation of this software, and all use is at your own risk.
 * Neither Amiga nor the authors assume any responsibility or liability
 * whatsoever with respect to your use of this software.
 *
 ******************************************************************************
 * dto.c
 * How to embed a DataType object within an Intuition window
 * Written by David N. Junod
 *
 */

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/libraries.h>
#include <dos/dos.h>
#include <dos/rdargs.h>
#include <datatypes/datatypes.h>
#include <datatypes/datatypesclass.h>
#include <datatypes/pictureclass.h>
#include <intuition/intuition.h>
#include <intuition/icclass.h>
#include <graphics/gfx.h>
#include <graphics/displayinfo.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <proto/alib.h>
#include <proto/datatypes.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/utility.h>

/*****************************************************************************/


struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
struct UtilityBase *UtilityBase;
struct Library *DataTypesBase;

/*****************************************************************************/

#define	TEMPLATE	"NAME/A"
#define	OPT_NAME	0
#define	NUM_OPTS	1

/*****************************************************************************/

#define	IDCMP_FLAGS	IDCMP_CLOSEWINDOW | IDCMP_VANILLAKEY | IDCMP_IDCMPUPDATE

/*****************************************************************************/

void PrintErrorMsg (ULONG errnum, STRPTR name)
{
    UBYTE errbuff[80];

    if (errnum >= DTERROR_UNKNOWN_DATATYPE)
	sprintf (errbuff, GetDTString (errnum), name);
    else
	Fault (errnum, NULL, errbuff, sizeof (errbuff));

    printf ("%s\nerror #%ld\n", errbuff, errnum);
}

/*****************************************************************************/

int main (int argc, char **argv)
{
    Object *dto;
    STRPTR name;

    ULONG modeid = INVALID_ID;
    ULONG nomwidth, nomheight;
    BOOL useScreen = FALSE;
    struct dtFrameBox dtf;
    struct FrameInfo fri;
    struct Screen *scr;
    struct Window *win;
    BOOL going = TRUE;

    struct IntuiMessage *imsg;
    ULONG sigr;

    const struct TagItem *tstate;
    struct TagItem *tag;
    struct TagItem *tags;
    ULONG tidata;
    ULONG errnum;

    ULONG options[NUM_OPTS];
    struct RDArgs *rdargs;

    /* Parse the arguments.  Note that this simple example assumes
     * that it was started from the shell. */
    memset (options, 0, sizeof (options));
    if ((rdargs = ReadArgs (TEMPLATE, (LONG *)options, NULL)))
    {
	/* Open DataTypes */
	if ((DataTypesBase = OpenLibrary ("datatypes.library", 39)))
	{
	    /* Open the other libraries */
	    IntuitionBase = (struct IntuitionBase *)OpenLibrary ("intuition.library", 39);
	    GfxBase = (struct GfxBase *)OpenLibrary ("graphics.library", 39);
	    UtilityBase = (struct UtilityBase *)OpenLibrary ("utility.library", 39);

	    /* Get a DataType object */
	    if ((dto = NewDTObject ((APTR) options[OPT_NAME], TAG_DONE, TAG_DONE)))
	    {
		/* Get information about the object */
		if (GetDTAttrs (dto,
				/* Get the name of the object */
				DTA_ObjName,		(IPTR) &name,

				/* Get the desired size */
				DTA_NominalHoriz,	(IPTR) &nomwidth,
				DTA_NominalVert,	(IPTR) &nomheight,

				/* Get the mode ID for graphical objects */
				PDTA_ModeID,		(IPTR) &modeid,
				TAG_DONE))
		{

		    /* Display any information we obtained */
		    if (name)
			printf ("opened \"%s\"\n", name);

		    /* Show the mode ID */
		    printf ("mode ID %08lx\n", modeid);

		    /* Display the nominal size */
		    printf ("nominal width %ld, height %ld\n", nomwidth, nomheight);
		}

		/* Ask the object what kind of environment it needs */
		memset (&dtf, 0, sizeof (struct dtFrameBox));
		memset (&fri, 0, sizeof (struct FrameInfo));
		dtf.MethodID = DTM_FRAMEBOX;
		dtf.dtf_FrameInfo = &fri;
		dtf.dtf_ContentsInfo = &fri;
		dtf.dtf_SizeFrameInfo = sizeof (struct FrameInfo);

		if (DoDTMethodA (dto, NULL, NULL, (Msg) &dtf) && fri.fri_Dimensions.Depth)
		{
		    printf ("PropertyFlags : 0x%lx\n", fri.fri_PropertyFlags);
		    printf ("RedBits       : 0x%x\n", fri.fri_RedBits);
		    printf ("GreenBits     : 0x%x\n", fri.fri_GreenBits);
		    printf ("BlueBits      : 0x%x\n", fri.fri_BlueBits);
		    printf ("Width         : %ld\n", (ULONG) fri.fri_Dimensions.Width);
		    printf ("Height        : %ld\n", (ULONG) fri.fri_Dimensions.Height);
		    printf ("Depth         : %ld\n", (ULONG) fri.fri_Dimensions.Depth);
		    printf ("Screen        : 0x%p\n", fri.fri_Screen);
		    printf ("ColorMap      : 0x%p\n", fri.fri_ColorMap);

		    if ((fri.fri_PropertyFlags & DIPF_IS_HAM) ||
			(fri.fri_PropertyFlags & DIPF_IS_EXTRAHALFBRITE))
		    {
			printf ("HAM or ExtraHalfBrite\n");
			useScreen = TRUE;
		    }

		    if ((fri.fri_PropertyFlags == 0) && (modeid & 0x800) && (modeid != INVALID_ID))
		    {
			printf ("ModeID=0x%08lx\n", modeid);
			useScreen = TRUE;
		    }
		}
		else
		    printf ("couldn't obtain environment information\n");

		if (useScreen)
		{
		    printf ("this object requires a private screen\n");
		}
		/* Get a lock on the default public screen */
		else if ((scr = LockPubScreen (NULL)))
		{
		    /* Make sure we have the dimensions */
		    nomwidth  = ((nomwidth) ? nomwidth : 600);
		    nomheight = ((nomheight) ? nomheight : 175);

		    /* Open the window */

		    if ((win = OpenWindowTags (NULL,
					      WA_InnerWidth,	nomwidth,
					      WA_InnerHeight,	nomheight,
					      WA_Title,		(IPTR) name,
					      WA_IDCMP,		IDCMP_FLAGS,
					      WA_DragBar,	TRUE,
					      WA_DepthGadget,	TRUE,
					      WA_CloseGadget,	TRUE,
					      WA_AutoAdjust,	TRUE,
					      WA_SimpleRefresh,	TRUE,
					      WA_BusyPointer,	TRUE,
					      WA_Activate,	TRUE,
					      WA_SizeGadget,	TRUE,
					      WA_SizeBBottom,	TRUE,
					      WA_MinWidth,	50,
					      WA_MinHeight,	50,
					      WA_MaxWidth,	10000,
					      WA_MaxHeight,	10000,
					      TAG_DONE)))

		    {

			/* Set the dimensions of the DataType object. */
			SetDTAttrs (dto, NULL, NULL,
				    GA_Left,	win->BorderLeft,
				    GA_Top,	win->BorderTop,
				    GA_RelWidth,	- win->BorderLeft - win->BorderRight,
				    GA_RelHeight,	- win->BorderTop - win->BorderBottom,
				    ICA_TARGET,	ICTARGET_IDCMP,
				    TAG_DONE);

			/* Add the object to the window */
			AddDTObject (win, NULL, dto, -1);

			/* Refresh the DataType object */
			RefreshDTObjectA (dto, win, NULL, NULL);

			/* Keep going until we're told to stop */
			while (going)
			{
			    /* Wait for an event */
			    sigr = Wait ((1L << win->UserPort->mp_SigBit) | SIGBREAKF_CTRL_C);

			    /* Did we get a break signal */
			    if (sigr & SIGBREAKF_CTRL_C)
				going = FALSE;

			    /* Pull Intuition messages */
			    while ((imsg = (struct IntuiMessage *) GetMsg (win->UserPort)))
			    {
				/* Handle each message */
				switch (imsg->Class)
				{
				    case IDCMP_CLOSEWINDOW:
					going = FALSE;
					break;

				    case IDCMP_VANILLAKEY:
					switch (imsg->Code)
					{
					    case 'Q':
					    case 'q':
					    case  27:
						going = FALSE;
						break;
					}
					break;

				    case IDCMP_IDCMPUPDATE:
					tstate = tags = (struct TagItem *) imsg->IAddress;
					while ((tag = NextTagItem(&tstate)))
					{
					    tidata = tag->ti_Data;
					    switch (tag->ti_Tag)
					    {
						/* Change in busy state */
						case DTA_Busy:
						    if (tidata)
							SetWindowPointer (win, WA_BusyPointer, TRUE, TAG_DONE);
						    else
							SetWindowPointer (win, WA_Pointer, NULL, TAG_DONE);
						    break;

						/* Error message */
						case DTA_ErrorLevel:
						    if (tidata)
						    {
							errnum = GetTagData (DTA_ErrorNumber, 0, tags);
							PrintErrorMsg (errnum, (STRPTR) options[OPT_NAME]);
						    }
						    break;

						/* Time to refresh */
						case DTA_Sync:
						    /* Refresh the DataType object */
						    RefreshDTObjectA (dto, win, NULL, NULL);
						    break;
					    }
					}
					break;
				}

				/* Done with the message, so reply to it */
				ReplyMsg ((struct Message *) imsg);
			    }
			}

			/* Remove the object from the window */
			RemoveDTObject (win, dto);

			/* Close the window now */
			CloseWindow (win);
		    }
		    else
			printf ("couldn't open window\n");

		    /* Unlock the screen */
		    UnlockPubScreen (NULL, scr);
		}
		else
		    printf ("couldn't lock default public screen\n");

		/* Dispose of the DataType object */
		DisposeDTObject (dto);
	    }
	    else
		PrintErrorMsg (IoErr (), (STRPTR) options[OPT_NAME]);

	    /* Close the libraries */
	    CloseLibrary ((struct Library *)UtilityBase);
	    CloseLibrary ((struct Library *)GfxBase);
	    CloseLibrary ((struct Library *)IntuitionBase);
	    CloseLibrary (DataTypesBase);
	}
	else
	    printf ("couldn't open datatypes.library\n");

	FreeArgs (rdargs);
    }
    else
	PrintErrorMsg (IoErr (), NULL);
    
    return 0;
}
