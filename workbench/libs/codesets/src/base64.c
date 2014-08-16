/***************************************************************************

 codesets.library - Amiga shared library for handling different codesets
 Copyright (C) 2001-2005 by Alfonso [alfie] Ranieri <alforan@tin.it>.
 Copyright (C) 2005-2014 codesets.library Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 codesets.library project: http://sourceforge.net/projects/codesetslib/

 $Id$

***************************************************************************/

#include "lib.h"
#include "SDI_stdarg.h"
#include "debug.h"

/****************************************************************************/

struct b64
{
  APTR   in;
  APTR   out;
  ULONG  flags;
  int    inIndex;
  int    inAvailable;
  int    outIndex;
  int    maxLineLen;
  int    lineCounter;
  const char *eols;
  LONG   error;
};

enum
{
  B64FLG_SourceFile = 1<<0,
  B64FLG_DestFile   = 1<<1,
  B64FLG_Unix       = 1<<2,
};

/****************************************************************************/

#define MAXLINELEN 72

#ifndef EOF
#define EOF (-1)
#endif

/****************************************************************************/

static const UBYTE etable[] =
{
   65,   66,   67,   68,   69,   70,   71,   72,   73,   74,
   75,   76,   77,   78,   79,   80,   81,   82,   83,   84,
   85,   86,   87,   88,   89,   90,   97,   98,   99,  100,
  101,  102,  103,  104,  105,  106,  107,  108,  109,  110,
  111,  112,  113,  114,  115,  116,  117,  118,  119,  120,
  121,  122,   48,   49,   50,   51,   52,   53,   54,   55,
   56,   57,   43,   47,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0
};

static const UBYTE dtable[] =
{
  128,  128,  128,  128,  128,  128,  128,  128,  128,  128,
  128,  128,  128,  128,  128,  128,  128,  128,  128,  128,
  128,  128,  128,  128,  128,  128,  128,  128,  128,  128,
  128,  128,  128,  128,  128,  128,  128,  128,  128,  128,
  128,  128,  128,   62,  128,  128,  128,   63,   52,   53,
   54,   55,   56,   57,   58,   59,   60,   61,  128,  128,
  128,    0,  128,  128,  128,    0,    1,    2,    3,    4,
    5,    6,    7,    8,    9,   10,   11,   12,   13,   14,
   15,   16,   17,   18,   19,   20,   21,   22,   23,   24,
   25,  128,  128,  128,  128,  128,  128,   26,   27,   28,
   29,   30,   31,   32,   33,   34,   35,   36,   37,   38,
   39,   40,   41,   42,   43,   44,   45,   46,   47,   48,
   49,   50,   51,  128,  128,  128,  128,  128,  128,  128,
  128,  128,  128,  128,  128,  128,  128,  128,  128,  128,
  128,  128,  128,  128,  128,  128,  128,  128,  128,  128,
  128,  128,  128,  128,  128,  128,  128,  128,  128,  128,
  128,  128,  128,  128,  128,  128,  128,  128,  128,  128,
  128,  128,  128,  128,  128,  128,  128,  128,  128,  128,
  128,  128,  128,  128,  128,  128,  128,  128,  128,  128,
  128,  128,  128,  128,  128,  128,  128,  128,  128,  128,
  128,  128,  128,  128,  128,  128,  128,  128,  128,  128,
  128,  128,  128,  128,  128,  128,  128,  128,  128,  128,
  128,  128,  128,  128,  128,  128,  128,  128,  128,  128,
  128,  128,  128,  128,  128,  128,  128,  128,  128,  128,
  128,  128,  128,  128,  128,  128,  128,  128,  128,  128,
  128,  128,  128,  128,  128,    0
};

/****************************************************************************/

#if defined(__amigaos4__)
static BPTR openIn(STRPTR name, int64 *size)
{
  BPTR file = 0;
  struct ExamineData *exd;

  ENTER();

  if((exd = ExamineObjectTags(EX_StringNameInput, name, TAG_END)) != NULL)
  {
    if(EXD_IS_FILE(exd))
    {
      if((file = Open(name, MODE_OLDFILE)))
        *size = exd->FileSize;
    }

    FreeDosObject(DOS_EXAMINEDATA, exd);
  }

  RETURN(file);
  return file;
}

