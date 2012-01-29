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

int main(int argc,char **argv)
{
    int res;

    if (argc==3)
    {
        if((CodesetsBase = OpenLibrary(CODESETSNAME,CODESETSVER)) &&
           GETINTERFACE(ICodesets, CodesetsBase))
        {
            ULONG r;

            r = CodesetsEncodeB64(CSA_B64SourceFile, (Tag)argv[1],
                                  CSA_B64DestFile,   (Tag)argv[2],
                                  TAG_DONE);
            printf("Res %d\n", (int)r);

            DROPINTERFACE(ICodesets);
            CloseLibrary(CodesetsBase);
            CodesetsBase = NULL;

            res = 0;
        }
        else
        {
            printf("can't open %s %d+\n",CODESETSNAME,CODESETSVER);
            res = 20;
        }
    }
    else
    {
        printf("Usage: b64e <in_file> <out_file>\n");
        res = 10;
    }

    return res;
}
