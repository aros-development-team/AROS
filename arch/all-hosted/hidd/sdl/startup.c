/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0

#include <aros/debug.h>
#include <dos/dosextens.h>
#include <hidd/graphics.h>
#include <hidd/keyboard.h>
#include <hidd/mouse.h>
#include <hidd/hidd.h>
#include <graphics/gfxbase.h>
#include <workbench/startup.h>
#include <workbench/workbench.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/icon.h>
#include <proto/oop.h>

#include <string.h>

#include "sdl_intern.h"

/* SDL includes define main to SDL_main, bring it back */
#undef main

struct Library *OOPBase;
struct Library *UtilityBase;

OOP_AttrBase MetaAttrBase;
OOP_AttrBase HiddAttrBase;
OOP_AttrBase HiddPixFmtAttrBase;
OOP_AttrBase HiddBitMapAttrBase;
OOP_AttrBase HiddColorMapAttrBase;
OOP_AttrBase HiddSyncAttrBase;
OOP_AttrBase HiddGfxAttrBase;
OOP_AttrBase HiddSDLBitMapAttrBase;
OOP_AttrBase HiddMouseAB;
OOP_AttrBase HiddKbdAB;

static struct OOP_ABDescr attrbases[] = {
    { IID_Meta,           &MetaAttrBase          },
    { IID_Hidd,           &HiddAttrBase          },
    { IID_Hidd_PixFmt,    &HiddPixFmtAttrBase    },
    { IID_Hidd_BitMap,    &HiddBitMapAttrBase    },
    { IID_Hidd_ColorMap,  &HiddColorMapAttrBase  },
    { IID_Hidd_Sync,      &HiddSyncAttrBase      },
    { IID_Hidd_Gfx,       &HiddGfxAttrBase       },
    { IID_Hidd_SDLBitMap, &HiddSDLBitMapAttrBase },
    { IID_Hidd_Mouse,     &HiddMouseAB           },
    { IID_Hidd_Kbd,       &HiddKbdAB             },
    { NULL,               NULL                   }
};

/* Class static data is really static now. :)
   If the driver would be compiled as a ROM resident, this structure could
   be allocated either on stack or using AllocMem() */
struct sdlhidd xsd = {NULL};

static int sdl_Startup(struct sdlhidd *xsd)
{
    struct GfxBase *GfxBase;
    OOP_Object *kbd, *ms;
    ULONG err;

    xsd->mousehidd = NULL;
    D(bug("[SDL] Class initialization OK, creating objects\n"));

    /* Add keyboard and mouse driver to the system */
    kbd = OOP_NewObject(NULL, CLID_Hidd_Kbd, NULL);
    if (kbd) {
        ms = OOP_NewObject(NULL, CLID_Hidd_Mouse, NULL);
	if (ms) {
            xsd->kbdhidd = HIDD_Kbd_AddHardwareDriver(kbd, xsd->kbdclass, NULL);
	    if (xsd->kbdhidd) {
		xsd->mousehidd = HIDD_Mouse_AddHardwareDriver(ms, xsd->mouseclass, NULL);
		D(bug("[SDL] Mouse driver object 0x%p\n", xsd->mousehidd));
		if (!xsd->mousehidd)
		    HIDD_Kbd_RemHardwareDriver(kbd, xsd->mousehidd);
	    }
	    OOP_DisposeObject(ms);
	}    
	OOP_DisposeObject(kbd);
    }

    /* If we got no input, we can't work, fail */
    if (!xsd->mousehidd)
        return FALSE;

    /* Now proceed to adding display modes */
    GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 41);
    if (!GfxBase)
        return FALSE;

    /*
     * SDL is a singletone by design, so we can't create many SDL displays.
     * So only one object!
     */
    err = AddDisplayDriverA(xsd->gfxclass, NULL, NULL);

    CloseLibrary(&GfxBase->LibNode);
    return err ? FALSE : TRUE;
}

extern struct WBStartup *WBenchMsg;

int __nocommandline = 1;

