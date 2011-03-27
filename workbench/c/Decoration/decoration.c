/*
    Copyright  1995-2011, The AROS Development Team.
    $Id$
*/

/******************************************************************************

    NAME

        Decoration

    SYNOPSIS

        (N/A)

    LOCATION

        C:

    FUNCTION

        Allows user definable skins for the intuition windows, menus and gadgets.
    	
    NOTES

        Must be launched before Wanderer - usually in the S:startup-sequence
	
    BUGS

    SEE ALSO

	IPREFS
	
    INTERNALS

******************************************************************************/

#include <clib/alib_protos.h> /* TODO: remove, needed for HOOKFUNC */

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>

#include <libraries/mui.h> /* TODO: REMOVE needed for get() */


#include <aros/detach.h>


#include "windowdecorclass.h"
#include "screendecorclass.h"
#include "menudecorclass.h"
#include "newimage.h"

struct IClass *wndcl, *scrcl, *menucl;

STRPTR __detached_name = "Decoration";

#define MAGIC_PRIVATE_SKIN      0x0001

struct SkinMessage {
    struct MagicMessage msg;
    UWORD  class;
    STRPTR  path;
    STRPTR  id;
};

void DeleteDecorator(struct NewDecorator *nd)
{
    if (nd == NULL) return;
    if (nd->nd_Menu != NULL) DisposeObject(nd->nd_Menu);
    if (nd->nd_Window != NULL) DisposeObject(nd->nd_Window);
    if (nd->nd_Screen != NULL) DisposeObject(nd->nd_Screen);
    FreeVec(nd);
}

struct NewDecorator *GetDecorator(STRPTR path)
{
    struct NewDecorator *nd = NULL;

    STRPTR newpath;

    if (path != NULL) newpath = path; else newpath = "Theme:";

    struct TagItem ScreenTags[] = { {SDA_UserBuffer, sizeof(struct ScreenData)}, {SDA_Configuration, (IPTR) newpath}, {TAG_DONE} };


    nd = AllocVec(sizeof(struct NewDecorator), MEMF_CLEAR | MEMF_ANY);
    if (nd)
    {
        nd->nd_Screen = NewObjectA(scrcl, NULL, ScreenTags);

        if (nd->nd_Screen)
        {
            APTR screendata = NULL;
            APTR decorimages = NULL;

            get(nd->nd_Screen, SDA_ScreenData, &screendata);
            get(nd->nd_Screen, SDA_DecorImages, &decorimages);

            struct TagItem WindowTags[] = { {WDA_UserBuffer, sizeof(struct WindowData)}, {WDA_Configuration, (IPTR) newpath}, {WDA_ScreenData, (IPTR)screendata}, {TAG_DONE} };
            struct TagItem MenuTags[] = 
            { 
                {MDA_UserBuffer, sizeof(struct MenuData)}, 
                {MDA_Configuration, (IPTR) newpath}, 
                {MDA_DecorImages, (IPTR)decorimages}, 
                {TAG_DONE} 
            };


            nd->nd_Window = NewObjectA(wndcl, NULL, WindowTags);
            nd->nd_Menu = NewObjectA(menucl, NULL, MenuTags);
            if ((nd->nd_Menu == NULL ) || (nd->nd_Window == NULL) || (nd->nd_Screen == NULL))
            {
                DeleteDecorator(nd);
                nd = NULL;
            }
        }
        else
        {
            DeleteDecorator(nd);
            nd = NULL;
        }
    }
    return nd;
}

#define ARGUMENT_TEMPLATE "PATH,SCREENID=ID/K"

char usage[] =
    "Decoration:\n"
    "Arguments:\n";

