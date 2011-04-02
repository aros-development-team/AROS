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

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>

#include <aros/detach.h>


#include "windowdecorclass.h"
#include "screendecorclass.h"
#include "menudecorclass.h"
#include "newimage.h"
#include "config.h"

struct IClass *wndcl, *scrcl, *menucl;

STRPTR __detached_name = "Decoration";

#define MAGIC_PRIVATE_SKIN      0x0001

struct DefaultNewDecorator
{
    struct NewDecorator base;   /* MUST BE FIRST */
    struct DecorConfig * dc;
    struct DecorImages * di;
};

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
    if (((struct DefaultNewDecorator *)nd)->dc != NULL) FreeConfig(((struct DefaultNewDecorator *)nd)->dc);
    if (((struct DefaultNewDecorator *)nd)->di != NULL) FreeImages(((struct DefaultNewDecorator *)nd)->di);
    FreeVec(nd);
}

struct NewDecorator *GetDecorator(STRPTR path)
{
    struct NewDecorator *nd = NULL;
    struct DefaultNewDecorator * dnd = NULL;

    STRPTR newpath;

    if (path != NULL) newpath = path; else newpath = "Theme:";

    dnd = AllocVec(sizeof(struct DefaultNewDecorator), MEMF_CLEAR | MEMF_ANY);
    
    if (dnd)
    {
        nd = (struct NewDecorator *)dnd;

        dnd->dc = LoadConfig(newpath);
        if (!dnd->dc)
        {
            DeleteDecorator(nd);
            return NULL;
        }
        
        dnd->di = LoadImages(dnd->dc);
        
        if (!dnd->di)
        {
            DeleteDecorator(nd);
            return NULL;
        }

        {
            struct TagItem ScreenTags[] = 
            { 
                {SDA_UserBuffer, sizeof(struct ScreenData)},
                {SDA_DecorImages, (IPTR)dnd->di},
                {SDA_DecorConfig, (IPTR)dnd->dc},
                {TAG_DONE}
            };
            nd->nd_Screen = NewObjectA(scrcl, NULL, ScreenTags);
        }
        
        if (nd->nd_Screen)
        {
            struct TagItem WindowTags[] = 
            { 
                {WDA_UserBuffer, sizeof(struct WindowData)},
                {WDA_DecorImages, (IPTR)dnd->di},
                {WDA_DecorConfig, (IPTR)dnd->dc},
                {TAG_DONE} 
            };

            struct TagItem MenuTags[] = 
            { 
                {MDA_UserBuffer, sizeof(struct MenuData)},
                {MDA_DecorImages, (IPTR)dnd->di},
                {MDA_DecorConfig, (IPTR)dnd->dc},
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

    wndcl = MakeWindowDecorClass();
    if (wndcl)
    {

        scrcl = MakeScreenDecorClass();
        if (scrcl)
        {

		
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
