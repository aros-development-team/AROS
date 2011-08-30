#include <dos/dosextens.h>
#include <exec/libraries.h>
#include <oop/ifmeta.h>
#include <oop/oop.h>

#include <proto/exec.h>
#include <proto/oop.h>

#include "compositing_intern.h"

/* Our global data */
struct Library *OOPBase;

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
    struct Process *me;

    /* 
     * Open oop.library manually because we don't want it to be
     * automatically close when we stay resident.
     */
    OOPBase = OpenLibrary("oop.library", 42);
    if (!OOPBase)
    	return RETURN_FAIL;

    if (OOP_FindClass(CLID_Hidd_Compositing))
    {
    	/* Double-starting is a valid operation. */
    	CloseLibrary(OOPBase);
    	return RETURN_OK;
    }

    if (!InitClass())
    {
    	CloseLibrary(OOPBase);
    	return RETURN_FAIL;
    }

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

    return RETURN_OK;
}
