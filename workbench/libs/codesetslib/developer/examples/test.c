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


int main(int argc,char **argv)
{
    int res = 0;
    if((CodesetsBase = OpenLibrary(CODESETSNAME,CODESETSVER)))
    {
#if 0
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
#endif
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
