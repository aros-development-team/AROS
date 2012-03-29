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

#include <aros/debug.h>

#define ARG_TEMPLATE "LN=LIBNAME/A,O=OFFSET/N/A"

enum
{
    ARG_LIBNAME,
    ARG_OFFSET,
    ARG_COUNT
};

struct RDArgs *rda;


#define BUFSIZE (50)

static TEXT buf_fnname[BUFSIZE];


#define CATCOMP_ARRAY
#include "strings.h"

#define CATALOG_NAME     "System/C/IdentifyTools.catalog"
#define CATALOG_VERSION  0

static struct Catalog 	*catalog;


const char *ver = "$VER: Function 2.0 (31.5.2011)";


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

    IPTR args[ARG_COUNT] = {0};

    ULONG offset;

    rda = (struct RDArgs *)AllocDosObject(DOS_RDARGS, NULL);
    if (rda)
    {
        rda->RDA_ExtHelp = (STRPTR)MSG(MSG_FUNC_HELP);
    
        if (ReadArgs(ARG_TEMPLATE, args, rda))
        {
            offset = *(ULONG *)args[ARG_OFFSET];

            if (IdFunctionTags((STRPTR)args[ARG_LIBNAME],
                               offset,
                               IDTAG_FuncNameStr, buf_fnname,
                               IDTAG_StrLength, BUFSIZE,
                               TAG_DONE) == IDERR_OKAY)
            {
                Printf(MSG(MSG_FUNC_RESULT),
                       args[ARG_LIBNAME],
                       offset,
                       buf_fnname);
            }
            FreeArgs(rda);
        }
        FreeDosObject(DOS_RDARGS, rda);
    }
    CleanupLocale();

    return 0;
}
