/***************************************************************************

 codesets.library - Amiga shared library for handling different codesets
 Copyright (C) 2001-2005 by Alfonso [alfie] Ranieri <alforan@tin.it>.
 Copyright (C) 2005-2013 by codesets.library Open Source Team

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

#define ISO8859_1_STR "Schmöre bröd, schmöre bröd, bröd bröd bräd."
#define CP1251_STR    "1251 êîäèðîâêà äëÿ ïðèìåðà."
#define ASCII_STR     "latin 1 bla bla bla."
#define KOI8R_STR     "koi îÅ×ÏÚÍÏÖÎÏ ÐÅÒÅËÏÄÉÒÏ×ÁÔØ ÉÚ ËÏÄÉÒÏ×ËÉ"

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

int main(void)
{
  int res;

  if((CodesetsBase = OpenLibrary(CODESETSNAME,CODESETSVER)) &&
      GETINTERFACE(ICodesets, CodesetsBase))
  {
    ULONG errNum = 0;
    struct codeset *cs;

    if((cs = CodesetsFindBest(CSA_Source, (Tag)ISO8859_1_STR,
                              CSA_ErrPtr, (Tag)&errNum,
                              TAG_DONE)))
    {
      printf("Identified ISO8859_1_STR as %s with %d of %d errors\n", cs->name, (int)errNum, (int)strlen(ISO8859_1_STR));
    }
    else
      printf("couldn't identify ISO8859_1_STR!\n");

    if((cs = CodesetsFindBest(CSA_Source, (Tag)CP1251_STR,
                              CSA_ErrPtr, (Tag)&errNum,
                              CSA_CodesetFamily, CSV_CodesetFamily_Cyrillic,
                              TAG_DONE)))
    {
      printf("Identified CP1251_STR as %s with %d of %d errors\n", cs->name, (int)errNum, (int)strlen(CP1251_STR));
    }
    else
      printf("couldn't identify CP1251_STR!\n");

    if((cs = CodesetsFindBest(CSA_Source, (Tag)ASCII_STR,
                              CSA_ErrPtr, (Tag)&errNum,
                              CSA_CodesetFamily, CSV_CodesetFamily_Cyrillic,
                              TAG_DONE)))
    {
      printf("Identified ASCII_STR as %s with %d of %d errors\n", cs->name, (int)errNum, (int)strlen(ASCII_STR));
    }
    else
      printf("couldn't identify ASCII_STR!\n");

    if((cs = CodesetsFindBest(CSA_Source, (Tag)KOI8R_STR,
                              CSA_ErrPtr, (Tag)&errNum,
                              CSA_CodesetFamily, CSV_CodesetFamily_Cyrillic,
                              TAG_DONE)))
    {
      printf("Identified KOI8R_STR as %s with %d of %d errors\n", cs->name, (int)errNum, (int)strlen(KOI8R_STR));
    }
    else
      printf("couldn't identify KOI8R_STR!\n");

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