#elif defined(__MORPHOS__)
static BPTR openIn(STRPTR name, ULONG *size)
{
  struct FileInfoBlock fib;
  BPTR file;

  ENTER();

  if((file = Open(name, MODE_OLDFILE)))
  {
    if(!ExamineFH(file, &fib))
    {
      Close(file);
      file = 0;
    }
    else
    {
      *size = fib.fib_Size;
    }
  }

  RETURN(file);
  return file;
}
#else
static BPTR openIn(STRPTR name, ULONG * size)
{
  struct FileInfoBlock *fib;
  BPTR file;

  ENTER();

  if((fib = AllocDosObject(DOS_FIB,NULL)) != NULL)
  {
    if((file = Open(name, MODE_OLDFILE)))
    {
      if(!ExamineFH(file, fib))
      {
        Close(file);
        file = 0;
      }
      else
      {
        *size = fib->fib_Size;
      }
    }

    FreeDosObject(DOS_FIB, fib);
  }
  else
    file = 0;

  RETURN(file);
  return file;
}
#endif

/****************************************************************************/

static int inchar(struct b64 *b64)
{
  int c;

  ENTER();

  if(b64->flags & B64FLG_SourceFile)
  {
    if((c = FGetC((BPTR)b64->in)) == EOF)
    {
      if(IoErr() != 0)
        b64->error = CSR_B64_ERROR_DOS;
    }
  }
  else
  {
    if(b64->inAvailable == 0)
    {
      RETURN(EOF);
      return EOF;
    }

    c = ((STRPTR)b64->in)[b64->inIndex++];
    b64->inAvailable--;
  }

  RETURN(c);
  return c;
}

/****************************************************************************/

static int ochar(struct b64 *b64, int c)
{
  ENTER();

  if(b64->flags & B64FLG_DestFile)
  {
    int r = 0;

    if(b64->maxLineLen && (b64->lineCounter>=b64->maxLineLen))
    {
      r = FPuts((BPTR)b64->out,b64->eols);
      b64->lineCounter = 0;
    }

    if(!r)
    {
      r = FPutC((BPTR)b64->out,c);
    }

    if(r==EOF)
    {
      b64->error = CSR_B64_ERROR_DOS;

      RETURN(EOF);
      return EOF;
    }

    b64->lineCounter++;
  }
  else
  {
    ((STRPTR)b64->out)[b64->outIndex++] = c;
  }

  RETURN(0);
  return 0;
}

/****************************************************************************/

static int ostring(struct b64 *b64, UBYTE * buf, int s)
{
  int i;

  ENTER();

  if(b64->flags & B64FLG_DestFile)
  {
    int r;

    for(r = i = 0; (r!=EOF) && (i<s); i++)
    {
      r = FPutC((BPTR)b64->out,buf[i]);
    }

    if(r==EOF)
    {
      b64->error = CSR_B64_ERROR_DOS;

      RETURN(EOF);
      return EOF;
    }
  }
  else
  {
    for(i = 0; i<s; i++)
      ((STRPTR)b64->out)[b64->outIndex++] = buf[i];
  }

  RETURN(0);
  return 0;
}

/****************************************************************************/

static int insig(struct b64 *b64)
{
  int c;

  for(;;)
  {
    c = inchar(b64);

    if((c==EOF) || (c>' '))
    {
      RETURN(c);
      return c;
    }
  }
}

/****************************************************************************/