int main(void)
{
    BPTR olddir = NULL;
    STRPTR myname;
    struct DiskObject *icon;
    struct RDArgs *rdargs = NULL;
    IPTR fullscreen = FALSE;
    int ret = RETURN_FAIL;

    /* Open libraries manually, otherwise they will be closed when this subroutine
       exits. Driver needs them. */
    OOPBase = OpenLibrary("oop.library", 42);
    if (!OOPBase)
	return RETURN_FAIL;

    /* If SDLGfx class is already registered, the user attempts to run us twice.
       Just ignore this. */
    if (OOP_FindClass(CLID_Hidd_SDLGfx)) {
        D(bug("[SDL] Driver already registered\n"));
	CloseLibrary(OOPBase);
        return RETURN_OK;
    }

    UtilityBase = OpenLibrary("utility.library", 36);
    if (!UtilityBase) {
	CloseLibrary(OOPBase);
	return RETURN_FAIL;
    }

    /* We don't open dos.library and icon.library manually because only startup code
       needs them and these libraries can be closed even upon successful exit */
    if (WBenchMsg) {
        olddir = CurrentDir(WBenchMsg->sm_ArgList[0].wa_Lock);
	myname = WBenchMsg->sm_ArgList[0].wa_Name;
    } else {
	struct Process *me = (struct Process *)FindTask(NULL);
    
	if (me->pr_CLI) {
            struct CommandLineInterface *cli = BADDR(me->pr_CLI);
	
	    myname = cli->cli_CommandName;
	} else
	    myname = me->pr_Task.tc_Node.ln_Name;
    }

    icon = GetDiskObject(myname);
    D(bug("[X11] Icon 0x%p\n", icon));

    if (icon) {
        STRPTR str;

	str = FindToolType(icon->do_ToolTypes, "FULLSCREEN");
	fullscreen = str ? TRUE : FALSE;
    }

    if (!WBenchMsg)
        rdargs = ReadArgs("FULLSCREEN/S", &fullscreen, NULL);

    xsd.use_fullscreen = fullscreen;
    if (rdargs)
        FreeArgs(rdargs);
    if (icon)
        FreeDiskObject(icon);
    if (olddir)
        CurrentDir(olddir);

    /* Obtain attribute bases first */
    if (OOP_ObtainAttrBases(attrbases)) {
	/* Load host libraries */
	if (sdl_hostlib_init(&xsd)) {
	    /* Create classes */
	    struct TagItem SDLGfx_tags[] = {
		{aMeta_SuperID       , (IPTR)CLID_Hidd_Gfx   },
		{aMeta_InterfaceDescr, (IPTR)SDLGfx_ifdescr  },
		{aMeta_InstSize      , sizeof(struct gfxdata)},
		{aMeta_ID            , (IPTR)CLID_Hidd_SDLGfx},
		{TAG_DONE            , 0                     }
	    };

	    xsd.gfxclass = OOP_NewObject(NULL, CLID_HiddMeta, SDLGfx_tags);
	    if (xsd.gfxclass) {
		struct TagItem SDLBitMap_tags[] = {
		    {aMeta_SuperID       , (IPTR)CLID_Hidd_BitMap },
		    {aMeta_InterfaceDescr, (IPTR)SDLBitMap_ifdescr},
		    {aMeta_InstSize      , sizeof(struct bmdata)  },
		    {TAG_DONE            , 0                      }
		};

		xsd.bmclass = OOP_NewObject(NULL, CLID_HiddMeta, SDLBitMap_tags);
	        if (xsd.bmclass) {
		    struct TagItem SDLMouse_tags[] = {
			{aMeta_SuperID       , (IPTR)CLID_Hidd         },
			{aMeta_InterfaceDescr, (IPTR)SDLMouse_ifdescr  },
			{aMeta_InstSize      , sizeof(struct mousedata)},
			{TAG_DONE            , 0                       }
		    };

		    xsd.mouseclass = OOP_NewObject(NULL, CLID_HiddMeta, SDLMouse_tags);
		    if (xsd.mouseclass) {
			struct TagItem SDLKbd_tags[] = {
			    {aMeta_SuperID       , (IPTR)CLID_Hidd       },
			    {aMeta_InterfaceDescr, (IPTR)SDLKbd_ifdescr  },
			    {aMeta_InstSize      , sizeof(struct kbddata)},
			    {TAG_DONE            , 0                     }
			};

			xsd.kbdclass = OOP_NewObject(NULL, CLID_HiddMeta, SDLKbd_tags);
			if (xsd.kbdclass) {
			    /* Init internal stuff */
			    sdl_keymap_init(&xsd);
			    if (sdl_event_init(&xsd)) {
			        if (sdl_hidd_init(&xsd)) {
				    if (sdl_Startup(&xsd)) {
					/* Register our gfx class as public, we use it as a
					   protection against double start */
				        OOP_AddClass(xsd.gfxclass);
					/* Everything is okay, stay resident and exit */
					struct Process *me = (struct Process *)FindTask(NULL);

					D(bug("[SDL] Staying resident, process 0x%p\n", me));
					if (me->pr_CLI) {
					    struct CommandLineInterface *cli = BADDR(me->pr_CLI);

					    D(bug("[SDL] CLI 0x%p\n", cli));
					    cli->cli_Module = NULL;
					} else  
					    me->pr_SegList = NULL;
					    
					/* Note also that we don't close needed libraries and
					   don't free attribute bases */
				        return RETURN_OK;
				    }
				    SV(SDL_Quit);
				}
			        sdl_event_expunge(&xsd);
			    }
			    OOP_DisposeObject((OOP_Object *)xsd.kbdclass);
			}
			OOP_DisposeObject((OOP_Object *)xsd.mouseclass);
		    }
		    OOP_DisposeObject((OOP_Object *)xsd.bmclass);
		}
	        OOP_DisposeObject((OOP_Object *)xsd.gfxclass);
	    }
	    sdl_hostlib_expunge(&xsd);
	} else
	    /* Perhaps some stupid user attempts to run this driver on
	       native system. Well, let's forgive him :) */
	    ret = RETURN_OK;
	OOP_ReleaseAttrBases(attrbases);
    }
    
    CloseLibrary(UtilityBase);
    CloseLibrary(OOPBase);
    return ret;
}
