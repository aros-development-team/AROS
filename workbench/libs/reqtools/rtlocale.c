/*********
* LOCALE *
*********/

#include "filereq.h"

#include <proto/locale.h>
#include <string.h>


char *REGARGS GetStr (struct Catalog *cat, char *idstr)
{
	char *local;

	local = idstr + 2;
	if (LocaleBase) return ((char *)GetCatalogStr (cat, *(UWORD *)idstr, local));
	return (local);
}

//ULONG catalogtags[] = { OC_Version, 38, TAG_END };
#define catalogtags	NULL

#undef ThisProcess()
#define ThisProcess()		((struct Process *)FindTask(NULL))

struct Catalog *REGARGS RT_OpenCatalog (struct Locale *locale)
{
	struct Process *proc;
	struct Catalog *cat;
	APTR 		oldwinptr;

	if (!LocaleBase) return (NULL);
	
	proc = ThisProcess();
	
	if (proc->pr_Task.tc_Node.ln_Type != NT_PROCESS) return (NULL);
	
	oldwinptr = proc->pr_WindowPtr;
	proc->pr_WindowPtr = (APTR)-1;
	cat = OpenCatalogA (locale, "reqtools.catalog", (struct TagItem *)catalogtags);
	proc->pr_WindowPtr = oldwinptr;
	
	return (cat);
}

void REGARGS RT_CloseCatalog (struct Catalog *cat)
{
	if (LocaleBase) CloseCatalog (cat);
}
