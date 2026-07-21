/*
    Copyright  1995-2026, The AROS Development Team.
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
        Provides the screen/window/menu decoration classes, using
        decorator.library for configuration loading, rendering and
        compositing support.
        
    NOTES

        Must be launched before Wanderer - usually in the S:startup-sequence
        
    BUGS

    SEE ALSO

        IPREFS
        
    INTERNALS

******************************************************************************/

#define DEBUG 0
#include <aros/debug.h>

#include <clib/alib_protos.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/decorator.h>
#include <proto/decortheme.h>

#include <libraries/decorator.h>
#include <libraries/decortheme.h>

#include <aros/detach.h>

#include "screendecorclass.h"
#include "windowdecorclass.h"
#include "menudecorclass.h"

const TEXT version_string[] = "$VER: Decoration 2.0 (28.04.2024)";

STRPTR __detached_name = "Decorator";

struct Library *DecoratorBase = NULL;
struct Library *DecorThemeBase = NULL;

#define MAGIC_PRIVATE_SKIN              0x0001
#define MAGIC_PRIVATE_TITLECLASS	    0x0F0E
#define MAGIC_PRIVATE_TITLECHILD 	    0x0F0F

struct DecorationDecorator
{
    struct NewDecorator base;   /* MUST BE FIRST */
    struct DecorTheme * theme;
};

struct  DecorationDecorator *basedecor = NULL;

struct SkinMessage {
    struct MagicMessage msg;
    UWORD  class;
    STRPTR  path;
    STRPTR  id;
};

void DeleteDecorator(struct NewDecorator *nd)
{
    if (nd == NULL) return;
    if (((struct DecorationDecorator *)nd)->theme != NULL) DTFreeTheme(((struct DecorationDecorator *)nd)->theme);
    FreeVec(nd);
}

struct DecorationDecorator *LoadDecoration(STRPTR path)
{
    struct DecorationDecorator * dnd = NULL;
    struct TagItem *screenTags, *menuTags, *windowTags;

    D(bug("LoadDecoration: Entering\n"));

    STRPTR newpath;

    if (path != NULL) newpath = path; else newpath = "Theme:";

    dnd = AllocVec(sizeof(struct DecorationDecorator), MEMF_CLEAR | MEMF_ANY);
    
    D(bug("LoadDecoration: decorator @ 0x%p\n", dnd));

    if (dnd)
    {
        dnd->theme = DTLoadTheme(newpath);
        D(bug("LoadDecoration: theme @ 0x%p\n", dnd->theme));
        if (!dnd->theme)
        {
            DeleteDecorator(&dnd->base);
            return NULL;
        }

        dnd->base.nd_ScreenTags = AllocVec(sizeof(struct TagItem) * 5, MEMF_CLEAR | MEMF_ANY);
        dnd->base.nd_MenuTags = AllocVec(sizeof(struct TagItem) * 5, MEMF_CLEAR | MEMF_ANY);
        dnd->base.nd_WindowTags = AllocVec(sizeof(struct TagItem) * 5, MEMF_CLEAR | MEMF_ANY);

        if (basedecor)
        {
            D(bug("LoadDecoration: copying base classes\n"));
            dnd->base.nd_ScreenClass = basedecor->base.nd_ScreenClass;
            dnd->base.nd_MenuClass = basedecor->base.nd_MenuClass;
            dnd->base.nd_WindowClass = basedecor->base.nd_WindowClass;
        }

        screenTags = dnd->base.nd_ScreenTags;
        menuTags = dnd->base.nd_MenuTags;
        windowTags = dnd->base.nd_WindowTags;

        screenTags[0].ti_Tag = SDA_UserBuffer;
        menuTags[0].ti_Tag = MDA_UserBuffer;
        windowTags[0].ti_Tag = WDA_UserBuffer;
        screenTags[0].ti_Data = sizeof(struct ScreenData);
        menuTags[0].ti_Data = sizeof(struct MenuData);
        windowTags[0].ti_Data = sizeof(struct WindowData);

        screenTags[1].ti_Tag = SDA_DecorImages;
        menuTags[1].ti_Tag = MDA_DecorImages;
        windowTags[1].ti_Tag = WDA_DecorImages;
        screenTags[1].ti_Data = (IPTR)DTGetImages(dnd->theme);
        menuTags[1].ti_Data = (IPTR)DTGetImages(dnd->theme);
        windowTags[1].ti_Data = (IPTR)DTGetImages(dnd->theme);

        screenTags[2].ti_Tag =  SDA_DecorConfig;
        menuTags[2].ti_Tag = MDA_DecorConfig;
        windowTags[2].ti_Tag = WDA_DecorConfig;
        screenTags[2].ti_Data = (IPTR)DTGetConfig(dnd->theme);
        menuTags[2].ti_Data = (IPTR)DTGetConfig(dnd->theme);
        windowTags[2].ti_Data = (IPTR)DTGetConfig(dnd->theme);
        
        screenTags[3].ti_Tag = SDA_DecorTheme;
        menuTags[3].ti_Tag = MDA_DecorTheme;
        windowTags[3].ti_Tag = WDA_DecorTheme;
        screenTags[3].ti_Data = (IPTR)dnd->theme;
        menuTags[3].ti_Data = (IPTR)dnd->theme;
        windowTags[3].ti_Data = (IPTR)dnd->theme;

        screenTags[4].ti_Tag = TAG_DONE;
        menuTags[4].ti_Tag = TAG_DONE;
        windowTags[4].ti_Tag = TAG_DONE;

    }
    return dnd;
}

