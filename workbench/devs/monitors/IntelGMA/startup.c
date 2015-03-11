/*
    Copyright © 2011-2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <dos/dosextens.h>
#include <hidd/graphics.h>
#include <hidd/hidd.h>
#include <workbench/startup.h>
#include <workbench/workbench.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/icon.h>
#include <proto/oop.h>

#include <stdlib.h>
#include <string.h>

#include "intelG45_intern.h"
#include "compositing_intern.h"
#include "gallium_intern.h"

struct Library *OOPBase;
struct Library *UtilityBase;
struct Library *StdCBase;
struct Library *StdCIOBase;

OOP_AttrBase HiddPCIDeviceAttrBase;
OOP_AttrBase HiddGMABitMapAttrBase;
OOP_AttrBase HiddI2CAttrBase;
OOP_AttrBase HiddI2CDeviceAttrBase;
OOP_AttrBase HiddGCAttrBase;
OOP_AttrBase HiddCompositingAttrBase;
OOP_AttrBase MetaAttrBase;
OOP_AttrBase HiddAttrBase;
OOP_AttrBase HiddPCIDeviceAttrBase;
OOP_AttrBase HiddGMABitMapAttrBase;
OOP_AttrBase HiddPixFmtAttrBase;
OOP_AttrBase HiddBitMapAttrBase;
OOP_AttrBase HiddColorMapAttrBase;
OOP_AttrBase HiddSyncAttrBase;
OOP_AttrBase HiddGfxAttrBase;
OOP_AttrBase __IHidd_PlanarBM;

/* 
 * Class static data is really static now. :)
 * If the driver would be compiled as a ROM resident, this structure 
 * needs to be allocated using AllocMem()
 */
struct g45staticdata sd;

static const struct OOP_ABDescr attrbases[] =
{
    {IID_Meta		    , &MetaAttrBase           },
    {IID_Hidd		    , &HiddAttrBase           },
    {IID_Hidd_PCIDevice     , &HiddPCIDeviceAttrBase  },
    {IID_Hidd_BitMap	    , &HiddBitMapAttrBase     },
    {IID_Hidd_PixFmt	    , &HiddPixFmtAttrBase     },
    {IID_Hidd_Sync	    , &HiddSyncAttrBase       },
    {IID_Hidd_Gfx	    , &HiddGfxAttrBase        },
    {IID_Hidd_IntelG45BitMap, &HiddGMABitMapAttrBase  },
    {IID_Hidd_I2C	    , &HiddI2CAttrBase        },
    {IID_Hidd_I2CDevice	    , &HiddI2CDeviceAttrBase  },
    {IID_Hidd_PlanarBM	    , &__IHidd_PlanarBM       },
    {IID_Hidd_GC	    , &HiddGCAttrBase         },
    {IID_Hidd_Compositing   , &HiddCompositingAttrBase},
    {NULL, NULL }
};

const TEXT version_string[] = "$VER: IntelGMA 3.9 (11.3.2015)\n";

extern struct WBStartup *WBenchMsg;
int __nocommandline = 1;

