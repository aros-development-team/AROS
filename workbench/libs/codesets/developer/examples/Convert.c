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

#include <exec/libraries.h>
#include <libraries/codesets.h>
#include <proto/codesets.h>
#include <proto/exec.h>

#include <stdio.h>

/* This is just a very quickly written test, not a full-featured convertor */
#define BUF_SIZE 102400

struct Library *CodesetsBase = NULL;
#if defined(__amigaos4__)
struct CodesetsIFace *ICodesets = NULL;
#endif

#if defined(__amigaos4__)
#define GETINTERFACE(iface, base)	(iface = (APTR)GetInterface((struct Library *)(base), "main", 1L, NULL))
#define DROPINTERFACE(iface)			(DropInterface((struct Interface *)iface), iface = NULL)
#else
#define GETINTERFACE(iface, base)	TRUE
#define DROPINTERFACE(iface)
#endif

struct codeset *srcCodeset;
struct codeset *destCodeset;

int main(int argc, char **argv)
{
  char *buf, *destbuf;
  ULONG destlen;
  FILE *f;

  if (argc < 4)
  {
    fprintf(stderr, "Usage: %s <source codeset> <destination codeset> <source file>\n", argv[0]);
    return 0;
  }
  if((CodesetsBase = OpenLibrary(CODESETSNAME,CODESETSVER)) &&
     GETINTERFACE(ICodesets, CodesetsBase))
  {
    srcCodeset = CodesetsFind(argv[1], CSA_FallbackToDefault, FALSE, TAG_DONE);
    if (srcCodeset)
    {
      destCodeset = CodesetsFind(argv[2], CSA_FallbackToDefault, FALSE, TAG_DONE);
      if (destCodeset)
      {
        buf = AllocMem(BUF_SIZE, MEMF_CLEAR);

        if (buf)
        {
          f = fopen(argv[3], "r");
          if (f)
          {
            fread(buf, BUF_SIZE-1, 1, f);
            fclose(f);
            destbuf = CodesetsConvertStr(CSA_SourceCodeset, (Tag)srcCodeset,
                                         CSA_DestCodeset, (Tag)destCodeset,
                                         CSA_Source, (Tag)buf,
                                         CSA_DestLenPtr, (Tag)&destlen,
                                         TAG_DONE);
            if (destbuf)
            {
              fprintf(stderr, "Result length: %u\n", (unsigned int)destlen);
              fwrite(destbuf, destlen, 1, stdout);
              fputc('\n', stderr);
              CodesetsFreeA(destbuf, NULL);
            }
          else
            fprintf(stderr, "Failed to convert text!\n");
          }
          FreeMem(buf, BUF_SIZE);
        }
        else
          fprintf(stderr, "Failed to allocate %d bytes for buffer\n", BUF_SIZE);
      }
      else
        fprintf(stderr, "Unknown destination codeset %s\n", argv[2]);
    }
    else
      fprintf(stderr, "Unknown source codeset %s\n", argv[1]);

    DROPINTERFACE(ICodesets);
    CloseLibrary(CodesetsBase);
  }
  else
    fprintf(stderr, "Failed to open codesets.library!\n");

  return 0;
}