LIBPROTO(CodesetsEncodeB64A, ULONG, REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, struct TagItem *attrs))
{
  struct b64     b64;
  struct TagItem *tag;
  STRPTR         source;
  APTR           dest, in, out;
  ULONG          totSize, stop, flags;
  #if defined(__amigaos4__)
  int64          size;
  #else
  ULONG          size;
  #endif
  int            sourceLen = 0, maxLineLen;

  flags = 0;

  if((tag = FindTagItem(CSA_B64SourceFile, attrs)) != NULL)
  {
    source = (STRPTR)tag->ti_Data;
    flags |= B64FLG_SourceFile;
  }
  else
  {
    if((source = (STRPTR)GetTagData(CSA_B64SourceString, 0, attrs)) == NULL)
    {
      RETURN(CSR_B64_ERROR_MEM);
      return CSR_B64_ERROR_MEM;
    }

    if((tag = FindTagItem(CSA_B64SourceLen, attrs)) != NULL)
      sourceLen = tag->ti_Data;
    else
      sourceLen = strlen(source);
  }

  if((tag = FindTagItem(CSA_B64DestFile, attrs)) != NULL)
  {
    dest = (APTR)tag->ti_Data;
    flags |= B64FLG_DestFile;
  }
  else
  {
    if((dest = (APTR)GetTagData(CSA_B64DestPtr, 0, attrs)) == NULL)
    {
      RETURN(CSR_B64_ERROR_MEM);
      return CSR_B64_ERROR_MEM;
    }
  }

  maxLineLen = GetTagData(CSA_B64MaxLineLen,0,attrs);
  if(maxLineLen<=0 || maxLineLen>=256)
    maxLineLen = MAXLINELEN;

  if(GetTagData(CSA_B64Unix,TRUE,attrs))
    flags |= B64FLG_Unix;

  /* source */
  if(flags & B64FLG_SourceFile)
  {
    if(!(in = (APTR)openIn(source,&size)))
    {
      RETURN(CSR_B64_ERROR_DOS);
      return CSR_B64_ERROR_DOS;
    }

    b64.inAvailable = 0;
  }
  else
  {
    in = source;
    size = sourceLen;
    b64.inAvailable = size;
  }

  totSize = size<<1;

  /* dest */
  if(flags & B64FLG_DestFile)
  {
    if(!(out = (APTR)Open(dest,MODE_NEWFILE)))
    {
      RETURN(CSR_B64_ERROR_DOS);
      return CSR_B64_ERROR_DOS;
    }
  }
  else
  {
    if(!totSize)
      totSize = 8;

    if((out = allocArbitrateVecPooled(totSize)) == NULL)
    {
      if(flags & B64FLG_SourceFile)
        Close((BPTR)in);

      RETURN(CSR_B64_ERROR_MEM);
      return CSR_B64_ERROR_MEM;
    }

    *((STRPTR *)dest) = out;
  }

  /* set globals */
  b64.in          = in;
  b64.out         = out;
  b64.flags       = flags;
  b64.inIndex     = 0;
  b64.outIndex    = 0;
  b64.maxLineLen  = maxLineLen;
  b64.lineCounter = 0;
  b64.eols        = (flags & B64FLG_Unix) ? "\n" : "\r\n";
  b64.error       = 0;

  /* encode */
  stop = FALSE;
  while(stop == FALSE)
  {
    UBYTE    igroup[3], ogroup[4];
    int i, c, n;

    igroup[0] = igroup[1] = igroup[2] = 0;

    for(n = 0; n<3; n++)
    {
      c = inchar(&b64);
      if(c==EOF)
      {
        stop = TRUE;
        break;
      }

      igroup[n] = (UBYTE) c;
    }

    if(n>0)
    {
      ogroup[0] = etable[igroup[0]>>2];
      ogroup[1] = etable[((igroup[0] & 3)<<4) | (igroup[1]>>4)];
      ogroup[2] = etable[((igroup[1] & 0xF)<<2) | (igroup[2]>>6)];
      ogroup[3] = etable[igroup[2] & 0x3F];

      if(n<3)
      {
        ogroup[3] = '=';
        if(n<2)
          ogroup[2] = '=';
      }

      for(i=0; i<4; i++)
      {
        c = ochar(&b64,ogroup[i]);
        if(c==EOF)
        {
          stop = TRUE;
          break;
        }
      }
    }
  }

  if(!(b64.flags & B64FLG_DestFile))
    ((STRPTR)out)[b64.outIndex] = 0;

  /* close source */
  if(flags & B64FLG_SourceFile)
    Close((BPTR)in);

  /* flush and close dest */
  if(flags & B64FLG_DestFile)
  {
    if(!b64.error)
    {
      if(FPuts((BPTR)out,b64.eols)==EOF)
        b64.error = CSR_B64_ERROR_DOS;

      #if defined(__amigaos4__)
      FFlush((BPTR)out);
      #else
      Flush((BPTR)out);
      #endif
    }

    Close((BPTR)out);
  }
  else
  {
    if(b64.error)
      freeArbitrateVecPooled(out);
  }

  RETURN((ULONG)b64.error);
  return (ULONG)b64.error;
}

#if defined(__amigaos4__)
LIBPROTOVA(CodesetsEncodeB64, ULONG, REG(a6, UNUSED __BASE_OR_IFACE), ...)
{
  ULONG res;
  VA_LIST args;

  VA_START(args, ICodesets);
  res = CodesetsEncodeB64A(VA_ARG(args, struct TagItem *));
  VA_END(args);

  return res;
}
#endif

/****************************************************************************/

