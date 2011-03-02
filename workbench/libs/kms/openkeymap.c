#include <libraries/kms.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include <string.h>

#include "kms_intern.h"

/*****************************************************************************

    NAME */
#include <proto/kms.h>

	AROS_LH1(struct KeyMapNode *, OpenKeymap,

/*  SYNOPSIS */
	AROS_LHA(STRPTR, name, A0),

/*  LOCATION */
	struct KMSLibrary *, KMSBase, 1, Kms)

/*  FUNCTION
	Open a keymap by name.

    INPUTS
	name - Keymap name. Can be a full pathname or just a name.
	       In the latter case DEVS:Keymaps is assumed to be a
	       path to the file.

    RESULT
    	A pointer to the loaded keymap.

    NOTES
	This function automatically keeps track of loaded keymaps
	via keymap.resource. No more than a single copy of the keymap
	will be loaded.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/	
{
    AROS_LIBFUNC_INIT

    struct KeyMapNode *kmn, *kmn2;
    ULONG buflen = 0;
    STRPTR km_name;
    BPTR km_seg;
    struct KeyMapResource *kmr = ((struct kms_base *)KMSBase)->kmr;

    km_name  = FilePart(name);
    if (km_name == name)
    {
	/*
	 * A short name was given.
	 * Check if the keymap is already resident.
	 * Unfortunately we still have to use Forbid()/Permit() locking
	 * because there can be lots of software which does the same.
	 * AmigaOS(tm) never provided a centralized semaphore for this.
	 */
	Forbid();
	kmn = (struct KeyMapNode *)FindName(&kmr->kr_List, name);
	Permit();

	/* If found, return it */
	if (kmn)
	    return kmn;

	/* Prepend DEVS:Keymaps to the supplied name */
	buflen = strlen(name) + PREFIX_LEN;
	name = AllocMem(buflen, MEMF_ANY);
	if (!name)
	    return NULL;

        strcpy(name, PREFIX_STR);
	AddPart(name, km_name, buflen);
    }

    /*
     * Currently keymaps are still loaded using LoadSeg().
     * In future we can extend this. For example we can add
     * support for AmigaOS4(tm)-compatible keymaps which are plain text files.
     * ELF keymaps have one strong disadvantage: they are CPU-dependent.
     */
    km_seg = LoadSeg(name);
    if (buflen)
	FreeMem(name, buflen);

    if (!km_seg)
	return NULL;

    kmn = BADDR(km_seg) + sizeof(APTR);

    Forbid();

    /*
     * Check if this keymap is already loaded once more before installing it.
     * Now use name contained in the KeyMapNode instead of user-supplied one.
     * This is needed for two cases:
     * 1. Several programs running concurrently tried to load and install
     *    the same keymap (rare but theoretically possible case).
     * 2. We are given full file path. We need to load the file and take
     *    keymap name from it.
     */
    kmn2 = (struct KeyMapNode *)FindName(&kmr->kr_List, kmn->kn_Node.ln_Name);
    if (!kmn2)
	Enqueue(&kmr->kr_List, &kmn->kn_Node);

    Permit();

    /* If the keymap was already loaded, use the resident copy and drop our one */
    if (kmn2)
    {
	UnLoadSeg(km_seg);
	kmn = kmn2;
    }

    return kmn;

    AROS_LIBFUNC_EXIT
}