int main(void)
{

    IPTR rd_Args[] = {0, 0, };

    struct RDArgs *args, *newargs;

    /* the 1M $$ Question why "Decoration ?" does not work in the shell? */
    
    newargs = (struct RDArgs*) AllocDosObject(DOS_RDARGS, NULL);

    if (newargs == NULL) return 0;

    newargs->RDA_ExtHelp = usage;
    newargs->RDA_Flags = 0;
    
    args = ReadArgs(ARGUMENT_TEMPLATE, rd_Args, newargs);

    if (args == NULL) {
        FreeDosObject (DOS_RDARGS, (APTR) newargs);
        return 0;
    }

    Forbid();
    if (FindPort("DECORATIONS")) {
        struct MsgPort *port = CreateMsgPort();
        if (port) {
            struct SkinMessage msg;
            msg.msg.mn_ReplyPort = port;
            msg.msg.mn_Magic = MAGIC_PRIVATE_SKIN;
            msg.class = 0;
            msg.path = (STRPTR) rd_Args[0];
            msg.id = (STRPTR) rd_Args[1];
            PutMsg(FindPort("DECORATIONS"), (struct Message *) &msg);
            WaitPort(port);
            GetMsg(port);
            Permit();
            DeleteMsgPort(port);
            FreeArgs(args);
            return 0;
        }
    }
    Permit();

    wndcl = MakeClass(NULL, WINDECORCLASS, NULL, sizeof(struct windecor_data), 0);
    if (wndcl)
    {
        wndcl->cl_Dispatcher.h_Entry    = HookEntry;
        wndcl->cl_Dispatcher.h_SubEntry = (HOOKFUNC)WinDecor_Dispatcher;

        scrcl = MakeClass(NULL, SCRDECORCLASS, NULL, sizeof(struct scrdecor_data), 0);
        if (scrcl)
        {
            scrcl->cl_Dispatcher.h_Entry    = HookEntry;
            scrcl->cl_Dispatcher.h_SubEntry = (HOOKFUNC)ScrDecor_Dispatcher;
		
            menucl = MakeMenuDecorClass();
            if (menucl)
            {
                struct MsgPort *port = CreateMsgPort();
                ULONG  skinSignal;
                if (port)
                {
                    skinSignal = 1 << port->mp_SigBit;
                    port->mp_Node.ln_Name="DECORATIONS";
                    AddPort(port);

                    struct  NewDecorator *decor = GetDecorator((STRPTR) rd_Args[0]);

                    if (decor != NULL)
                    {
                        decor->nd_Pattern = (STRPTR) rd_Args[1];
                        decor->nd_Port = port;
                        ChangeDecoration(DECORATION_SET, decor);
                    }

                    Detach();

                    BOOL running = TRUE;

                    while (running)
                    {
                        ULONG sigs = Wait(SIGBREAKF_CTRL_C | skinSignal);
                        if ((sigs & SIGBREAKF_CTRL_C) != 0)
                        {
                            //       running = FALSE; /* at the moment no exit */
                        }
                        else if ((sigs & skinSignal) != 0)
                        {
                            struct DecoratorMessage *dmsg;
                            struct SkinMessage * msg = (struct SkinMessage *) GetMsg(port);
                            while (msg)
                            {
                                switch(msg->msg.mn_Magic)
                                {
                                    case MAGIC_PRIVATE_SKIN:
                                        decor = GetDecorator(msg->path);
                                        if (decor != NULL)
                                        {
                                            decor->nd_Pattern = msg->id;
                                            decor->nd_Port = port;
                                            ChangeDecoration(DECORATION_SET, decor);
                                        }
                                        break;
                                    case MAGIC_DECORATOR:
                                        dmsg = (struct DecoratorMessage *) msg;
                                        DeleteDecorator((struct NewDecorator *) dmsg->dm_Object);
                                        break;
                                }
                                ReplyMsg((struct Message *) msg);
                                msg = (struct SkinMessage *) GetMsg(port);
                            }
                        }
                    }
                }
                FreeClass(menucl);
            }
            FreeClass(scrcl);
        }
        FreeClass(wndcl);
    }
    FreeDosObject (DOS_RDARGS, (APTR) newargs);
    FreeArgs(args);
    return 0;
}