#define ARGUMENT_TEMPLATE "PATH,SCREENID=ID/K"

char usage[] =
    "Decoration:\n"
    "Arguments:\n";

int main(void)
{
    struct  NewDecorator *decor;
    struct MsgPort *port;

    IPTR rd_Args[] = {0, 0, };

    struct RDArgs *args, *newargs;

    newargs = (struct RDArgs*) AllocDosObject(DOS_RDARGS, NULL);

    if (newargs == NULL) return 0;

    newargs->RDA_ExtHelp = usage;
    newargs->RDA_Flags = 0;
    
    args = ReadArgs(ARGUMENT_TEMPLATE, rd_Args, newargs);

    if (args == NULL) {
        FreeDosObject (DOS_RDARGS, (APTR) newargs);
        return 0;
    }

    DecoratorBase = OpenLibrary("decorator.library", 0);
    if (!DecoratorBase)
    {
        D(bug("Decoration: Failed to open decorator.library\n"));
        FreeArgs(args);
        FreeDosObject (DOS_RDARGS, (APTR) newargs);
        return 0;
    }

    DecorThemeBase = OpenLibrary("decortheme.library", 0);
    if (!DecorThemeBase)
    {
        D(bug("Decoration: Failed to open decortheme.library\n"));
        CloseLibrary(DecoratorBase);
        FreeArgs(args);
        FreeDosObject (DOS_RDARGS, (APTR) newargs);
        return 0;
    }

    if ((port = CreateMsgPort()) == NULL)
    {
        CloseLibrary(DecorThemeBase);
        CloseLibrary(DecoratorBase);
        FreeArgs(args);
        FreeDosObject (DOS_RDARGS, (APTR) newargs);
        return 0;
    }

    Forbid();
    if (FindPort(__detached_name)) {
        struct SkinMessage msg;
        msg.msg.mn_ReplyPort = port;
        msg.msg.mn_Magic = MAGIC_PRIVATE_SKIN;
        msg.class = 0;
        msg.path = (STRPTR) rd_Args[0];
        msg.id = (STRPTR) rd_Args[1];
        PutMsg(FindPort(__detached_name), (struct Message *) &msg);
        WaitPort(port);
        GetMsg(port);
        Permit();
        DeleteMsgPort(port);
        FreeArgs(args);
        CloseLibrary(DecorThemeBase);
        CloseLibrary(DecoratorBase);
        return 0;
    }
    Permit();

    D(bug("Decoration: making classes...\n"));

    if ((basedecor = LoadDecoration((STRPTR) rd_Args[0])) != NULL)
    {
        decor = &basedecor->base;
        if ((basedecor->base.nd_ScreenClass = MakeScreenDecorClass()) != NULL)
        {
            if ((basedecor->base.nd_MenuClass = MakeMenuDecorClass()) != NULL)
            {
                if ((basedecor->base.nd_WindowClass = MakeWindowDecorClass()) != NULL)
                {
                    D(bug("Decoration: Classes made (screen @ 0x%p, menu @ 0x%p, win @ 0x%p)\n", basedecor->base.nd_ScreenClass, basedecor->base.nd_MenuClass, basedecor->base.nd_WindowClass));

                    ULONG  skinSignal = 1 << port->mp_SigBit;
                    port->mp_Node.ln_Name=__detached_name;
                    AddPort(port);

                    D(bug("Decoration: Port created and added\n"));

                    D(bug("Decoration: Got decorator\n"));

                    if (decor != NULL)
                    {
                        decor->nd_Pattern = (STRPTR) rd_Args[1];
                        decor->nd_Port = port;
                        ChangeDecoration(DECORATION_SET, decor);
                    }

                    D(bug("Before Detach()\n"));

                    Detach();

                    D(bug("After Detach()\n"));

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
                                    case MAGIC_PRIVATE_TITLECLASS:
                                        dmsg = (struct DecoratorMessage *) msg;
                                        if (decor) {
                                            dmsg->dm_Class = (IPTR)GetScreenChildGClass();
                                        }
                                        break;
                                    case MAGIC_PRIVATE_TITLECHILD:
                                        dmsg = (struct DecoratorMessage *) msg;
                                        if (decor)
                                        {
                                            SetAttrs((Object *)*((Object **)((IPTR)dmsg->dm_Class + (IPTR)decor->nd_ScreenObjOffset)), SDA_TitleChild, dmsg->dm_Object, TAG_DONE);
                                        }
                                        break;
                                    case MAGIC_PRIVATE_SKIN:
                                        decor = (struct NewDecorator *)LoadDecoration(msg->path);
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
                    FreeClass(decor->nd_WindowClass);
                }
                FreeClass(decor->nd_MenuClass);
            }
            FreeClass(decor->nd_ScreenClass);
        }
    }
    CloseLibrary(DecorThemeBase);
    CloseLibrary(DecoratorBase);
    FreeDosObject (DOS_RDARGS, (APTR) newargs);
    FreeArgs(args);
    return 0;
}
