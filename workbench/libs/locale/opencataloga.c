/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/
#define AROS_ALMOST_COMPATIBLE

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <libraries/iffparse.h>
#include <proto/iffparse.h>
#include <proto/utility.h>
#include <string.h>
#include "locale_intern.h"

#include <aros/debug.h>

#define	DEBUG_OPENCATALOG(x)	;

struct header
{
    unsigned char id[4];
    unsigned char len[4];
};

/*****************************************************************************

    NAME */
#include <proto/locale.h>

	AROS_LH3(struct Catalog *, OpenCatalogA,

/*  SYNOPSIS */
	AROS_LHA(struct Locale  *, locale, A0),
	AROS_LHA(STRPTR          , name, A1),
	AROS_LHA(struct TagItem *, tags, A2),

/*  LOCATION */
	struct LocaleBase *, LocaleBase, 25, Locale)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    locale_lib.fd and clib/locale_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,LocaleBase)

    struct Locale   	* def_locale = NULL;
    struct IntCatalog 	* catalog = NULL;
    char    	    	* language;
    char    	    	* app_language;
    char    	    	* specific_language;
    struct Process	* MyProcess;	
#define FILENAMESIZE 256

    char    	    	  filename[FILENAMESIZE];
    char                  buf[100];
    LONG                  chars;
    ULONG   	    	  version;
    ULONG		    	  catversion,catrevision;
    WORD    	    	  pref_language;
    UWORD                 i;

    DEBUG_OPENCATALOG(dprintf("OpenCatalogA: locale 0x%lx name <%s> Tags 0x%lx localebase 0x%lx\n",
				locale,
				name,
				tags,
				LocaleBase));

    SetIoErr(0);
    
    if (!locale)
    {
	DEBUG_OPENCATALOG(dprintf("OpenCatalogA: no locale\n"));
	locale = def_locale = OpenLocale(NULL);
	DEBUG_OPENCATALOG(dprintf("OpenCatalogA: def_locale 0x%lx\n",
				def_locale));
    }

    if (!locale)
    {
	DEBUG_OPENCATALOG(dprintf("OpenCatalogA: nolocale..done\n"));
	return NULL;
    }

    MyProcess=(struct Process *)FindTask(NULL);

    specific_language = (char *)GetTagData(OC_Language, (IPTR)0, tags);

    DEBUG_OPENCATALOG(dprintf("OpenCatalogA: specific lang 0x%lx\n",specific_language));
    if (specific_language)
    {
	language = specific_language;
	pref_language = -1;
	DEBUG_OPENCATALOG(dprintf("OpenCatalogA: language 0x%lx\n",language));
    }
    else
    {
	language = locale->loc_PrefLanguages[0];
	pref_language = 0;
	DEBUG_OPENCATALOG(dprintf("OpenCatalogA: language 0x%lx\n",language));
    }

    if (language == NULL)
    {
    	if (def_locale) CloseLocale(def_locale);
	DEBUG_OPENCATALOG(dprintf("OpenCatalogA: nolanguage..done\n"));
    	return NULL;
    }

    /*
    ** Check whether the built in language of the application matches
    ** the language of the default locale. If it matches, then I
    ** don't need to load anything.
    */
    
    app_language = "english";    
    app_language = (char *)GetTagData(OC_BuiltInLanguage,
                                      (IPTR)app_language,
                                      tags);

    DEBUG_OPENCATALOG(dprintf("OpenCatalogA: app_language 0x%lx\n",
				app_language));

    if (NULL != app_language && 0 == strcmp(app_language, language))
    {
    	if (def_locale) CloseLocale(def_locale);
	DEBUG_OPENCATALOG(dprintf("OpenCatalogA: failure..done\n"));
	return NULL;
    }

    version = GetTagData(OC_Version,
                	 0,
                	 tags);

    DEBUG_OPENCATALOG(dprintf("OpenCatalogA: version %ld\n",version));

    if (NULL != name)
    {
	struct IFFHandle * iff = NULL;

	/*
	** The wanted catalog might be in the list of catalogs that are
	** already loaded. So check that list first.
	*/

	DEBUG_OPENCATALOG(dprintf("OpenCatalogA: CatalogLock 0x%lx\n",
					&IntLB(LocaleBase)->lb_CatalogLock));

	ObtainSemaphore(&IntLB(LocaleBase)->lb_CatalogLock);

	DEBUG_OPENCATALOG(dprintf("OpenCatalogA: search cached Catalog\n"));

	ForeachNode(&IntLB(LocaleBase)->lb_CatalogList, catalog)
	{
	    if (catalog->ic_Name &&
        	0 == strcmp(catalog->ic_Name, name) &&
        	catalog->ic_Catalog.cat_Language &&
        	0 == strcmp(catalog->ic_Catalog.cat_Language, language))
	    {
		DEBUG_OPENCATALOG(dprintf("OpenCatalogA: found Catalog 0x%lx\n",catalog));
        	catalog->ic_UseCount++;
        	ReleaseSemaphore(&IntLB(LocaleBase)->lb_CatalogLock);

		if (def_locale)
		{
		    CloseLocale(def_locale);
		}

		SetIoErr(ERROR_ACTION_NOT_KNOWN);

		DEBUG_OPENCATALOG(dprintf("OpenCatalogA: return Catalog 0x%lx\n",catalog));
		return (struct Catalog *)catalog;
	    }
	}
	DEBUG_OPENCATALOG(dprintf("OpenCatalogA: found none\n"));

	ReleaseSemaphore(&IntLB(LocaleBase)->lb_CatalogLock);


	/* Clear error condition before we start. */
	SetIoErr(0);

	iff = AllocIFF();

	DEBUG_OPENCATALOG(dprintf("OpenCatalogA: iff 0x%lx\n",iff));
	if (NULL == iff)
	{
	    if (def_locale) CloseLocale(def_locale);
	    SetIoErr(ERROR_NO_FREE_STORE);

	    return NULL;
	}

	DEBUG_OPENCATALOG(dprintf("OpenCatalogA: pref_language %ld language 0x%lx\n",
				pref_language,
				language));

	while ((pref_language < 10) && language)
	{
	    if (app_language && (strcmp(language, app_language) == 0))
	    {
		FreeIFF(iff);
	    	if (def_locale) CloseLocale(def_locale);
		SetIoErr(0);
		return NULL;
	    }
	    
	    if ((MyProcess->pr_HomeDir) != NULL)
	    {
		DEBUG_OPENCATALOG(dprintf("OpenCatalogA: HomeDir !=NULL..try progdir\n"));
		strcpy(filename, "PROGDIR:Catalogs");
		DEBUG_OPENCATALOG(dprintf("OpenCatalogA: filename <%s>\n",filename));
		AddPart(filename, language, FILENAMESIZE);
		DEBUG_OPENCATALOG(dprintf("OpenCatalogA: filename <%s>\n",filename));
		AddPart(filename, name    , FILENAMESIZE);

		DEBUG_OPENCATALOG(dprintf("OpenCatalogA: filename <%s>\n",filename));
		iff->iff_Stream = (IPTR)Open(filename, MODE_OLDFILE);
		DEBUG_OPENCATALOG(dprintf("OpenCatalogA: iffstream 0x%lx\n",
					iff->iff_Stream));
	    }
	    if (iff->iff_Stream) break;

#ifdef __MORPHOS__
    	    strcpy(filename, "MOSSYS:LOCALE/Catalogs");

	    DEBUG_OPENCATALOG(dprintf("OpenCatalogA: filename <%s>\n",filename));
            AddPart(filename, language, FILENAMESIZE);
	    DEBUG_OPENCATALOG(dprintf("OpenCatalogA: filename <%s>\n",filename));
            AddPart(filename, name    , FILENAMESIZE);

	    DEBUG_OPENCATALOG(dprintf("OpenCatalogA: try filename <%s>\n",filename));

	    { 	APTR oldwinptr=MyProcess->pr_WindowPtr;
		MyProcess->pr_WindowPtr=(APTR)-1;
		iff->iff_Stream = (IPTR)Open(filename, MODE_OLDFILE);
		MyProcess->pr_WindowPtr=oldwinptr;
 	    }

	    if (iff->iff_Stream) break;
#endif
    	    strcpy(filename, "LOCALE:Catalogs");

	    DEBUG_OPENCATALOG(dprintf("OpenCatalogA: filename <%s>\n",filename));
            AddPart(filename, language, FILENAMESIZE);
	    DEBUG_OPENCATALOG(dprintf("OpenCatalogA: filename <%s>\n",filename));
            AddPart(filename, name    , FILENAMESIZE);

	    DEBUG_OPENCATALOG(dprintf("OpenCatalogA: try filename <%s>\n",filename));

            iff->iff_Stream = (IPTR)Open(filename, MODE_OLDFILE);
	    if (iff->iff_Stream) break;

	    pref_language++;
	    language = locale->loc_PrefLanguages[pref_language];
	}

	DEBUG_OPENCATALOG(dprintf("OpenCatalogA: language 0x%lx\n",language));

    	/* I no longer need the locale. So close it if we opened it */

    	if (def_locale)
	{
	    CloseLocale(def_locale);
	    def_locale = NULL;
	}

#undef FILENAMESIZE

	if (iff->iff_Stream == 0)
	{
	    DEBUG_OPENCATALOG(dprintf("OpenCatalogA: end..nostream\n"));
	    FreeIFF(iff);
	    SetIoErr(ERROR_OBJECT_NOT_FOUND);
	    return NULL;
	}

	DEBUG_OPENCATALOG(dprintf("OpenCatalogA: catalog name <%s>\n",name));
	catalog = AllocVec(sizeof(struct IntCatalog) + strlen(name) + 1, MEMF_CLEAR | MEMF_PUBLIC);
	DEBUG_OPENCATALOG(dprintf("OpenCatalogA: catalog 0x%lx\n",catalog));
	if (NULL == catalog)
	{
	    DEBUG_OPENCATALOG(dprintf("OpenCatalogA: end..no catalog\n"));
	    Close((BPTR)iff->iff_Stream);
	    FreeIFF(iff);
	    SetIoErr(ERROR_NO_FREE_STORE);
	    return NULL;
	}

	catalog->ic_UseCount = 1;
    	catalog->ic_Catalog.cat_Language = catalog->ic_LanguageName;
	strcpy(catalog->ic_Name, name);

	InitIFFasDOS(iff);

	if (!OpenIFF(iff, IFFF_READ))
	{
	    while (1)
	    {
        	LONG error = ParseIFF(iff, IFFPARSE_STEP);

        	if (IFFERR_EOF == error)
        	{
		    DEBUG_OPENCATALOG(dprintf("OpenCatalogA: parsed catalog\n"));
        	    /* Did everything go fine? */

    	    	    if (!(catalog->ic_LanguageName[0]))
		    {
		    	/* No ID_LANG chunk found. So setup languagename ourselves.
			   Hmm ... maybe this should be done anyway always. Because
			   if the catalog file *does* contain an ID_LANG chunk then
			   this should be the same as "language" anyway. And if it
			   is not, then maybe OpenCatalogA() should fail :-\ */

        	    	strcpy(catalog->ic_LanguageName, language);
    	    	    }

        	    /* Connect this catalog to the list of catalogs */
        	    ObtainSemaphore (&IntLB(LocaleBase)->lb_CatalogLock);
        	    AddHead((struct List *)&IntLB(LocaleBase)->lb_CatalogList,
                	    &catalog->ic_Catalog.cat_Link);
        	    ReleaseSemaphore(&IntLB(LocaleBase)->lb_CatalogLock);

    	    	    CloseIFF(iff);
	    	    Close((BPTR)iff->iff_Stream);
		    FreeIFF(iff);

		    DEBUG_OPENCATALOG(dprintf("OpenCatalogA: return catalog 0x%lx\n",catalog));
		    
        	    return &catalog->ic_Catalog;
        	}

        	if (IFFERR_EOC == error) /* end of chunk */
        	    continue;

        	if (0 == error)
        	{
        	    struct ContextNode * top = CurrentChunk(iff);

        	    switch (top->cn_ID)
        	    {
        		case ID_FORM:
        		    break;

        		case ID_FVER:
                
        		    if (top->cn_Size > 100) break; /*max 100 bytes*/

			    error = ReadChunkBytes(iff, buf, top->cn_Size);
			    if (error == top->cn_Size)
			    {
			    	error = 0;
			    }
			    else
			    {
				break;
			    }

                buf[99] = 0;

                /*Ok now we want to get the version and the revision.
                They are separated by a blank space*/

				i = 0;
                while (buf[i] && (buf[i] != ' '))
                i++;
                while (buf[i] && (buf[i] == ' '))
                i++;
                while (buf[i] && (buf[i] != ' '))
                i++;

                if (buf[i])
                {
                	if ((chars = StrToLong(&buf[i],(LONG *)&catversion)) < 0)
                		break;

                	i += chars;
                	if (buf[i++])
                	{
                		if (StrToLong(&buf[i],(LONG *)&catrevision) < 0)
                    	break;
           			}

                 	catalog->ic_Catalog.cat_Version  = catversion;
                	catalog->ic_Catalog.cat_Revision = catrevision;
                    
                 	if (version && (catversion != version))
                    	{
                    		error = RETURN_ERROR;
                    	}

                    	break;
                }


        		case ID_LANG:
			    /* The IntCatalog structure has only 30 bytes reserved for
			       the language name. So make sure the chunk is not bigger. */

        		    if (top->cn_Size > 30) break;

			    error = ReadChunkBytes(iff, catalog->ic_LanguageName, top->cn_Size);
			    if (error == top->cn_Size)
			    {
			    	error = 0;
			    }
        		    break;

        		case ID_CSET:
        		    if (top->cn_Size != sizeof(struct CodeSet)) break;
			    if (top->cn_Size == ReadChunkBytes(iff,
			    	    	    	    	       &catalog->ic_CodeSet,
							       top->cn_Size))
			    {
			    	/* Who cares: codeset is not used at the moment */
			    }
     			    break;

        		case ID_STRS:
    	    	    	    if (!(catalog->ic_StringChunk = AllocVec(top->cn_Size, MEMF_PUBLIC | MEMF_CLEAR)))
			    {
    	    	    	    	error = ERROR_NO_FREE_STORE;
			    	SetIoErr(error);
			    	break;
			    }

			    error = ReadChunkBytes(iff, catalog->ic_StringChunk, top->cn_Size);
			    if (error == top->cn_Size)
			    {
			    	error = 0;
			    }
			    else
			    {
			    	break;
			    }

			    /* Count the number of strings */

			    {
			    	UBYTE *buffer = catalog->ic_StringChunk;
				LONG   c = 0, strlen;

			    	catalog->ic_NumStrings = 0;

				while(c < top->cn_Size)
				{
				    catalog->ic_NumStrings++;

				    strlen = (buffer[4] << 24) +
				    	     (buffer[5] << 16) +
					     (buffer[6] << 8) +
					     (buffer[7]);

				    strlen  = 8 + strlen + (strlen & 1);
				    if (strlen & 3) strlen += 4 - (strlen & 3);

				    c += strlen; buffer += strlen;
				}

			    }

    	    	    	    if (catalog->ic_NumStrings == 0) break; /* Paranoia? */

    	    	    	    if (!(catalog->ic_CatStrings = AllocVec(catalog->ic_NumStrings * sizeof(struct CatStr), MEMF_PUBLIC | MEMF_CLEAR)))
			    {
    	    	    	    	error = ERROR_NO_FREE_STORE;
			    	SetIoErr(error);
				break;
			    }

			    /* Fill out catalog->ic_CatStrings array */

			    {
			    	UBYTE *buffer = catalog->ic_StringChunk;
				ULONG i, strlen, id, previd = 0;
				BOOL  inorder = TRUE;

				for(i = 0; i < catalog->ic_NumStrings; i++)
				{
				    id = (buffer[0] << 24) +
				    	 (buffer[1] << 16) +
					 (buffer[2] << 8) +
					 (buffer[3]);

				    strlen = (buffer[4] << 24) +
				    	     (buffer[5] << 16) +
					     (buffer[6] << 8) +
					     (buffer[7]);

				    catalog->ic_CatStrings[i].cs_String = &buffer[8];
				    catalog->ic_CatStrings[i].cs_Id = id;

    	    	    	    	    //kprintf("Catalog String #%d id=%d string=\"%s\"\n", i, id, catalog->ic_CatStrings[i].cs_String);

				    strlen  = 8 + strlen + (strlen & 1);
				    if (strlen & 3) strlen += 4 - (strlen & 3);

				    if (id < previd) inorder = FALSE;

				    buffer += strlen;
				    previd = id;
				}

				if (inorder) catalog->ic_Flags |= ICF_INORDER;
			    }
			    break;

        	    } /* switch (top->cn_ID) */

        	} /* if (0 == error) */

		if (error)
        	{
		    dispose_catalog(catalog, LocaleBase);
        	    /*
        	    ** An error with the file occurred
        	    */
        	    break;
        	}

	    } /* while (1) */

	    CloseIFF(iff);

	} /* if (!OpenIFF(iff, IFFF_READ)) */


	Close((BPTR)iff->iff_Stream);
	FreeIFF(iff);
	FreeVec(catalog);

    } /* if (NULL != name) */

    if (def_locale) CloseLocale(def_locale);

    DEBUG_OPENCATALOG(dprintf("OpenCatalogA: catalog not loaded\n"));
    return NULL;

    AROS_LIBFUNC_EXIT

} /* OpenCatalogA */

