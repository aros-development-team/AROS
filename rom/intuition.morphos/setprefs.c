/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/graphics.h>
#include "intuition_intern.h"
#include <intuition/preferences.h>
#include <devices/input.h>
#include <devices/inputevent.h>
#include <stddef.h>

/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>

AROS_LH3(struct Preferences *, SetPrefs,

         /*  SYNOPSIS */
         AROS_LHA(struct Preferences * , prefbuffer, A0),
         AROS_LHA(LONG                 , size, D0),
         AROS_LHA(BOOL                 , inform, D1),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 54, Intuition)

/*  FUNCTION
    Sets the current Preferences structure.
 
    INPUTS
    prefbuffer - The buffer which contains your settings for the
        preferences.
    size - The number of bytes of the buffer you want to be copied.
    inform - If TRUE, all windows with IDCMP_NEWPREFS IDCMPFlags set
        get an IDCMP_NEWPREFS message.
 
    RESULT
    Returns your parameter buffer.
 
    NOTES
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
    GetDefPrefs(), GetPrefs()
 
    INTERNALS
 
    HISTORY
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    DEBUG_SETPREFS(dprintf("SetPrefs: Buffer 0x%lx Size 0x%lx Inform %d\n",
                           (ULONG) prefbuffer, size, inform));
    if (size > 0 && NULL != prefbuffer)
    {
        ULONG lock = LockIBase(0);
        BOOL  changepointer = FALSE;

        if (size > offsetof(struct Preferences, PointerMatrix))
        {
            if (memcmp(&prefbuffer->PointerMatrix,&GetPrivIBase(IntuitionBase)->ActivePreferences->PointerMatrix,POINTERSIZE) != 0)
                changepointer = TRUE;
        }

        CopyMem(prefbuffer,
                GetPrivIBase(IntuitionBase)->ActivePreferences,
                size <= sizeof(struct Preferences) ? size : sizeof(struct Preferences));

        UnlockIBase(lock);

        DEBUG_SETPREFS(dprintf("SetPrefs: DoubleClick %ld.%ld\n",
                               GetPrivIBase(IntuitionBase)->ActivePreferences->DoubleClick.tv_secs,
                               GetPrivIBase(IntuitionBase)->ActivePreferences->DoubleClick.tv_micro));

        if (GetPrivIBase(IntuitionBase)->InputIO)
        {
            struct timerequest req =
            {
                {{{0}, 0}, 0},
                {0}
            };

            if (size > offsetof(struct Preferences, KeyRptDelay))
            {
    	    #ifdef __MORPHOS__
                /* No need to setup a reply port, this command is guaranteed to support
                 * quick I/O.
                 */
    	    #else
                struct MsgPort *port = CreateMsgPort();

                if (port)
                {
                    req.tr_node.io_Message.mn_ReplyPort = port;

    	    #endif
                    DEBUG_SETPREFS(dprintf("SetPrefs: KeyRptDelay %ld secs micros %ld\n",
                                           GetPrivIBase(IntuitionBase)->ActivePreferences->KeyRptDelay.tv_secs,
                                           GetPrivIBase(IntuitionBase)->ActivePreferences->KeyRptDelay.tv_micro));
                    req.tr_node.io_Device = GetPrivIBase(IntuitionBase)->InputIO->io_Device;
                    req.tr_node.io_Unit = GetPrivIBase(IntuitionBase)->InputIO->io_Unit;
                    req.tr_node.io_Command = IND_SETTHRESH;
                    req.tr_time = GetPrivIBase(IntuitionBase)->ActivePreferences->KeyRptDelay;
                    DoIO(&req.tr_node);

    	    #ifndef __MORPHOS__
                    DeleteMsgPort(port);
                }
    	    #endif
            }
    
            if (size > offsetof(struct Preferences, KeyRptSpeed))
            {
    	    #ifdef __MORPHOS__
                /* No need to setup a reply port, this command is guaranteed to support
                 * quick I/O.
                 */
    	    #else
                struct MsgPort *port = CreateMsgPort();
    
                if (port)
                {
                    req.tr_node.io_Message.mn_ReplyPort = port;
    	    #endif

                DEBUG_SETPREFS(dprintf("SetPrefs: KeyRptSpeed secs %ld micros %ld\n",
                                       GetPrivIBase(IntuitionBase)->ActivePreferences->KeyRptSpeed.tv_secs,
                                       GetPrivIBase(IntuitionBase)->ActivePreferences->KeyRptSpeed.tv_micro));
    
                req.tr_node.io_Device = GetPrivIBase(IntuitionBase)->InputIO->io_Device;
                req.tr_node.io_Unit = GetPrivIBase(IntuitionBase)->InputIO->io_Unit;
                req.tr_node.io_Command = IND_SETPERIOD;
                req.tr_time = GetPrivIBase(IntuitionBase)->ActivePreferences->KeyRptSpeed;
                DoIO(&req.tr_node);
    
    	    #ifndef __MORPHOS__
                DeleteMsgPort(port);
                }
    	    #endif
            }
        } 
        else 
        {
            DEBUG_SETPREFS(dprintf("SetPrefs: no InputIO..don't set Key prefs\n"));
        }

        //#ifndef __MORPHOS__
        if (size > offsetof(struct Preferences, PointerMatrix) && changepointer)
        {
            Object *pointer = MakePointerFromPrefs(IntuitionBase, GetPrivIBase(IntuitionBase)->ActivePreferences);
            if (pointer)
            {
                InstallPointer(IntuitionBase, &GetPrivIBase(IntuitionBase)->DefaultPointer, pointer);
            }
        }
        //#endif
    
        /*
        ** If inform == TRUE then notify all windows that want to know about
        ** an update on the preferences.
        ** Do that by creating an inputevent, that will be handled by our
        ** handler and converted to idcmp messages, as well as by all other
        ** input handlers (not sure it should be that way, but that shouldn't
        ** do any harm).
        */
    
        if (inform)
        {
            struct MsgPort *port = CreateMsgPort();
    
            DEBUG_SETPREFS(dprintf("SetPrefs: Send NEWPREFS event\n"));
    
            if (port)
            {
                struct InputEvent ie;
                struct IOStdReq   req;
    
                ie.ie_NextEvent     = NULL;
                ie.ie_Class 	    = IECLASS_NEWPREFS;
                ie.ie_SubClass      = 0;
                ie.ie_Code  	    = 0;
                ie.ie_Qualifier     = 0;
                ie.ie_EventAddress  = NULL;
    
                req.io_Message.mn_ReplyPort 	= port;
                req.io_Device 	    	    	= GetPrivIBase(IntuitionBase)->InputIO->io_Device;
                req.io_Unit 	    	    	= GetPrivIBase(IntuitionBase)->InputIO->io_Unit;
                req.io_Command      	    	= IND_WRITEEVENT;
                req.io_Length 	    	    	= sizeof(ie);
                req.io_Data 	    	    	= &ie;
    
                DoIO((struct IORequest *)&req);
    
                DeleteMsgPort(port);
            }
        }
    }

    #warning Is there any further immediate action to be taken when the prefences are updated?

    return (struct Preferences *) prefbuffer;

    AROS_LIBFUNC_EXIT
} /* SetPrefs() */
