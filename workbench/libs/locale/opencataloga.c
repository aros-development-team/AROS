/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/
#include <exec/types.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <libraries/iffparse.h>
#include <proto/iffparse.h>
#include <string.h>
#include <aros/macros.h>
#include "locale_intern.h"

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

  struct Locale * def_locale = NULL;
  struct IntCatalog * catalog = NULL;
  char * language;
  char * app_language;
#define FILENAMESIZE 256
  char filename[FILENAMESIZE];
  ULONG version;
  
  if (NULL == locale)
    def_locale = OpenLocale(NULL);
  else
    def_locale = locale;

  language = (char *)GetTagData(OC_Language, 
                                (ULONG)def_locale->loc_LanguageName, 
                                tags);

  /*
  ** Check whether the built in language of the application matches
  ** the language of the default locale. If it matches, then I
  ** don't need to load anything.
  */
  app_language = (char *)GetTagData(OC_BuiltInLanguage,
                                    (ULONG)0UL,
                                    tags);
  
  if (NULL != app_language && 0 == strcmp(app_language, language))
    return NULL;

  version = GetTagData(OC_Version,
                       0,
                       tags);

  if (NULL != name)
  {
    struct IFFHandle * iff = NULL;

    /* 
    ** The wanted catalog might be in the list of catalogs that are
    ** already loaded. So check that list first.
    */
    ObtainSemaphore(&IntLB(LocaleBase)->lb_CatalogLock);
    ForeachNode(&IntLB(LocaleBase)->lb_CatalogList,
                (struct Node *)catalog)
    {
      if (catalog->ic_Name && 
          0 == strcmp(catalog->ic_Name, name) &&
          catalog->ic_Catalog.cat_Language &&
          0 == strcmp(catalog->ic_Catalog.cat_Language, language))
      {
        ReleaseSemaphore(&IntLB(LocaleBase)->lb_CatalogLock);
        catalog->ic_UseCount++;
        return (struct Catalog *)catalog;
      }
    }
    
    ReleaseSemaphore(&IntLB(LocaleBase)->lb_CatalogLock);
    

    /* Clear error condition before we start. */
    SetIoErr(0);

    iff = AllocIFF();
    if (NULL == iff)
    {
      SetIoErr(ERROR_NO_FREE_STORE);
      return NULL;
    }

    strcpy(filename, "LOCALE:Catalogs");
    AddPart(filename, language, FILENAMESIZE);
    AddPart(filename, name    , FILENAMESIZE);
    iff->iff_Stream = (ULONG)Open(filename, MODE_OLDFILE);

#if 0
    if (NULL == iff->iff_Stream)
    {
      /* try it in PROGDIR */
      strcpy(filename, "PROGDIR:Catalogs");  
      AddPart(filename, language, FILENAMESIZE);
      AddPart(filename, name    , FILENAMESIZE);
      iff->iff_Stream = (ULONG)Open(filename, MODE_OLDFILE);
    }
#endif
#undef FILENAMESIZE

    if (NULL == iff->iff_Stream)
    {
      FreeIFF(iff);
      SetIoErr(ERROR_NO_FREE_STORE);
      return NULL;
    }

    catalog = AllocMem(sizeof(struct IntCatalog), MEMF_CLEAR|MEMF_PUBLIC);
    if (NULL == catalog)
    {
      FreeIFF(iff);
      SetIoErr(ERROR_NO_FREE_STORE);
      return NULL;
    }

    catalog->ic_UseCount = 1;

    InitIFFasDOS(iff);
    if (!OpenIFF(iff, IFFF_READ))
    {
      while (1)
      {
        ULONG error = ParseIFF(iff, IFFPARSE_STEP);
          
        if (IFFERR_EOF == error)
        {
          /* Did everything go fine? */
          catalog->ic_Name = AllocVec(strlen(name)+1, MEMF_ANY);
          strcpy(catalog->ic_Name, name);

          catalog->ic_Catalog.cat_Language = AllocVec(strlen(language)+1,MEMF_ANY);
          strcpy(catalog->ic_Catalog.cat_Language, language);

          /* Connect this catalog to the list of catalogs */
          ObtainSemaphore (&IntLB(LocaleBase)->lb_CatalogLock);
          AddHead((struct List *)&IntLB(LocaleBase)->lb_CatalogList, 
                  &catalog->ic_Catalog.cat_Link);
          ReleaseSemaphore(&IntLB(LocaleBase)->lb_CatalogLock);

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
            break;

            case ID_LANG:
              catalog->ic_Catalog.cat_Language = AllocVec(top->cn_Size, MEMF_PUBLIC);
              if (top->cn_Size != ReadChunkBytes(iff,
                                                 catalog->ic_Catalog.cat_Language,
                                                 top->cn_Size))
              {
                /* an error occurred */
                dispose_catalog(catalog, LocaleBase);
                break;
              }
            break;

            case ID_CSET:
            {
              /* what am I supposed to do with this ? */
            }

            case ID_STRS:
            {
              ULONG c = 0;
              
              while (c < top->cn_Size)
              {
                ULONG read;
                struct CatStr * catstr;
                struct header _header;
                ULONG id, len;

                /* first read the id and the length of the string */
                read = ReadChunkBytes(iff,
                                      &_header,
                                      8);
                                      
                if (8 != read)
                {
                  /* an error happened while reading */
                  dispose_catalog(catalog, LocaleBase);
                  break;
                }
                
                
#if (AROS_BIG_ENDIAN == 0)
                len = ((_header.len[3]      ) & 0x000000ff) |
                      ((_header.len[2] << 8 ) & 0x0000ff00) |
                      ((_header.len[1] << 16) & 0x00ff0000) |
                      ((_header.len[0] << 24) & 0xff000000) ;

                id = ((_header.id[3]      ) & 0x000000ff) |
                     ((_header.id[2] << 8 ) & 0x0000ff00) |
                     ((_header.id[1] << 16) & 0x00ff0000) |
                     ((_header.id[0] << 24) & 0xff000000) ;
#else
                len = *(ULONG *)&_header.len[0];
                id  = *(ULONG *)&_header.id[0];
#endif

                c += read;
                catstr = (struct CatStr* ) 
                                AllocVec(sizeof(struct CatStr)+len+(len&1), 
                                         MEMF_PUBLIC|MEMF_CLEAR);
                
                if (NULL == catstr)
                {
                  SetIoErr(ERROR_NO_FREE_STORE);
                  dispose_catalog(catalog, LocaleBase);
                }

                read = ReadChunkBytes(iff, 
                                      &catstr->cs_Data[0], 
                                      len+(len&1));
                                      
                if (read != len+(len&1))
                {
                  /* An error happened */
                  dispose_catalog(catalog, LocaleBase);
                  break;
                }
                
                /* 
                ** Initialize the CatStr structure
                */
                catstr->cs_Id = id;
                catstr->cs_Data[len+(len&1)-1] = '\0';
                
                catstr->cs_Next = catalog->ic_First;
                catalog->ic_First = catstr;

//                kprintf("id of string:  %d\n",id);
//                kprintf("string length: %d\n",len);
//                kprintf("string: %s\n",catstr->cs_Data);

                c+=read+(read&1);
                
                /* Align it to 4 byte boundaries, if necessary */
                if (0 != (c & 3))
                  read = ReadChunkBytes(iff, &_header, 4-( c&3 ));
                
                if (read < 0)
                {
                  /* an error occurred! */
                  dispose_catalog(catalog, LocaleBase);
                  break;
                }
                
                c += read;
              }
            }  
            break;
          } /* switch () */
        } 
        else
        {
          /*
          ** An error with the file occurred 
          */
          break;
        }
      } /* while (1) */
      CloseIFF(iff);
    }


    Close((BPTR)iff->iff_Stream);
    FreeIFF(iff);
    FreeMem(catalog, sizeof(struct IntCatalog));
  }  
    
  if (def_locale)
    CloseLocale(def_locale);
    
  return NULL;

  AROS_LIBFUNC_EXIT
} /* OpenCatalogA */
