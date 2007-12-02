/***************************************************************************

 codesets.library - Amiga shared library for handling different codesets
 Copyright (C) 2001-2005 by Alfonso [alfie] Ranieri <alforan@tin.it>.
 Copyright (C) 2005-2007 by codesets.library Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 codesets.library project: http://sourceforge.net/projects/codesetslib/

 Most of the code included in this file was relicensed from GPL to LGPL
 from the source code of SimpleMail (http://www.sf.net/projects/simplemail)
 with full permissions by its authors.

 $Id$

***************************************************************************/

#include <proto/exec.h>
#include <proto/codesets.h>
#include <stdio.h>
#include <string.h>

// plain:   "�������"
#define STR "öäüÖÄÜß" \
            "öäüÖÄÜß" \
            "öäüÖÄÜß" \
            "öäüÖÄÜß" \
            "öäüÖÄÜß" \
            "öäüÖÄÜß"

#ifdef __AROS__
#include <aros/asmcall.h>
#else
#include "SDI_hook.h"
#endif

struct Library *CodesetsBase = NULL;
#if defined(__amigaos4__)
struct CodesetsIFace* ICodesets = NULL;
#endif

#if defined(__amigaos4__)
#define GETINTERFACE(iface, base)	(iface = (APTR)GetInterface((struct Library *)(base), "main", 1L, NULL))
#define DROPINTERFACE(iface)			(DropInterface((struct Interface *)iface), iface = NULL)
#else
#define GETINTERFACE(iface, base)	TRUE
#define DROPINTERFACE(iface)
#endif

#ifdef __AROS__

AROS_UFH3S(ULONG, destFunc,
AROS_UFHA(struct Hook *, h, A0),
AROS_UFHA(struct convertMsg *, msg, A2),
AROS_UFHA(STRPTR, buf, A1))
{
    AROS_USERFUNC_INIT
    printf("[%3ld] [%s]\n",msg->len,buf);

    if(msg->state == CSV_End)
      printf("\n");

    return 0;
    AROS_USERFUNC_EXIT
}

static struct Hook destHook;

#else /* __AROS__ */

HOOKPROTONH(destFunc, ULONG, struct convertMsg* msg, STRPTR buf)
{
    printf("[%3ld] [%s]\n",msg->len,buf);

    if(msg->state == CSV_End)
      printf("\n");

    return 0;
}
MakeStaticHook(destHook, destFunc);

#endif /* __AROS__ */

int main(int argc,char **argv)
{
    int            res;

    #ifdef __AROS__
    destHook.h_Entry = (HOOKFUNC)destFunc;
    #endif
    
    if((CodesetsBase = OpenLibrary(CODESETSNAME,CODESETSVER)) &&
        GETINTERFACE(ICodesets, CodesetsBase))
    {
        char *str;

        if(argc>1)
          str = argv[1];
        else
          str = STR;

        // check that the string only contains UTF8
        // sequences.
        if(CodesetsIsValidUTF8(str))
        {
          CodesetsUTF8ToStr(CSA_Source,   str,
                            CSA_DestLen,  32,
                            CSA_DestHook, &destHook,
                            TAG_DONE);
        }
        else
          printf("Error: example string wasn't recognized as UTF8!\n");

        res = 0;

        DROPINTERFACE(ICodesets);
        CloseLibrary(CodesetsBase);
        CodesetsBase = NULL;
    }
    else
    {
        printf("can't open %s %d+\n",CODESETSNAME,CODESETSVER);
        res = 20;
    }

    return res;
}