int main(void)
{
    BPTR olddir = BNULL;
    STRPTR myname;
    struct DiskObject *icon;
    struct RDArgs *rdargs = NULL;
    IPTR args[3] = {0};
    int ret = RETURN_FAIL;
    BOOL success = TRUE;

    /* 
     * Open libraries manually, otherwise they will be closed
     * when this subroutine exits. Driver needs them.
     */
    OOPBase = OpenLibrary("oop.library", 42);
    if (OOPBase == NULL)
        success = FALSE;

    /*
     * If our class is already registered, the user attempts to run us twice.
     * Just ignore this.
     */
    if (success)
    {
        if (OOP_FindClass(CLID_Hidd_Gfx_IntelG45))
        {
            success = FALSE;
            ret = RETURN_OK;
        }
    }

    UtilityBase = OpenLibrary("utility.library", 36);
    StdCBase = OpenLibrary("stdc.library", 0);
    StdCIOBase = OpenLibrary("stdcio.library", 0);
    if (UtilityBase == NULL || StdCBase == NULL || StdCIOBase == NULL)
        success = FALSE;

    if (success)
    {
        memset(&sd, 0, sizeof(sd));

        /* We don't open dos.library and icon.library manually because only startup code
           needs them and these libraries can be closed even upon successful exit */
        if (WBenchMsg)
        {
            olddir = CurrentDir(WBenchMsg->sm_ArgList[0].wa_Lock);
	    myname = WBenchMsg->sm_ArgList[0].wa_Name;
        }
        else
        {
	    struct Process *me = (struct Process *)FindTask(NULL);

	    if (me->pr_CLI)
	    {
                struct CommandLineInterface *cli = BADDR(me->pr_CLI);

                myname = AROS_BSTR_ADDR(cli->cli_CommandName);
            }
            else
                myname = me->pr_Task.tc_Node.ln_Name;
        }

        icon = GetDiskObject(myname);

        if (icon)
        {
            STRPTR str;

            str = FindToolType(icon->do_ToolTypes, "FORCEGMA");
            args[0] = str ? TRUE : FALSE;

            str = FindToolType(icon->do_ToolTypes, "GMA_MEM=");
            if (str)
                sd.memsize = atoi(str);

            str = FindToolType(icon->do_ToolTypes, "FORCEGALLIUM");
            args[2] = str ? TRUE : FALSE;
        }

        if (!WBenchMsg)
            rdargs = ReadArgs("FORCEGMA/S,GMA_MEM/N,FORCEGALLIUM/S", args, NULL);

        sd.forced  = args[0];
        if (args[1])
            sd.memsize = *((ULONG *)args[1]);
        sd.force_gallium  = args[2];

        if (rdargs)
            FreeArgs(rdargs);
        if (icon)
            FreeDiskObject(icon);
        if (olddir)
            CurrentDir(olddir);
    }

    /* Obtain attribute bases first */
    if (success)
    {
        if (!OOP_ObtainAttrBases(attrbases))
            success = FALSE;
    }

    if (success)
    {
	struct TagItem INTELG45_tags[] =
	{
	    {aMeta_SuperID	 , (IPTR)CLID_Hidd_Gfx         },
	    {aMeta_InterfaceDescr, (IPTR)INTELG45_ifdescr      },
	    {aMeta_InstSize	 , sizeof(struct g45data)      },
	    {aMeta_ID		 , (IPTR)CLID_Hidd_Gfx_IntelG45},
	    {TAG_DONE, 0}
	};

	/* Create classes */
	sd.IntelG45Class = OOP_NewObject(NULL, CLID_HiddMeta, INTELG45_tags);
	if (sd.IntelG45Class)
	{
	    struct TagItem GMABM_tags[] =
	    {
		{aMeta_SuperID       , (IPTR)CLID_Hidd_BitMap},
		{aMeta_InterfaceDescr, (IPTR)GMABM_ifdescr   },
		{aMeta_InstSize      , sizeof(GMABitMap_t)   },
		{TAG_DONE, 0}
	    };

	    /* According to a tradition, we store a pointer to static data in class' UserData */
	    sd.IntelG45Class->UserData = &sd;
	    
	    sd.BMClass = OOP_NewObject(NULL, CLID_HiddMeta, GMABM_tags);
	    if (sd.BMClass)
	    {
		struct TagItem INTELI2C_tags[] =
		{
		    {aMeta_SuperID       , (IPTR)CLID_Hidd_I2C   },
		    {aMeta_InterfaceDescr, (IPTR)INTELI2C_ifdescr},
		    {aMeta_InstSize      , sizeof(struct i2cdata)},
		    {TAG_DONE, 0}
		};

		sd.BMClass->UserData = &sd;

		sd.IntelI2C = OOP_NewObject(NULL, CLID_HiddMeta, INTELI2C_tags);
		if (sd.IntelI2C)
		{
		    struct TagItem Compositing_tags[] =
		    {
			{aMeta_SuperID       , (IPTR)CLID_Hidd},
			{aMeta_InterfaceDescr, (IPTR)Compositing_ifdescr},
			{aMeta_InstSize      , sizeof(struct HIDDCompositingData)},
			{TAG_DONE, 0}
		    };

		    sd.IntelI2C->UserData = &sd;

		    sd.compositingclass = OOP_NewObject(NULL, CLID_HiddMeta, Compositing_tags);
		    if (sd.compositingclass)
		    {

            #ifndef GALLIUM_SIMULATION
			/* Init internal stuff */
			if (G45_Init(&sd))
            #endif
			{
			    struct Process *me = (struct Process *)FindTask(NULL);

                #ifndef GALLIUM_SIMULATION
			    /* 
			     * Register our gfx class as public, we use it as a
			     * protection against double start
			     */
			    OOP_AddClass(sd.IntelG45Class);
                #endif

                /* Init Galliumclass */
                InitGalliumClass();

			    /* Everything is okay, stay resident and exit */

			    D(bug("[SDL] Staying resident, process 0x%p\n", me));
			    if (me->pr_CLI)
			    {
				struct CommandLineInterface *cli = BADDR(me->pr_CLI);

				cli->cli_Module = BNULL;
			    }
			    else  
				me->pr_SegList = BNULL;

			    /* 
			     * Note also that we don't close needed libraries and
			     * don't free attribute bases
			     */
			    return RETURN_OK;
			}
			OOP_DisposeObject((OOP_Object *)sd.compositingclass);
		    }
		    OOP_DisposeObject((OOP_Object *)sd.IntelI2C);
		}
	        OOP_DisposeObject((OOP_Object *)sd.BMClass);
	    }
	    OOP_DisposeObject((OOP_Object *)sd.IntelG45Class);
	}
	OOP_ReleaseAttrBases(attrbases);
    }

    if (StdCIOBase != NULL)
        CloseLibrary(StdCIOBase);
    if (StdCBase != NULL)
        CloseLibrary(StdCBase);
    if (UtilityBase != NULL)
        CloseLibrary(UtilityBase);
    if (OOPBase != NULL)
        CloseLibrary(OOPBase);

    return ret;
}
