/*
 *  Copyright (c) 2010-2011 Matthias Rustler
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a
 *  copy of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included
 *  in all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 *  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 *  IN THE SOFTWARE.
 *   
 *  $Id$
 */

#include <libraries/identify.h>

#include <proto/identify.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/locale.h>


#define ARG_TEMPLATE "GURU,L=LASTALERT/S"

enum
{
    ARG_GURU,
    ARG_LAST,
    ARG_COUNT
};

struct RDArgs *rda;


#define BUFSIZE (50)

static TEXT buf_dead[BUFSIZE];
static TEXT buf_subsys[BUFSIZE];
static TEXT buf_general[BUFSIZE];
static TEXT buf_spec[BUFSIZE];


#define CATCOMP_ARRAY
#include "strings.h"

#define CATALOG_NAME     "System/C/IdentifyTools.catalog"
#define CATALOG_VERSION  0

static struct Catalog 	*catalog;


const char *ver = "$VER: Guru 2.0 (31.5.2011)";

static ULONG htoi(const char *ptr)
{
    ULONG value = 0;
    char ch = *ptr;

    for (;;)
    {
        if (ch >= '0' && ch <= '9')
            value = (value << 4) + (ch - '0');
        else if (ch >= 'A' && ch <= 'F')
            value = (value << 4) + (ch - 'A' + 10);
        else if (ch >= 'a' && ch <= 'f')
            value = (value << 4) + (ch - 'a' + 10);
        else
            return value;
        ch = *(++ptr);
    }
}

static void InitLocale(STRPTR catname, ULONG version)
{
    LocaleBase = (struct LocaleBase *)OpenLibrary("locale.library", 39);
    if (LocaleBase)
    {
	catalog = OpenCatalog(NULL, catname, OC_Version, version,
					     TAG_DONE);
    }
}


static void CleanupLocale(void)
{
    if (catalog) CloseCatalog(catalog);
    if (LocaleBase) CloseLibrary((struct Library *)LocaleBase);
}


static CONST_STRPTR MSG(ULONG id)
{
    ULONG arridx;

    // we have defined message IDs in the *.cd file, so we must first search
    // for the ID in the array
    for
    (
        arridx = 0;
        arridx < sizeof (CatCompArray) / sizeof (struct CatCompArrayType) - 1;
        arridx++
    )
    {
        if (CatCompArray[arridx].cca_ID == id)
        {
            break;
        }
    }
    if (catalog != NULL)
    {
        return GetCatalogStr(catalog, id, CatCompArray[arridx].cca_Str);
    }
    return CatCompArray[arridx].cca_Str;
}


int main(void)
{
    InitLocale(CATALOG_NAME, CATALOG_VERSION);

    ULONG guru = 0;

    IPTR args[ARG_COUNT] = {0};
    
    rda = (struct RDArgs *)AllocDosObject(DOS_RDARGS, NULL);
    if (rda)
    {
        rda->RDA_ExtHelp = (STRPTR)MSG(MSG_GURU_HELP);
    
        if (ReadArgs(ARG_TEMPLATE, args, rda))
        {
            if (args[ARG_LAST])
            {
                guru = IdHardwareNum(IDHW_LASTALERT, NULL);
            }
            else if (args[ARG_GURU])
            {
                guru = htoi((STRPTR)args[ARG_GURU]);
            }

            if (IdAlertTags(guru,
                            IDTAG_DeadStr, buf_dead,
                            IDTAG_SubsysStr, buf_subsys,
                            IDTAG_GeneralStr, buf_general,
                            IDTAG_SpecStr, buf_spec,
                            IDTAG_StrLength, BUFSIZE,
                            TAG_DONE) == IDERR_OKAY)
            {
                Printf(MSG(MSG_GURU_RESULT),
                       guru,
                       buf_dead,
                       buf_subsys,
                       buf_general,
                       buf_spec);
            }
            else
            {
                PutStr(MSG(MSG_GURU_BADCODE));
            }
            FreeArgs(rda);
        }
        FreeDosObject(DOS_RDARGS, rda);
    }
    CleanupLocale();

    return 0;
}
