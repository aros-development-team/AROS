/*
    Copyright  1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Backwards compatibility display driver loader.
    Lang: english
*/

#include <aros/bootloader.h>
#include <dos/dos.h>
#include <hidd/hidd.h>
#include <proto/bootloader.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <oop/oop.h>

#include <ctype.h>
#include <string.h>

/************************************************************************/

#define HIDDPREFSFILE "SYS:S/hidd.prefs"
#define BUFSIZE 256

static UBYTE buf[BUFSIZE];
static UBYTE hiddname[BUFSIZE];

int __nocommandline = 1;

/*
 * Currently this code does what dosboot resident previously did:
 * 1. Opens S:hidd.prefs, opens specified libraries and picks up specified display driver name
 * 2. Opens bootloader.resource and processes its arguments in the same way.
 * 3. Creates a single object of obtained display driver HIDD class and adds it to the system.
 *
 * In future S:hidd.prefs and bootloader arguments will completely go away. This program will
 * have to process its tooltypes (there will be a possibility to have several copies of this
 * program with different icons in order to load several different drivers)
 */

int main(void)
{
    BPTR fh;
    APTR BootLoaderBase;

    hiddname[0] = 0;

    /* Open the hidd prefsfile */	
    fh = Open(HIDDPREFSFILE, MODE_OLDFILE);
    if (fh) {

        while (FGets(fh, buf, BUFSIZE)) {
	    STRPTR keyword = buf, arg, end;
	    STRPTR s;

	    if (*buf == '#')
		continue;

	    s = buf;
	    if (*s) {
		for (; *s; s ++);

		if (s[-1] == 10)
		    s[-1] = 0;
	    }		    

	    /* Get keyword */
	    while ((*keyword != 0) && isspace(*keyword))
		keyword ++;

	    if (*keyword == 0)
		continue;

	    /* terminate keyword */
	    arg = keyword;
	    while ((*arg != 0) && (!isblank(*arg)))
		arg ++;
	    if (*arg == 0)
		continue;
	    *arg++ = 0;

	    /* Find start of argument */
	    while ((*arg != 0) && isblank(*arg))
		arg ++;

	    if (*arg == 0)
		continue;

	    /* terminate argument */
	    end = arg;
	    while ( (*end != 0) && (!isblank(*end)))
		end ++;
	    if (*end != 0)
		*end = 0;

	    if (0 == strcmp(keyword, "library")) {
		/* Open a specified library */
		OpenLibrary(arg, 0);
	    } else if (0 == strcmp(keyword, "gfx")) {
		strncpy(hiddname, arg, BUFSIZE - 1);
	    }
	}
	Close(fh);
    }

    BootLoaderBase = OpenResource("bootloader.resource");
    if (BootLoaderBase) {
	struct List *list;
	struct Node *node;

	list = (struct List *)GetBootInfo(BL_Args);
	if (list) {
	    ForeachNode(list,node) {
		if (0 == strncmp(node->ln_Name,"lib=",4)) {
		    OpenLibrary(&node->ln_Name[4],0L);
		} else if (0 == strncmp(node->ln_Name,"gfx=",4)) {
		    strncpy(hiddname, &node->ln_Name[4], BUFSIZE-1);
		}
	    }
	}
    }

    if (hiddname[0]) {
        struct GfxBase *GfxBase;
	OOP_Object *gfxhidd;
	int res = RETURN_ERROR;

	GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 41);
	if (!GfxBase)
	    return RETURN_FAIL;

	gfxhidd = OOP_NewObject(NULL, hiddname, NULL);
	if (gfxhidd) {
	    if (AddDisplayDriverA(gfxhidd, NULL))
		OOP_DisposeObject(gfxhidd);
	    else
		res = RETURN_OK;
	}
	
	CloseLibrary(&GfxBase->LibNode);
	
	return res;
    }

    return RETURN_OK;
}

