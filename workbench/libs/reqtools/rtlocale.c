/*********
* LOCALE *
*********/

#include "filereq.h"

#include <proto/locale.h>
#include <string.h>

#ifdef __AROS__
#include <aros/macros.h>
#endif

/****************************************************************************************/

char *REGARGS GetStr (struct Catalog *cat, char *idstr)
{
    char  *local;
    UWORD id = *(UWORD *)idstr;

#ifdef __AROS__
#if !AROS_BIG_ENDIAN
    id = AROS_BE2WORD(id);
#endif
#endif	
    local = idstr + 2;
    if (LocaleBase) return ((char *)GetCatalogStr (cat, id, local));
    return (local);
}

/****************************************************************************************/

//ULONG catalogtags[] = { OC_Version, 38, TAG_END };
#define catalogtags	NULL

#undef ThisProcess
#define ThisProcess()		((struct Process *)FindTask(NULL))

/****************************************************************************************/

struct Catalog *REGARGS RT_OpenCatalog (struct Locale *locale)
{
    struct Process 	*proc;
    struct Catalog 	*cat;
    APTR 		oldwinptr;

    if (!LocaleBase) return (NULL);

    proc = ThisProcess();

    if (proc->pr_Task.tc_Node.ln_Type != NT_PROCESS) return (NULL);

    oldwinptr = proc->pr_WindowPtr;
    proc->pr_WindowPtr = (APTR)-1;
    cat = OpenCatalogA (locale, "System/Libs/reqtools.catalog", (struct TagItem *)catalogtags);
    proc->pr_WindowPtr = oldwinptr;

    return (cat);
}

/****************************************************************************************/

void REGARGS RT_CloseCatalog (struct Catalog *cat)
{
    if (LocaleBase) CloseCatalog (cat);
}

/****************************************************************************************/