LIBPROTO(CodesetsDecodeB64A, ULONG, REG(a6, UNUSED __BASE_OR_IFACE), REG(a0, struct TagItem *attrs))
{
  struct b64              b64;
  struct TagItem *tag;
  STRPTR         source;
  APTR           dest, in, out;
  ULONG          totSize, flags, errcheck;
  #if defined(__amigaos4__)
  int64          size;
  #else
  ULONG          size;
  #endif
  int            sourceLen = 0;

  ENTER();

  flags = 0;

  if((tag = FindTagItem(CSA_B64SourceFile, attrs)) != NULL)
  {
    source = (STRPTR)tag->ti_Data;
    flags |= B64FLG_SourceFile;
  }
  else
  {
    if ((source = (STRPTR)GetTagData(CSA_B64SourceString, 0, attrs)) == NULL)
    {
      RETURN(CSR_B64_ERROR_MEM);
      return CSR_B64_ERROR_MEM;
    }

    if((tag = FindTagItem(CSA_B64SourceLen, attrs)) != NULL)
      sourceLen = tag->ti_Data;
    else
      sourceLen = strlen(source);
  }

  if((tag = FindTagItem(CSA_B64DestFile, attrs)) != NULL)
  {
    dest = (APTR)tag->ti_Data;
    flags |= B64FLG_DestFile;
  }
  else
  {
    if((dest = (APTR)GetTagData(CSA_B64DestPtr, 0, attrs)) == NULL)
    {
      RETURN(CSR_B64_ERROR_MEM);
      return CSR_B64_ERROR_MEM;
    }
  }

  /* source */
  if(flags & B64FLG_SourceFile)
  {
    if(!(in = (APTR)openIn(source, &size)))
    {
      RETURN(CSR_B64_ERROR_DOS);
      return CSR_B64_ERROR_DOS;
    }

    b64.inAvailable = 0;
  }
  else
  {
    in = source;
    size = sourceLen;
    b64.inAvailable = size;
  }

  totSize = size<<1;

  /* dest */
  if(flags & B64FLG_DestFile)
  {
    if(!(out = (APTR)Open(dest, MODE_NEWFILE)))
    {
      RETURN(CSR_B64_ERROR_DOS);
      return CSR_B64_ERROR_DOS;
    }
  }
  else
  {
    if(!totSize)
      totSize = 8;

    if(!(out = allocArbitrateVecPooled(totSize)))
    {
      if(flags & B64FLG_SourceFile)
        Close((BPTR)in);

      RETURN(CSR_B64_ERROR_MEM);
      return CSR_B64_ERROR_MEM;
    }

    *((STRPTR *)dest) = out;
  }

  b64.in          = in;
  b64.out         = out;
  b64.flags       = flags;
  b64.inIndex     = 0;
  b64.outIndex    = 0;
  b64.maxLineLen  = 0;
  b64.lineCounter = 0;
  b64.eols        = NULL;
  b64.error       = 0;

  /* parse error check */
  errcheck = !GetTagData(CSA_B64FLG_NtCheckErr, FALSE, attrs);

  /* decode */
  for(;;)
  {
    UBYTE a[4], b[4], o[3];
    int i;

    for(i = 0; i<4; i++)
    {
      int c = insig (&b64);

      if(c==EOF)
      {
        if(!(b64.flags & B64FLG_DestFile))
          ((STRPTR)out)[b64.outIndex] = 0;

        if(!b64.error && errcheck && (i>0))
          b64.error = CSR_B64_ERROR_INCOMPLETE;

        goto end;
      }

      if(dtable[c] & 0x80)
      {
        if(errcheck)
        {
          b64.error = CSR_B64_ERROR_ILLEGAL;

          goto end;
        }

        i--;
        continue;
      }

      a[i] = (UBYTE) c;
      b[i] = (UBYTE) dtable[c];
    }

    o[0] = (b[0]<<2) | (b[1]>>4);
    o[1] = (b[1]<<4) | (b[2]>>2);
    o[2] = (b[2]<<6) | b[3];

    i = a[2]=='=' ? 1 : (a[3]=='=' ? 2 : 3);

    if(ostring(&b64,o,i)==EOF || i < 3)
      goto end;
  }

end:

  /* close source */
  if(flags & B64FLG_SourceFile)
    Close((BPTR)in);

  /* flush and close dest */
  if(flags & B64FLG_DestFile)
  {
    if(!b64.error)
    {
      if(FPuts((BPTR)out,b64.eols)==EOF)
        b64.error = CSR_B64_ERROR_DOS;

      #if defined(__amigaos4__)
      FFlush((BPTR)out);
      #else
      Flush((BPTR)out);
      #endif
    }

    Close((BPTR)out);
  }
  else
  {
    if(b64.error)
      freeArbitrateVecPooled(out);
  }

  RETURN((ULONG)b64.error);
  return (ULONG)b64.error;
}

#if defined(__amigaos4__)
LIBPROTOVA(CodesetsDecodeB64, ULONG, REG(a6, UNUSED __BASE_OR_IFACE), ...)
{
  ULONG res;
  VA_LIST args;

  VA_START(args, ICodesets);
  res = CodesetsDecodeB64A(VA_ARG(args, struct TagItem *));
  VA_END(args);

  return res;
}
#endif

/****************************************************************************/
