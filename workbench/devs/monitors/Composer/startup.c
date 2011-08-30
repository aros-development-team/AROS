#include <dos/dosextens.h>
#include <exec/libraries.h>
#include <oop/ifmeta.h>
#include <oop/oop.h>

#include <proto/exec.h>
#include <proto/oop.h>

#include "compositing_intern.h"

/* Libraries */
struct Library *OOPBase;
struct Library *UtilityBase;

/* Attr bases */
OOP_AttrBase HiddPixFmtAttrBase;
OOP_AttrBase HiddSyncAttrBase;
OOP_AttrBase HiddBitMapAttrBase;
OOP_AttrBase HiddGCAttrBase;
OOP_AttrBase HiddCompositingAttrBase;

static const struct OOP_ABDescr attrbases[] = 
{
    { IID_Hidd_PixFmt,          &HiddPixFmtAttrBase },
    { IID_Hidd_Sync,            &HiddSyncAttrBase },
    { IID_Hidd_BitMap,          &HiddBitMapAttrBase },
    { IID_Hidd_GC,              &HiddGCAttrBase },
    { IID_Hidd_Compositing,     &HiddCompositingAttrBase },
    { NULL, NULL }
};

static OOP_Class *InitClass(void)
{
    OOP_AttrBase MetaAttrBase = OOP_ObtainAttrBase(IID_Meta);
    OOP_Class *cl = NULL;

    struct TagItem Compositing_tags[] =
    {
        {aMeta_SuperID	     , (IPTR)CLID_Root			 },
        {aMeta_InterfaceDescr, (IPTR)Compositing_ifdescr	 },
        {aMeta_InstSize	     , sizeof(struct HIDDCompositingData)},
        {aMeta_ID	     , (IPTR)CLID_Hidd_Compositing	 },
        {TAG_DONE	     , 0				 }
    };

    if (MetaAttrBase == 0)
        return FALSE;

    cl = OOP_NewObject(NULL, CLID_HiddMeta, Compositing_tags);
    if (cl)
    	OOP_AddClass(cl);

    OOP_ReleaseAttrBase(IID_Meta);
    return cl;
}

int __nocommandline = 1;

int main(void)
{
    int ret = RETURN_FAIL;
    struct Process *me;

    /* 
     * Open oop.library manually because we don't want it to be
     * automatically close when we stay resident.
     */
    UtilityBase = OpenLibrary("utility.library", 0);
    if (UtilityBase)
    {
    	OOPBase = OpenLibrary("oop.library", 42);
	if (OOPBase)
	{
	    if (OOP_ObtainAttrBases(attrbases))
	    {
    	    	if (OOP_FindClass(CLID_Hidd_Compositing))
    	    	{
	    	    /* Double-starting is a valid operation. */
	    	    ret = RETURN_OK;
	    	}
	    	else
	    	{
    		    if (InitClass())
    		    {
		    	/* TODO: Tell graphics.library that we are up and running */

		    	/* Stay resident */
		    	me = (struct Process *)FindTask(NULL);
		    	if (me->pr_CLI)
		    	{
			    struct CommandLineInterface *cli = BADDR(me->pr_CLI);

			    cli->cli_Module = BNULL;
		    	}
		    	else
			    me->pr_SegList = BNULL;

		    	/* Don't close our libraries. The HIDD needs them. */

		    	return RETURN_OK;
		    }
		}
	    }
	    OOP_ReleaseAttrBases(attrbases);
    	    CloseLibrary(OOPBase);
    	}
    	CloseLibrary(UtilityBase);
    }

    return ret;
}
