/*
    (C) 1995-99 AROS - The Amiga Research OS
    $Id$

    Desc: Intuition function OpenWorkBench()
    Lang: english
*/

#include "intuition_intern.h"

#include <intuition/intuition.h>
#include <proto/intuition.h>

/*****************************************************************************

    NAME */

    AROS_LH0(ULONG, OpenWorkBench,

/*  SYNOPSIS */

/*  LOCATION */
    struct IntuitionBase *, IntuitionBase, 35, Intuition)

/*  FUNCTION
	Attempt to open the Workbench screen.

    INPUTS
	None.

    RESULT
	Tries to (re)open WorkBench screen. If successful return value
	is a pointer to the screen structure, which shouldn't be used,
	because other programs may close the WorkBench and make the
	pointer invalid.
	If this function fails the return value is NULL.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	CloseWorkBench()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    /* Open the Workbench screen if we don't have one. */
    if( GetPrivIBase(IntuitionBase)->WorkBench == NULL ) {
        UWORD pens[] = { ~0 };

        struct Screen *screen = NULL;

        struct TagItem screenTags[] = {
            { SA_Depth      , AROS_DEFAULT_WBDEPTH  },
            { SA_Type       , WBENCHSCREEN          },
            { SA_Title      , (IPTR)"Workbench"     },
            { SA_Width      , AROS_DEFAULT_WBWIDTH  },
            { SA_Height     , AROS_DEFAULT_WBHEIGHT },
            { SA_PubName    , (IPTR)"Workbench"     },
            { SA_Pens       , (IPTR) pens           },
            { SA_SharePens  , TRUE                  },
            { TAG_END       , 0                     }
        };

        screen = OpenScreenTagList( NULL, screenTags );

        if( screen ) {
            GetPrivIBase(IntuitionBase)->WorkBench = screen;

            /* Make the screen public. */
            PubScreenStatus( screen, 0 );

            /* Make the screen the default one. */
            SetDefaultPubScreen( NULL );
        } else {
            /* Maybe we should have a Alert() here? */
            return NULL;
        }
    }

    /* Tell the Workbench process to open it's windows, if there is one. */
    if( GetPrivIBase(IntuitionBase)->WorkBenchMP != NULL ) {
        struct MsgPort      replymp;
        struct IntuiMessage imsg;

        /* Setup our reply port. By doing this manually, we can use SIGB_SINGLE
         * and thus avoid allocating a signal (which may fail).*/
        memset( &replymp, 0, sizeof( replymp ) );

        replymp.mp_Node.ln_Type = NT_MSGPORT;
        replymp.mp_Flags        = PA_SIGNAL;
        replymp.mp_SigBit       = SIGB_SINGLE;
        replymp.mp_SigTask      = FindTask( NULL );
        NEWLIST( &replymp.mp_MsgList );

        /* Setup our message. */
    	imsg.ExecMessage.mn_ReplyPort = &replymp;	
        imsg.Class = IDCMP_WBENCHMESSAGE;
        imsg.Code  = WBENCHOPEN;

        /* Sends it to the handler and wait for the reply. */
        PutMsg( GetPrivIBase(IntuitionBase)->WorkBenchMP, (struct Message *) (&imsg) );
        WaitPort( &replymp );

        /* After leaving this block imsg and repymp will be automagically freed,
         * so we don't have to deallocate them ourselves. */
    }

    return (ULONG) GetPrivIBase(IntuitionBase)->WorkBench;

    AROS_LIBFUNC_EXIT
} /* OpenWorkBench */
