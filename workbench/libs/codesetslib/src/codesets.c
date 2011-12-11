/***************************************************************************

 codesets.library - Amiga shared library for handling different codesets
 Copyright (C) 2001-2005 by Alfonso [alfie] Ranieri <alforan@tin.it>.
 Copyright (C) 2005-2010 by codesets.library Open Source Team

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

#include "lib.h"

#include <clib/alib_protos.h>

#include <diskfont/glyph.h>
#include <diskfont/diskfonttag.h>
#include <proto/diskfont.h>
#include <ctype.h>
#include <limits.h>

#ifdef __MORPHOS__
#include <proto/keymap.h>
#include <proto/locale.h>
#endif

#include "codesets_table.h"
#include "convertUTF.h"
#include "codepages.h"

#include <SDI/SDI_stdarg.h>

#include "debug.h"

/**************************************************************************/

// a union used for various type casts while avoiding the annoying "dereferencing
// type punned pointer is breaking strict alias rules" warnings of GCC4+
union TypeAliases
{
  void **voidptr;
  char **schar;
  unsigned char **uchar;
  STRPTR *strptr;
  UTF8 **utf8;
  const UTF8 **cutf8;
  UTF16 **utf16;
  const UTF16 **cutf16;
  UTF32 **utf32;
  const UTF32 **cutf32;
};

/// BIN_SEARCH()
// search a sorted array in O(log n) e.g.
// BIN_SEARCH(strings,0,sizeof(strings)/sizeof(strings[0]),strcmp(key,array[mid]),res);
#define BIN_SEARCH(array,low,high,compare,result) \
  {\
    int l = low;\
    int h = high;\
    int m = (low+high)/2;\
    result = NULL;\
    while (l<=h)\
    {\
      int d = compare;\
      if (!d){ result = &array[m]; break; }\
      if (d < 0) h = m - 1;\
      else l = m + 1;\
      m = (l + h)/2;\
    }\
  }

///
/// mystrdup()
static STRPTR mystrdup(const char *str)
{
  STRPTR newStr = NULL;

  ENTER();

  if(str != NULL)
  {
    int len;

    if((len = strlen(str)) > 0)
    {
      if((newStr = allocArbitrateVecPooled(len+1)) != NULL)
        strlcpy(newStr, str, len+1);
    }
  }

  RETURN(newStr);
  return newStr;
}

///
/// mystrndup()
static STRPTR mystrndup(const char *str1, int n)
{
  STRPTR dest;

  ENTER();

  if((dest = allocArbitrateVecPooled(n+1)) != NULL)
  {
    if(str1 != NULL)
      strlcpy(dest, str1, n+1);
    else
      dest[0] = '\0';

    dest[n] = '\0';
  }

  RETURN(dest);
  return dest;
}

///
/// readLine()
static BOOL readLine(BPTR fh, char *buf, ULONG size)
{
  BOOL success = FALSE;
  char *c;

  ENTER();

  if((c = FGets(fh, buf, size)) != NULL)
  {
    // we succeeded in reading something
    success = TRUE;

    // now find the end of the line and strip the LF/CR character
    for(; *c; c++)
    {
      if(*c == '\n' || *c == '\r')
      {
        *c = '\0';
        break;
      }
    }
  }

  RETURN(success);
  return success;
}

///
/// getConfigItem()
static const char *getConfigItem(const char *buf, const char *item)
{
  const char *configItem = NULL;
  int len;

  ENTER();

  len = strlen(item);

  if(strnicmp(buf, item, len) == 0)
  {
    char c;

    buf += len;

    // skip spaces
    while((c = *buf) != '\0' && isspace(c))
      buf++;

    if(*buf == '=')
    {
      buf++;

      // skip spaces
      while((c = *buf) != '\0'  && isspace(c))
        buf++;

      configItem = buf;
    }
  }

  RETURN(configItem);
  return configItem;
}

///
/// parseUtf8()
static int parseUtf8(STRPTR *ps)
{
  STRPTR s = *ps;
  int wc, n, i;

  ENTER();

  if(*s<0x80)
  {
    *ps = s+1;

    RETURN(*s);
    return *s;
  }

  if(*s<0xc2)
  {
    RETURN(-1);
    return -1;
  }
  else
  {
    if(*s<0xe0)
    {
      if((s[1] & 0xc0)!=0x80)
      {
        RETURN(-1);
        return -1;
      }

      *ps = s+2;

      RETURN(((s[0] & 0x1f)<<6) | (s[1] & 0x3f));
      return ((s[0] & 0x1f)<<6) | (s[1] & 0x3f);
    }
    else
    {
      if(*s<0xf0)
      {
        n = 3;
      }
      else
      {
        if(*s<0xf8)
        {
          n = 4;
        }
        else
        {
          if(*s<0xfc)
          {
            n = 5;
          }
          else
          {
            if(*s<0xfe)
            {
              n = 6;
            }
            else
            {
              RETURN(-1);
              return -1;
            }
          }
        }
      }
    }
  }

  wc = *s++ & ((1<<(7-n))-1);

  for(i = 1; i<n; i++)
  {
    if((*s & 0xc0) != 0x80)
    {
      RETURN(-1);
      return -1;
    }

    wc = (wc << 6) | (*s++ & 0x3f);
  }

  if(wc < (1 << (5 * n - 4)))
  {
    RETURN(-1);
    return -1;
  }

  *ps = s;

  RETURN(wc);
  return wc;
}

///
/// countCodesets()
static int countCodesets(struct codesetList *csList)
{
  struct Node *node;
  int num = 0;

  for(node = GetHead((struct List *)csList); node != NULL; node = GetSucc(node))
    num++;

  return num;
}

///
/// mapUTF8toASCII()
// in case some UTF8 sequences can not be converted during CodesetsUTF8ToStrA(), this
// function is used to replace these unknown sequences with lookalike characters that
// still make the text more readable. For more replacement see
// http://www.utf8-zeichentabelle.de/unicode-utf8-table.pl
//
// The conversion table in this function is partly borrowed from the awebcharset plugin
// written by Frank Weber. See http://cvs.sunsite.dk/viewcvs.cgi/aweb/plugins/charset/awebcharset.c
//
struct UTF8Replacement
{
  const char *utf8;     // the original UTF8 string we are going to replace
  const int utf8len;    // the length of the UTF8 string
  const char *rep;      // pointer to the replacement string
  const int replen;     // the length of the replacement string (minus for signalling an UTF8 string)
};

static int compareUTF8Replacements(const void *p1, const void *p2)
{
  struct UTF8Replacement *key = (struct UTF8Replacement *)p1;
  struct UTF8Replacement *rep = (struct UTF8Replacement *)p2;
  int cmp;

  // compare the length first, after that compare the strings
  cmp = key->utf8len - rep->utf8len;
  if(cmp == 0)
    cmp = memcmp(key->utf8, rep->utf8, key->utf8len);

  return cmp;
}

static int mapUTF8toASCII(const char **dst, const unsigned char *src, const int utf8len)
{
  int len = 0;
  struct UTF8Replacement key = { (char *)src, utf8len, NULL, 0 };
  struct UTF8Replacement *rep;

  static struct UTF8Replacement const utf8map[] =
  {
    // U+0100 ... U+017F (Latin Extended-A)
    { "\xC4\x80", 2, "A",         1 }, // U+0100 -> A       (LATIN CAPITAL LETTER A WITH MACRON)
    { "\xC4\x81", 2, "a",         1 }, // U+0101 -> a       (LATIN SMALL LETTER A WITH MACRON)
    { "\xC4\x82", 2, "A",         1 }, // U+0102 -> A       (LATIN CAPITAL LETTER A WITH BREVE)
    { "\xC4\x83", 2, "a",         1 }, // U+0103 -> a       (LATIN SMALL LETTER A WITH BREVE)
    { "\xC4\x84", 2, "A",         1 }, // U+0104 -> A       (LATIN CAPITAL LETTER A WITH OGONEK)
    { "\xC4\x85", 2, "a",         1 }, // U+0105 -> a       (LATIN SMALL LETTER A WITH OGONEK)
    { "\xC4\x86", 2, "C",         1 }, // U+0106 -> C       (LATIN CAPITAL LETTER C WITH ACUTE)
    { "\xC4\x87", 2, "c",         1 }, // U+0107 -> c       (LATIN SMALL LETTER C WITH ACUTE)
    { "\xC4\x88", 2, "C",         1 }, // U+0108 -> C       (LATIN CAPITAL LETTER C WITH CIRCUMFLEX)
    { "\xC4\x89", 2, "c",         1 }, // U+0109 -> c       (LATIN SMALL LETTER C WITH CIRCUMFLEX)
    { "\xC4\x8A", 2, "C",         1 }, // U+010A -> C       (LATIN CAPITAL LETTER C WITH DOT ABOVE)
    { "\xC4\x8B", 2, "c",         1 }, // U+010B -> c       (LATIN SMALL LETTER C WITH DOT ABOVE)
    { "\xC4\x8C", 2, "C",         1 }, // U+010C -> C       (LATIN CAPITAL LETTER C WITH CARON)
    { "\xC4\x8D", 2, "c",         1 }, // U+010D -> c       (LATIN SMALL LETTER C WITH CARON)
    { "\xC4\x8E", 2, "D",         1 }, // U+010E -> D       (LATIN CAPITAL LETTER D WITH CARON)
    { "\xC4\x8F", 2, "d",         1 }, // U+010F -> d       (LATIN SMALL LETTER D WITH CARON)
    { "\xC4\x90", 2, "D",         1 }, // U+0110 -> D       (LATIN CAPITAL LETTER D WITH STROKE)
    { "\xC4\x91", 2, "d",         1 }, // U+0111 -> d       (LATIN SMALL LETTER D WITH STROKE)
    { "\xC4\x92", 2, "E",         1 }, // U+0112 -> E       (LATIN CAPITAL LETTER E WITH MACRON)
    { "\xC4\x93", 2, "e",         1 }, // U+0113 -> e       (LATIN SMALL LETTER E WITH MACRON)
    { "\xC4\x94", 2, "E",         1 }, // U+0114 -> E       (LATIN CAPITAL LETTER E WITH BREVE)
    { "\xC4\x95", 2, "e",         1 }, // U+0115 -> e       (LATIN SMALL LETTER E WITH BREVE)
    { "\xC4\x96", 2, "E",         1 }, // U+0116 -> E       (LATIN CAPITAL LETTER E WITH DOT ABOVE)
    { "\xC4\x97", 2, "e",         1 }, // U+0117 -> e       (LATIN SMALL LETTER E WITH DOT ABOVE)
    { "\xC4\x98", 2, "E",         1 }, // U+0118 -> E       (LATIN CAPITAL LETTER E WITH OGONEK)
    { "\xC4\x99", 2, "e",         1 }, // U+0119 -> e       (LATIN SMALL LETTER E WITH OGONEK)
    { "\xC4\x9A", 2, "E",         1 }, // U+011A -> E       (LATIN CAPITAL LETTER E WITH CARON)
    { "\xC4\x9B", 2, "e",         1 }, // U+011B -> e       (LATIN SMALL LETTER E WITH CARON)
    { "\xC4\x9C", 2, "G",         1 }, // U+011C -> G       (LATIN CAPITAL LETTER G WITH CIRCUMFLEX)
    { "\xC4\x9D", 2, "g",         1 }, // U+011D -> g       (LATIN SMALL LETTER G WITH CIRCUMFLEX)
    { "\xC4\x9E", 2, "G",         1 }, // U+011E -> G       (LATIN CAPITAL LETTER G WITH BREVE)
    { "\xC4\x9F", 2, "g",         1 }, // U+011F -> g       (LATIN SMALL LETTER G WITH BREVE)
    { "\xC4\xA0", 2, "G",         1 }, // U+0120 -> G       (LATIN CAPITAL LETTER G WITH DOT ABOVE)
    { "\xC4\xA1", 2, "g",         1 }, // U+0121 -> g       (LATIN SMALL LETTER G WITH DOT ABOVE)
    { "\xC4\xA2", 2, "G",         1 }, // U+0122 -> G       (LATIN CAPITAL LETTER G WITH CEDILLA)
    { "\xC4\xA3", 2, "g",         1 }, // U+0123 -> g       (LATIN SMALL LETTER G WITH CEDILLA)
    { "\xC4\xA4", 2, "H",         1 }, // U+0124 -> H       (LATIN CAPITAL LETTER H WITH CIRCUMFLEX)
    { "\xC4\xA5", 2, "h",         1 }, // U+0125 -> h       (LATIN SMALL LETTER H WITH CIRCUMFLEX)
    { "\xC4\xA6", 2, "H",         1 }, // U+0126 -> H       (LATIN CAPITAL LETTER H WITH STROKE)
    { "\xC4\xA7", 2, "h",         1 }, // U+0127 -> h       (LATIN SMALL LETTER H WITH STROKE)
    { "\xC4\xA8", 2, "I",         1 }, // U+0128 -> I       (LATIN CAPITAL LETTER I WITH TILDE)
    { "\xC4\xA9", 2, "i",         1 }, // U+0129 -> i       (LATIN SMALL LETTER I WITH TILDE)
    { "\xC4\xAA", 2, "I",         1 }, // U+012A -> I       (LATIN CAPITAL LETTER I WITH MACRON)
    { "\xC4\xAB", 2, "i",         1 }, // U+012B -> i       (LATIN SMALL LETTER I WITH MACRON)
    { "\xC4\xAC", 2, "I",         1 }, // U+012C -> I       (LATIN CAPITAL LETTER I WITH BREVE)
    { "\xC4\xAD", 2, "i",         1 }, // U+012D -> i       (LATIN SMALL LETTER I WITH BREVE)
    { "\xC4\xAE", 2, "I",         1 }, // U+012E -> I       (LATIN CAPITAL LETTER I WITH OGONEK)
    { "\xC4\xAF", 2, "i",         1 }, // U+012F -> i       (LATIN SMALL LETTER I WITH OGONEK)
    { "\xC4\xB0", 2, "I",         1 }, // U+0130 -> I       (LATIN CAPITAL LETTER I WITH DOT ABOVE)
    { "\xC4\xB1", 2, "i",         1 }, // U+0131 -> i       (LATIN SMALL LETTER DOTLESS I)
    { "\xC4\xB2", 2, "Ij",        2 }, // U+0132 -> Ij      (LATIN CAPITAL LIGATURE IJ)
    { "\xC4\xB3", 2, "ij",        2 }, // U+0133 -> ij      (LATIN SMALL LIGATURE IJ)
    { "\xC4\xB4", 2, "J",         1 }, // U+0134 -> J       (LATIN CAPITAL LETTER J WITH CIRCUMFLEX)
    { "\xC4\xB5", 2, "j",         1 }, // U+0135 -> j       (LATIN SMALL LETTER J WITH CIRCUMFLEX)
    { "\xC4\xB6", 2, "K",         1 }, // U+0136 -> K       (LATIN CAPITAL LETTER K WITH CEDILLA)
    { "\xC4\xB7", 2, "k",         1 }, // U+0137 -> k       (LATIN SMALL LETTER K WITH CEDILLA)
    { "\xC4\xB8", 2, "k",         1 }, // U+0138 -> k       (LATIN SMALL LETTER KRA)
    { "\xC4\xB9", 2, "L",         1 }, // U+0139 -> L       (LATIN CAPITAL LETTER L WITH ACUTE)
    { "\xC4\xBA", 2, "l",         1 }, // U+013A -> l       (LATIN SMALL LETTER L WITH ACUTE)
    { "\xC4\xBB", 2, "L",         1 }, // U+013B -> L       (LATIN CAPITAL LETTER L WITH CEDILLA)
    { "\xC4\xBC", 2, "l",         1 }, // U+013C -> l       (LATIN SMALL LETTER L WITH CEDILLA)
    { "\xC4\xBD", 2, "L",         1 }, // U+013D -> L       (LATIN CAPITAL LETTER L WITH CARON)
    { "\xC4\xBE", 2, "l",         1 }, // U+013E -> l       (LATIN SMALL LETTER L WITH CARON)
    { "\xC4\xBF", 2, "L",         1 }, // U+013F -> L       (LATIN CAPITAL LETTER L WITH MIDDLE DOT)
    { "\xC5\x80", 2, "l",         1 }, // U+0140 -> l       (LATIN SMALL LETTER L WITH MIDDLE DOT)
    { "\xC5\x81", 2, "L",         1 }, // U+0141 -> L       (LATIN CAPITAL LETTER L WITH STROKE)
    { "\xC5\x82", 2, "l",         1 }, // U+0142 -> l       (LATIN SMALL LETTER L WITH STROKE)
    { "\xC5\x83", 2, "N",         1 }, // U+0143 -> N       (LATIN CAPITAL LETTER N WITH ACUTE)
    { "\xC5\x84", 2, "n",         1 }, // U+0144 -> n       (LATIN SMALL LETTER N WITH ACUTE)
    { "\xC5\x85", 2, "N",         1 }, // U+0145 -> N       (LATIN CAPITAL LETTER N WITH CEDILLA)
    { "\xC5\x86", 2, "n",         1 }, // U+0146 -> n       (LATIN SMALL LETTER N WITH CEDILLA)
    { "\xC5\x87", 2, "N",         1 }, // U+0147 -> N       (LATIN CAPITAL LETTER N WITH CARON)
    { "\xC5\x88", 2, "n",         1 }, // U+0148 -> n       (LATIN SMALL LETTER N WITH CARON)
    { "\xC5\x89", 2, "'n",        2 }, // U+0149 -> 'n      (LATIN SMALL LETTER N PRECEDED BY APOSTROPHE)
    { "\xC5\x8A", 2, "Ng",        2 }, // U+014A -> Ng      (LATIN CAPITAL LETTER ENG)
    { "\xC5\x8B", 2, "ng",        2 }, // U+014B -> ng      (LATIN SMALL LETTER ENG)
    { "\xC5\x8C", 2, "O",         1 }, // U+014C -> O       (LATIN CAPITAL LETTER O WITH MACRON)
    { "\xC5\x8D", 2, "o",         1 }, // U+014D -> o       (LATIN SMALL LETTER O WITH MACRON)
    { "\xC5\x8E", 2, "O",         1 }, // U+014E -> O       (LATIN CAPITAL LETTER O WITH BREVE)
    { "\xC5\x8F", 2, "o",         1 }, // U+014F -> o       (LATIN SMALL LETTER O WITH BREVE)
    { "\xC5\x90", 2, "O",         1 }, // U+0150 -> O       (LATIN CAPITAL LETTER O WITH DOUBLE ACUTE)
    { "\xC5\x91", 2, "o",         1 }, // U+0151 -> o       (LATIN SMALL LETTER O WITH DOUBLE ACUTE)
    { "\xC5\x92", 2, "Oe",        2 }, // U+0152 -> Oe      (LATIN CAPITAL LIGATURE OE)
    { "\xC5\x93", 2, "oe",        2 }, // U+0153 -> oe      (LATIN SMALL LIGATURE OE)
    { "\xC5\x94", 2, "R",         1 }, // U+0154 -> R       (LATIN CAPITAL LETTER R WITH ACUTE)
    { "\xC5\x95", 2, "r",         1 }, // U+0155 -> r       (LATIN SMALL LETTER R WITH ACUTE)
    { "\xC5\x96", 2, "R",         1 }, // U+0156 -> R       (LATIN CAPITAL LETTER R WITH CEDILLA)
    { "\xC5\x97", 2, "r",         1 }, // U+0157 -> r       (LATIN SMALL LETTER R WITH CEDILLA)
    { "\xC5\x98", 2, "R",         1 }, // U+0158 -> R       (LATIN CAPITAL LETTER R WITH CARON)
    { "\xC5\x99", 2, "r",         1 }, // U+0159 -> r       (LATIN SMALL LETTER R WITH CARON)
    { "\xC5\x9A", 2, "S",         1 }, // U+015A -> S       (LATIN CAPITAL LETTER S WITH ACUTE)
    { "\xC5\x9B", 2, "s",         1 }, // U+015B -> s       (LATIN SMALL LETTER S WITH ACUTE)
    { "\xC5\x9C", 2, "S",         1 }, // U+015C -> S       (LATIN CAPITAL LETTER S WITH CIRCUMFLEX)
    { "\xC5\x9D", 2, "s",         1 }, // U+015D -> s       (LATIN SMALL LETTER S WITH CIRCUMFLEX)
    { "\xC5\x9E", 2, "S",         1 }, // U+015E -> S       (LATIN CAPITAL LETTER S WITH CEDILLA)
    { "\xC5\x9F", 2, "s",         1 }, // U+015F -> s       (LATIN SMALL LETTER S WITH CEDILLA)
    { "\xC5\xA0", 2, "S",         1 }, // U+0160 -> S       (LATIN CAPITAL LETTER S WITH CARON)
    { "\xC5\xA1", 2, "s",         1 }, // U+0161 -> s       (LATIN SMALL LETTER S WITH CARON)
    { "\xC5\xA2", 2, "T",         1 }, // U+0162 -> T       (LATIN CAPITAL LETTER T WITH CEDILLA)
    { "\xC5\xA3", 2, "t",         1 }, // U+0163 -> t       (LATIN SMALL LETTER T WITH CEDILLA)
    { "\xC5\xA4", 2, "T",         1 }, // U+0164 -> T       (LATIN CAPITAL LETTER T WITH CARON)
    { "\xC5\xA5", 2, "t",         1 }, // U+0165 -> t       (LATIN SMALL LETTER T WITH CARON)
    { "\xC5\xA6", 2, "T",         1 }, // U+0166 -> T       (LATIN CAPITAL LETTER T WITH STROKE)
    { "\xC5\xA7", 2, "t",         1 }, // U+0167 -> t       (LATIN SMALL LETTER T WITH STROKE)
    { "\xC5\xA8", 2, "U",         1 }, // U+0168 -> U       (LATIN CAPITAL LETTER U WITH TILDE)
    { "\xC5\xA9", 2, "u",         1 }, // U+0169 -> u       (LATIN SMALL LETTER U WITH TILDE)
    { "\xC5\xAA", 2, "U",         1 }, // U+016A -> U       (LATIN CAPITAL LETTER U WITH MACRON)
    { "\xC5\xAB", 2, "u",         1 }, // U+016B -> u       (LATIN SMALL LETTER U WITH MACRON)
    { "\xC5\xAC", 2, "U",         1 }, // U+016C -> U       (LATIN CAPITAL LETTER U WITH BREVE)
    { "\xC5\xAD", 2, "u",         1 }, // U+016D -> u       (LATIN SMALL LETTER U WITH BREVE)
    { "\xC5\xAE", 2, "U",         1 }, // U+016E -> U       (LATIN CAPITAL LETTER U WITH RING ABOVE)
    { "\xC5\xAF", 2, "u",         1 }, // U+016F -> u       (LATIN SMALL LETTER U WITH RING ABOVE)
    { "\xC5\xB0", 2, "U",         1 }, // U+0170 -> U       (LATIN CAPITAL LETTER U WITH DOUBLE ACUTE)
    { "\xC5\xB1", 2, "u",         1 }, // U+0171 -> u       (LATIN SMALL LETTER U WITH DOUBLE ACUTE)
    { "\xC5\xB2", 2, "U",         1 }, // U+0172 -> U       (LATIN CAPITAL LETTER U WITH OGONEK)
    { "\xC5\xB3", 2, "u",         1 }, // U+0173 -> u       (LATIN SMALL LETTER U WITH OGONEK)
    { "\xC5\xB4", 2, "W",         1 }, // U+0174 -> W       (LATIN CAPITAL LETTER W WITH CIRCUMFLEX)
    { "\xC5\xB5", 2, "w",         1 }, // U+0175 -> w       (LATIN SMALL LETTER W WITH CIRCUMFLEX)
    { "\xC5\xB6", 2, "Y",         1 }, // U+0176 -> Y       (LATIN CAPITAL LETTER Y WITH CIRCUMFLEX)
    { "\xC5\xB7", 2, "y",         1 }, // U+0177 -> y       (LATIN SMALL LETTER Y WITH CIRCUMFLEX)
    { "\xC5\xB8", 2, "Y",         1 }, // U+0178 -> Y       (LATIN CAPITAL LETTER Y WITH DIAERESIS)
    { "\xC5\xB9", 2, "Z",         1 }, // U+0179 -> Z       (LATIN CAPITAL LETTER Z WITH ACUTE)
    { "\xC5\xBA", 2, "z",         1 }, // U+017A -> z       (LATIN SMALL LETTER Z WITH ACUTE)
    { "\xC5\xBB", 2, "Z",         1 }, // U+017B -> Z       (LATIN CAPITAL LETTER Z WITH DOT ABOVE)
    { "\xC5\xBC", 2, "z",         1 }, // U+017C -> z       (LATIN SMALL LETTER Z WITH DOT ABOVE)
    { "\xC5\xBD", 2, "Z",         1 }, // U+017D -> Z       (LATIN CAPITAL LETTER Z WITH CARON)
    { "\xC5\xBE", 2, "z",         1 }, // U+017E -> z       (LATIN SMALL LETTER Z WITH CARON)
    { "\xC5\xBF", 2, "s",         1 }, // U+017F -> s       (LATIN SMALL LETTER LONG S

    // U+2000 ... U+206F (General Punctuation)
    { "\xE2\x80\x90", 3, "-",         1 }, // U+2010 -> -       (HYPHEN)
    { "\xE2\x80\x91", 3, "-",         1 }, // U+2011 -> -       (NON-BREAKING HYPHEN)
    { "\xE2\x80\x92", 3, "--",        2 }, // U+2012 -> --      (FIGURE DASH)
    { "\xE2\x80\x93", 3, "--",        2 }, // U+2013 -> --      (EN DASH)
    { "\xE2\x80\x94", 3, "---",       3 }, // U+2014 -> ---     (EM DASH)
    { "\xE2\x80\x95", 3, "---",       3 }, // U+2015 -> ---     (HORIZONTAL BAR)
    { "\xE2\x80\x96", 3, "||",        2 }, // U+2016 -> ||      (DOUBLE VERTICAL LINE)
    { "\xE2\x80\x97", 3, "_",         1 }, // U+2017 -> _       (DOUBLE LOW LINE)
    { "\xE2\x80\x98", 3, "`",         1 }, // U+2018 -> `       (LEFT SINGLE QUOTATION MARK)
    { "\xE2\x80\x99", 3, "'",         1 }, // U+2019 -> '       (RIGHT SINGLE QUOTATION MARK)
    { "\xE2\x80\x9A", 3, ",",         1 }, // U+201A -> ,       (SINGLE LOW-9 QUOTATION MARK)
    { "\xE2\x80\x9B", 3, "'",         1 }, // U+201B -> '       (SINGLE HIGH-REVERSED-9 QUOTATION MARK)
    { "\xE2\x80\x9C", 3, "\"",        1 }, // U+201C -> "       (LEFT DOUBLE QUOTATION MARK)
    { "\xE2\x80\x9D", 3, "\"",        1 }, // U+201D -> "       (RIGHT DOUBLE QUOTATION MARK)
    { "\xE2\x80\x9E", 3, ",,",        2 }, // U+201E -> ,,      (DOUBLE LOW-9 QUOTATION MARK)
    { "\xE2\x80\x9F", 3, "``",        2 }, // U+201F -> ``      (DOUBLE HIGH-REVERSED-9 QUOTATION MARK)
    { "\xE2\x80\xA0", 3, "+",         1 }, // U+2020 -> +       (DAGGER)
    { "\xE2\x80\xA1", 3, "+",         1 }, // U+2021 -> +       (DOUBLE DAGGER)
    { "\xE2\x80\xA2", 3, "\xC2\xB7", -2 }, // U+2022 -> U+00B7  (BULLET) -> (MIDDLE POINT)
    { "\xE2\x80\xA3", 3, ".",         1 }, // U+2023 -> .       (TRIANGULAR BULLET)
    { "\xE2\x80\xA4", 3, ".",         1 }, // U+2024 -> .       (ONE DOT LEADER)
    { "\xE2\x80\xA5", 3, "..",        2 }, // U+2025 -> ..      (TWO DOT LEADER)
    { "\xE2\x80\xA6", 3, "...",       3 }, // U+2026 -> ...     (HORIZONTAL ELLIPSIS)
    { "\xE2\x80\xA7", 3, "\xC2\xB7", -2 }, // U+2027 -> U+00B7  (HYPHENATION POINT) -> (MIDDLE POINT)
    { "\xE2\x80\xB0", 3, "%.",        2 }, // U+2030 -> %.      (PER MILLE SIGN)
    { "\xE2\x80\xB1", 3, "%..",       3 }, // U+2031 -> %..     (PER TEN THOUSAND SIGN)
    { "\xE2\x80\xB2", 3, "'",         1 }, // U+2032 -> `       (PRIME)
    { "\xE2\x80\xB3", 3, "''",        2 }, // U+2033 -> ''      (DOUBLE PRIME)
    { "\xE2\x80\xB4", 3, "'''",       3 }, // U+2034 -> '''     (TRIPLE PRIME)
    { "\xE2\x80\xB5", 3, "`",         1 }, // U+2035 -> `       (REVERSED PRIME)
    { "\xE2\x80\xB6", 3, "``",        2 }, // U+2036 -> ``      (REVERSED DOUBLE PRIME)
    { "\xE2\x80\xB7", 3, "```",       3 }, // U+2037 -> ```     (REVERSED TRIPLE PRIME)
    { "\xE2\x80\xB8", 3, "^",         1 }, // U+2038 -> ^       (CARET)
    { "\xE2\x80\xB9", 3, "<",         1 }, // U+2039 -> <       (SINGLE LEFT-POINTING ANGLE QUOTATION MARK)
    { "\xE2\x80\xBA", 3, ">",         1 }, // U+203A -> >       (SINGLE RIGHT-POINTING ANGLE QUOTATION MARK)
    { "\xE2\x80\xBB", 3, "\xC3\x97", -2 }, // U+203B -> U+00D7  (REFERENCE MARK) -> (MULTIPLICATION SIGN)
    { "\xE2\x80\xBC", 3, "!!",        2 }, // U+203C -> !!      (DOUBLE EXCLAMATION MARK)
    { "\xE2\x80\xBD", 3, "?",         1 }, // U+203D -> ?       (INTERROBANG)
    { "\xE2\x81\x82", 3, "*",         1 }, // U+2042 -> *       (ASTERISM)
    { "\xE2\x81\x83", 3, ".",         1 }, // U+2043 -> .       (HYPHEN BULLET)
    { "\xE2\x81\x84", 3, "/",         1 }, // U+2044 -> /       (FRACTION SLASH)
    { "\xE2\x81\x87", 3, "??",        2 }, // U+2047 -> ??      (DOUBLE QUESTION MARK)
    { "\xE2\x81\x88", 3, "?!",        2 }, // U+2048 -> ?!      (QUESTION EXCLAMATION MARK)
    { "\xE2\x81\x89", 3, "!?",        2 }, // U+2049 -> !?      (EXCLAMATION QUESTION MARK)
    { "\xE2\x81\x8E", 3, "*",         1 }, // U+204E -> *       (LOW ASTERISK)
    { "\xE2\x81\x8F", 3, ";",         1 }, // U+204F -> ;       (REVERSED SEMICOLON)
    { "\xE2\x81\x91", 3, "*",         1 }, // U+2051 -> *       (TWO ASTERISKS ALIGNED VERTICALLY)
    { "\xE2\x81\x92", 3, "-",         1 }, // U+2052 -> -       (COMMERCIAL MINUS SIGN)
    { "\xE2\x81\x93", 3, "~",         1 }, // U+2053 -> ~       (SWUNG DASH)
    { "\xE2\x81\x95", 3, "*",         1 }, // U+2055 -> *       (FLOWER PUNCTUATION MARK)
    { "\xE2\x81\x97", 3, "''''",      4 }, // U+2057 -> ''''    (QUADRUPLE PRIME)
    { "\xE2\x81\x9A", 3, ":",         1 }, // U+205A -> :       (TWO DOT PUNCTUATION)
    { "\xE2\x81\x9C", 3, "+",         1 }, // U+205C -> +       (DOTTED CROSS)

    // U+20A0 ... U+20CF (Currency Symbols)
    { "\xE2\x82\xA0", 3, "ECU",       3 }, // U+20A0 -> ECU     (EURO-CURRENCY SIGN)
    { "\xE2\x82\xA1", 3, "CRC",       3 }, // U+20A1 -> CRC     (COLON SIGN)
    { "\xE2\x82\xA2", 3, "BRC",       3 }, // U+20A2 -> BRC     (CRUZEIRO SIGN)
    { "\xE2\x82\xA3", 3, "BEF",       3 }, // U+20A3 -> BEF     (FRENCH FRANC SIGN)
    { "\xE2\x82\xA4", 3, "ITL",       3 }, // U+20A4 -> ITL     (LIRA SIGN)
    { "\xE2\x82\xA6", 3, "NGN",       3 }, // U+20A6 -> NGN     (NEIRA SIGN)
    { "\xE2\x82\xA7", 3, "ESP",       3 }, // U+20A7 -> ESP     (PESETA SIGN)
    { "\xE2\x82\xA8", 3, "MVQ",       3 }, // U+20A8 -> MVQ     (RUPEE SIGN)
    { "\xE2\x82\xA9", 3, "KPW",       3 }, // U+20A9 -> KPW     (WON SIGN)
    { "\xE2\x82\xAA", 3, "ILS",       3 }, // U+20AA -> ILS     (NEW SHEQEL SIGN)
    { "\xE2\x82\xAB", 3, "VNC",       3 }, // U+20AB -> VNC     (DONG SIGN)
    { "\xE2\x82\xAC", 3, "EUR",       3 }, // U+20AC -> EUR     (EURO SIGN)
    { "\xE2\x82\xAD", 3, "LAK",       3 }, // U+20AD -> LAK     (KIP SIGN)
    { "\xE2\x82\xAE", 3, "MNT",       3 }, // U+20AE -> MNT     (TUGRIK SIGN)
    { "\xE2\x82\xAF", 3, "GRD",       3 }, // U+20AF -> GRD     (DRACHMA SIGN)
    { "\xE2\x82\xB0", 3, "Pf",        2 }, // U+20B0 -> Pf      (GERMAN PENNY SIGN)
    { "\xE2\x82\xB1", 3, "P",         1 }, // U+20B1 -> P       (PESO SIGN)
    { "\xE2\x82\xB2", 3, "PYG",       3 }, // U+20B2 -> PYG     (GUARANI SIGN)
    { "\xE2\x82\xB3", 3, "ARA",       3 }, // U+20B3 -> ARA     (AUSTRAL SIGN)
    { "\xE2\x82\xB4", 3, "UAH",       3 }, // U+20B4 -> UAH     (HRYVNIA SIGN)
    { "\xE2\x82\xB5", 3, "GHS",       3 }, // U+20B5 -> GHS     (CEDI SIGN)

    // U+2190 ... U+21FF (Arrows)
    { "\xE2\x86\x90", 3, "<-",        2 }, // U+2190 -> <-      (LEFTWARDS ARROW)
    { "\xE2\x86\x92", 3, "->",        2 }, // U+2192 -> ->      (RIGHTWARDS ARROW)
  };

  ENTER();

  // start with no replacement string
  *dst = NULL;

  // perform a binary search in the lookup table
  if((rep = bsearch(&key, utf8map, sizeof(utf8map) / sizeof(utf8map[0]), sizeof(utf8map[0]), compareUTF8Replacements)) != NULL)
  {
    // if we found something, then copy this over to the result variables
    *dst = rep->rep;
    len = rep->replen;
  }

  RETURN(len);
  return len;
}

///
/// matchCodesetAlias()
//
struct CodesetAliases
{
  const char *MIMEname;   // The official and correct MIME name for a codeset
  const char *Aliases;    // A space separated array with well-known aliases
};

const struct CodesetAliases codesetAliases[] =
{
  // MIME name       Aliases
  { "Amiga-1251",   "Ami1251 Amiga1251"  },
  { "AmigaPL",      "AmiPL Amiga-PL"     },
  { "ISO-8859-1",   "ISO8859-1 8859-1" },
  { "ISO-8859-2",   "ISO8859-2 8859-2" },
  { "ISO-8859-3",   "ISO8859-3 8859-3" },
  { "ISO-8859-4",   "ISO8859-4 8859-4" },
  { "ISO-8859-5",   "ISO8859-5 8859-5" },
  { "ISO-8859-6",   "ISO8859-6 8859-6" },
  { "ISO-8859-7",   "ISO8859-7 8859-7" },
  { "ISO-8859-8",   "ISO8859-8 8859-8" },
  { "ISO-8859-9",   "ISO8859-9 8859-9" },
  { "ISO-8859-10",  "ISO8859-10 8859-10" },
  { "ISO-8859-11",  "ISO8859-11 8859-11" },
  { "ISO-8859-12",  "ISO8859-12 8859-12" },
  { "ISO-8859-13",  "ISO8859-13 8859-13" },
  { "ISO-8859-14",  "ISO8859-14 8859-14" },
  { "ISO-8859-15",  "ISO8859-15 8859-15" },
  { "ISO-8859-16",  "ISO8859-16 8859-16" },
  { "ISO-8859-10",  "ISO8859-10 8859-10" },
  { "KOI8-R",       "KOI8R" },
  { "US-ASCII",     "ASCII" },
  { "UTF-8",        "UTF8 UTF" },
  { "UTF-16",       "UTF16" },
  { "UTF-32",       "UTF32" },
  { "windows-1250", "cp1250 windows1250" },
  { "windows-1251", "cp1251 windows1251" },
  { "windows-1252", "cp1252 windows1252" },
  { "windows-1253", "cp1253 windows1253" },
  { "windows-1254", "cp1254 windows1254" },
  { "windows-1255", "cp1255 windows1255" },
  { "windows-1256", "cp1256 windows1256" },
  { "windows-1257", "cp1257 windows1257" },
  { NULL,           NULL,                }
};

static char *matchCodesetAlias(const char *search)
{
  char *result = NULL;
  size_t len = strlen(search);
  int i;

  ENTER();

  for(i=0; codesetAliases[i].MIMEname != NULL; i++)
  {
    BOOL found = FALSE;

    // search the MIMEname first
    if(stricmp(search, codesetAliases[i].MIMEname) == 0)
      found = TRUE;
    else
    {
      const char *s = codesetAliases[i].Aliases;

      // loop through space separated list of aliases
      while(s != NULL && *s != '\0')
      {
        if(strnicmp(search, s, len) == 0)
        {
          found = TRUE;
          break;
        }

        if((s = strpbrk(s, " ")) != NULL)
          s++;
      }
    }

    if(found == TRUE)
    {
      result = (char *)codesetAliases[i].MIMEname;

      break;
    }
  }

  RETURN(result);
  return result;
}

///

/**************************************************************************/

/// defaultCodeset()
static struct codeset *defaultCodeset(BOOL useSemaphore)
{
  char buf[256];
  struct codeset *codeset;

  ENTER();

  if(useSemaphore == TRUE)
    ObtainSemaphoreShared(&CodesetsBase->libSem);

  buf[0] = '\0';
  GetVar("codeset_default" ,buf, sizeof(buf), GVF_GLOBAL_ONLY);

  if(buf[0] == '\0' || (codeset = codesetsFind(&CodesetsBase->codesets, buf)) == NULL)
    codeset = CodesetsBase->systemCodeset;

  if(useSemaphore == TRUE)
    ReleaseSemaphore(&CodesetsBase->libSem);

  RETURN(codeset);
  return codeset;
}

///
/// codesetsCmpUnicode()
// The compare function
static int codesetsCmpUnicode(const void *a1, const void *a2)
{
  struct single_convert *arg1 = (struct single_convert *)a1;
  struct single_convert *arg2 = (struct single_convert *)a2;

  return strcmp((char*)&arg1->utf8[1], (char*)&arg2->utf8[1]);
}

///
/// codesetsReadTable()

#define ITEM_STANDARD           "Standard"
#define ITEM_ALTSTANDARD        "AltStandard"
#define ITEM_READONLY           "ReadOnly"
#define ITEM_CHARACTERIZATION   "Characterization"

// Reads a coding table and adds it
static BOOL codesetsReadTable(struct codesetList *csList, STRPTR name)
{
  BPTR fh;
  BOOL res = FALSE;

  ENTER();

  D(DBF_STARTUP, "trying to read charset file '%s'...", name);

  if((fh = Open(name, MODE_OLDFILE)) != (BPTR)NULL)
  {
    struct codeset *codeset;

    if((codeset = (struct codeset *)allocArbitrateVecPooled(sizeof(*codeset))) != NULL)
    {
      int i;
      char buf[512];

      memset(codeset, 0, sizeof(*codeset));

      for(i = 0; i<256; i++)
      {
        codeset->table[i].code = i;
        codeset->table[i].ucs4 = i;
      }

      while(readLine(fh, buf, sizeof(buf)) == TRUE)
      {
        const char *result;

        if(buf[0] != '#')
        {
          if((result = getConfigItem(buf, ITEM_STANDARD)) != NULL)
            codeset->name = mystrdup(result);
          else if(codeset->name == NULL) // a valid file starts with "Standard" and nothing else!!
            break;
          else if((result = getConfigItem(buf, ITEM_ALTSTANDARD)) != NULL)
            codeset->alt_name = mystrdup(result);
          else if((result = getConfigItem(buf, ITEM_READONLY)) != NULL)
            codeset->read_only = (atoi(result) == 0) ? 0 : 1;
          else if((result = getConfigItem(buf, ITEM_CHARACTERIZATION)) != NULL)
          {
            if(result[0] == '_' && result[1] == '(' && result[2] == '"')
            {
              char *end = strchr(result + 3, '"');

              if(end != NULL)
                codeset->characterization = mystrndup(result+3, end-(result+3));
            }
            else
              codeset->characterization = mystrdup(result);
          }
          else
          {
            char *p = buf;
            int fmt2 = 0;

            if(*p == '=' || (fmt2 = ((*p=='0') || (*(p+1)=='x'))))
            {
              p++;
              p += fmt2;

              i = strtol(p, &p, 16);
              if(i>0 && i<256)
              {
                while(isspace(*p))
                  p++;

                if(strnicmp(p, "U+", 2) == 0)
                {
                  p += 2;
                  codeset->table[i].ucs4 = strtol(p, &p, 16);
                }
                else if(*p != '#')
                {
                  codeset->table[i].ucs4 = strtol(p, &p, 0);
                }
              }
            }
          }
        }
      }

      // check if there is not already codeset with the same name in here
      if(codeset->name != NULL && codesetsFind(csList, codeset->name) == NULL)
      {
        for(i=0; i<256; i++)
        {
          UTF32 src = codeset->table[i].ucs4;
          UTF32 *src_ptr = &src;
          UTF8 *dest_ptr = &codeset->table[i].utf8[1];

          CodesetsConvertUTF32toUTF8((const UTF32 **)&src_ptr, src_ptr+1, &dest_ptr, dest_ptr+6, CSF_StrictConversion);
          *dest_ptr = 0;
          codeset->table[i].utf8[0] = (IPTR)dest_ptr-(IPTR)(&codeset->table[i].utf8[1]);
        }

        memcpy(codeset->table_sorted, codeset->table, sizeof(codeset->table));
        qsort(codeset->table_sorted, 256, sizeof(codeset->table[0]), codesetsCmpUnicode);
        D(DBF_STARTUP, "adding external codeset '%s'", codeset->name);
        AddTail((struct List *)csList, (struct Node *)&codeset->node);

        res = TRUE;
      }
      else
      {
        // cleanup
        if(codeset->name != NULL)
          freeArbitrateVecPooled(codeset->name);
        if(codeset->alt_name != NULL)
          freeArbitrateVecPooled(codeset->alt_name);
        if(codeset->characterization != NULL)
          freeArbitrateVecPooled(codeset->characterization);
        freeArbitrateVecPooled(codeset);
      }
    }

    Close(fh);
  }

  RETURN(res);
  return res;
}
///
/// codesetsScanDir()
static void codesetsScanDir(struct codesetList *csList, const char *dirPath)
{
  ENTER();

  if(dirPath != NULL && dirPath[0] != '\0')
  {
    #if defined(__amigaos4__)
    APTR dirContext;

    if((dirContext = ObtainDirContextTags(EX_StringNameInput, dirPath,
                                          EX_DataFields,      EXF_NAME|EXF_TYPE,
                                          TAG_END)) != NULL)
    {
      struct ExamineData *exd;

      D(DBF_STARTUP, "scanning directory '%s' for codesets tables", dirPath);

      while((exd = ExamineDir(dirContext)) != NULL)
      {
        if(EXD_IS_FILE(exd))
        {
          char filePath[620];

          strlcpy(filePath, dirPath, sizeof(filePath));
          AddPart(filePath, exd->Name, sizeof(filePath));

          D(DBF_STARTUP, "about to read codeset table '%s'", filePath);

          codesetsReadTable(csList, filePath);
        }
      }

      ReleaseDirContext(dirContext);
    }
    #else
    BPTR dirLock;

    if((dirLock = Lock(dirPath, ACCESS_READ)))
    {
      struct ExAllControl *eac;

      D(DBF_STARTUP, "scanning directory '%s' for codesets tables", dirPath);

      if((eac = AllocDosObject(DOS_EXALLCONTROL, NULL)) != NULL)
      {
        struct ExAllData *ead;
        struct ExAllData *eabuffer;
        LONG more;

        eac->eac_LastKey = 0;
        eac->eac_MatchString = NULL;
        eac->eac_MatchFunc = NULL;

        if((eabuffer = allocVecPooled(CodesetsBase->pool, 10*sizeof(struct ExAllData))) != NULL)
        {
          char filePath[620];

          do
          {
            more = ExAll(dirLock, eabuffer, 10*sizeof(struct ExAllData), ED_TYPE, eac);
            if(!more && IoErr() != ERROR_NO_MORE_ENTRIES)
              break;

            if(eac->eac_Entries == 0)
              continue;

            ead = (struct ExAllData *)eabuffer;
            do
            {
              // we only take that ead if it is a file (ed_Type < 0)
              if(ead->ed_Type < 0)
              {
                strlcpy(filePath, dirPath, sizeof(filePath));
                AddPart(filePath, (char *)ead->ed_Name, sizeof(filePath));

                D(DBF_STARTUP, "about to read codeset table '%s'", filePath);

                codesetsReadTable(csList, filePath);
              }
              ead = ead->ed_Next;
            }
            while(ead != NULL);
          }
          while(more);

          freeVecPooled(CodesetsBase->pool, eabuffer);
        }

        FreeDosObject(DOS_EXALLCONTROL, eac);
      }

      UnLock(dirLock);
    }
    #endif
  }

  LEAVE();
}

///
/// codesetsInit()
// Initialized and loads the codesets
BOOL codesetsInit(struct codesetList *csList)
{
  BOOL success = FALSE;
  struct codeset *codeset;
  UTF32 src;
  int i;
  #if defined(__amigaos4__)
  ULONG nextMIB = 3;
  #endif

  ENTER();

  NewList((struct List *)csList);

  // to make the list of the supported codesets complete we also add fake
  // 'UTF-8', 'UTF-16' and 'UTF-32' only so that our users can query for those codesets as well.
  if((codeset = allocArbitrateVecPooled(sizeof(*codeset))) == NULL)
    goto end;

  memset(codeset, 0, sizeof(*codeset));
  codeset->name             = mystrdup("UTF-8");
  codeset->alt_name         = mystrdup("UTF8");
  codeset->characterization = mystrdup("Unicode");
  codeset->read_only        = 0;
  D(DBF_STARTUP, "adding internal codeset 'UTF-8'");
  AddTail((struct List *)csList, (struct Node *)&codeset->node);
  CodesetsBase->utf8Codeset = codeset;

  if((codeset = allocArbitrateVecPooled(sizeof(*codeset))) == NULL)
    goto end;

  memset(codeset, 0, sizeof(*codeset));
  codeset->name             = mystrdup("UTF-16");
  codeset->alt_name         = mystrdup("UTF16");
  codeset->characterization = mystrdup("16-bit Unicode");
  codeset->read_only        = 0;
  D(DBF_STARTUP, "adding internal codeset 'UTF-16'");
  AddTail((struct List *)csList, (struct Node *)&codeset->node);
  CodesetsBase->utf16Codeset = codeset;

  if((codeset = allocArbitrateVecPooled(sizeof(*codeset))) == NULL)
    goto end;

  memset(codeset, 0, sizeof(*codeset));
  codeset->name             = mystrdup("UTF-32");
  codeset->alt_name         = mystrdup("UTF32");
  codeset->characterization = mystrdup("32-bit Unicode");
  codeset->read_only        = 0;
  D(DBF_STARTUP, "adding internal codeset 'UTF-32'");
  AddTail((struct List *)csList, (struct Node *)&codeset->node);
  CodesetsBase->utf32Codeset = codeset;

  // on AmigaOS4 we can use diskfont.library to inquire charset information as
  // it comes with a quite rich implementation of different charsets.
  #if defined(__amigaos4__)
  D(DBF_STARTUP, "OS4, asking diskfont.library for codesets");
  do
  {
    char *mimename;
    char *ianaName;
    ULONG *mapTable;
    ULONG curMIB = nextMIB;

    nextMIB = ObtainCharsetInfo(DFCS_NUMBER, curMIB, DFCS_NEXTNUMBER);
    if(nextMIB == 0)
      break;

    mapTable = (ULONG *)ObtainCharsetInfo(DFCS_NUMBER, curMIB, DFCS_MAPTABLE);
    mimename = (char *)ObtainCharsetInfo(DFCS_NUMBER, curMIB, DFCS_MIMENAME);
    ianaName = (char *)ObtainCharsetInfo(DFCS_NUMBER, curMIB, DFCS_NAME);
    if(mapTable != NULL && mimename != NULL && codesetsFind(csList, mimename) == NULL)
    {
      D(DBF_STARTUP, "loading charset '%s' from diskfont.library...", mimename);

      if((codeset = allocArbitrateVecPooled(sizeof(*codeset))) == NULL)
        goto end;

      codeset->name             = mystrdup(mimename);
      codeset->alt_name         = NULL;
      codeset->characterization = mystrdup(ianaName);
      codeset->read_only        = 0;

      for(i=0; i<256; i++)
      {
        UTF32 *src_ptr = &src;
        UTF8 *dest_ptr = &codeset->table[i].utf8[1];

        src = mapTable[i];

        codeset->table[i].code = i;
        codeset->table[i].ucs4 = src;
        CodesetsConvertUTF32toUTF8((const UTF32 **)&src_ptr, src_ptr+1, &dest_ptr, dest_ptr+6, CSF_StrictConversion);
        *dest_ptr = 0;
        codeset->table[i].utf8[0] = (IPTR)dest_ptr-(IPTR)&codeset->table[i].utf8[1];
      }

      memcpy(codeset->table_sorted, codeset->table, sizeof(codeset->table));
      qsort(codeset->table_sorted, 256, sizeof(codeset->table[0]), codesetsCmpUnicode);

      D(DBF_STARTUP, "adding diskfont.library codeset '%s'", codeset->name);
      AddTail((struct List *)csList, (struct Node *)&codeset->node);
    }
  }
  while(TRUE);
  #endif

  #if defined(__MORPHOS__)
  {
    struct Library *KeymapBase;
    struct Library *LocaleBase;
    // assume success at first
    BOOL success = TRUE;

    D(DBF_STARTUP, "MorphOS, asking keymap.library for codesets");
    if((KeymapBase = OpenLibrary("keymap.library", 51)) != NULL)
    {
      if((LocaleBase = OpenLibrary("locale.library", 51)) != NULL)
      {
        struct KeyMap *keymap = AskKeyMapDefault();
        // it doesn't matter if this call fails, as we don't depend on the system codesets
        CONST_STRPTR name = GetKeyMapCodepage(keymap);

        // legacy keymaps dont have codepage or Unicode mappings
        if(name != NULL && keymap != NULL)
        {
          D(DBF_STARTUP, "loading charset '%s' from keymap.library...", name);

          if((codeset = allocArbitrateVecPooled(sizeof(*codeset))) != NULL)
          {
             codeset->name             = mystrdup(name);
             codeset->alt_name         = NULL;
             codeset->characterization = mystrdup(name);  // No further information available
             codeset->read_only        = 0;

             for(i=0; i<256; i++)
             {
               UTF8 *dest_ptr = &codeset->table[i].utf8[1];
               LONG rc;

               codeset->table[i].code = i;
               codeset->table[i].ucs4 = src = ToUCS4(i, keymap);

               // here we use UTF8_Encode() instead of ConvertUCS4ToUTF8() because
               // of an internal bug in MorphOS 2.2.
               rc = UTF8_Encode(src, dest_ptr);
               rc = rc > 0 ? rc : 1;

               dest_ptr[rc] = '\0';
               codeset->table[i].utf8[0] = rc;
             }

             memcpy(codeset->table_sorted, codeset->table, sizeof(codeset->table));
             qsort(codeset->table_sorted, 256, sizeof(codeset->table[0]), codesetsCmpUnicode);

             D(DBF_STARTUP, "adding keymap.library codeset '%s'", codeset->name);
             AddTail((struct List *)csList, (struct Node *)&codeset->node);
          }
          else
          {
            // only failed memory allocations are treated as error
            success = FALSE;
          }
        }

        CloseLibrary(LocaleBase);
      }

      CloseLibrary(KeymapBase);
    }

    if(success == FALSE)
      goto end;
  }
  #endif

  D(DBF_STARTUP, "loading charsets from LIBS:Charsets...");

  // we try to walk to the LIBS:Charsets directory on our own and readin our
  // own charset tables
  codesetsScanDir(csList, "LIBS:Charsets");

  //
  // now we go and initialize our internally supported codesets but only if
  // we have not already loaded a charset with the same name
  //
  D(DBF_STARTUP, "initializing internal charsets...");

  // ISO-8859-1 + EURO
  if(codesetsFind(csList, "ISO-8859-1 + Euro") == NULL)
  {
    if((codeset = allocArbitrateVecPooled(sizeof(*codeset))) == NULL)
      goto end;

    codeset->name             = mystrdup("ISO-8859-1 + Euro");
    codeset->alt_name         = NULL;
    codeset->characterization = mystrdup("West European (with EURO)");
    codeset->read_only        = 1;

    for(i = 0; i<256; i++)
    {
      UTF32 *src_ptr = &src;
      UTF8 *dest_ptr = &codeset->table[i].utf8[1];

      if(i==164)
        src = 0x20AC; // the EURO sign
      else
        src = i;

      codeset->table[i].code = i;
      codeset->table[i].ucs4 = src;
      CodesetsConvertUTF32toUTF8((const UTF32 **)&src_ptr, src_ptr+1, &dest_ptr, dest_ptr+6, CSF_StrictConversion);
      *dest_ptr = 0;
      codeset->table[i].utf8[0] = (IPTR)dest_ptr-(IPTR)&codeset->table[i].utf8[1];
    }
    memcpy(codeset->table_sorted, codeset->table, sizeof(codeset->table));
    qsort(codeset->table_sorted, 256, sizeof(codeset->table[0]), codesetsCmpUnicode);

    D(DBF_STARTUP, "adding internal codeset '%s'", codeset->name);
    AddTail((struct List *)csList, (struct Node *)&codeset->node);
  }

  // ISO-8859-1
  if(codesetsFind(csList, "ISO-8859-1") == NULL)
  {
    if((codeset = allocArbitrateVecPooled(sizeof(*codeset))) == NULL)
      goto end;

    codeset->name             = mystrdup("ISO-8859-1");
    codeset->alt_name         = mystrdup("ISO8859-1");
    codeset->characterization = mystrdup("West European");
    codeset->read_only        = 0;

    for(i = 0; i<256; i++)
    {
      UTF32 *src_ptr = &src;
      UTF8 *dest_ptr = &codeset->table[i].utf8[1];

      src = i;

      codeset->table[i].code = i;
      codeset->table[i].ucs4 = src;
      CodesetsConvertUTF32toUTF8((const UTF32 **)&src_ptr, src_ptr+1, &dest_ptr, dest_ptr+6, CSF_StrictConversion);
      *dest_ptr = 0;
      codeset->table[i].utf8[0] = (IPTR)dest_ptr-(IPTR)&codeset->table[i].utf8[1];
    }
    memcpy(codeset->table_sorted, codeset->table, sizeof(codeset->table));
    qsort(codeset->table_sorted, 256, sizeof(codeset->table[0]), codesetsCmpUnicode);

    D(DBF_STARTUP, "adding internal codeset '%s'", codeset->name);
    AddTail((struct List *)csList, (struct Node *)&codeset->node);
  }

  // ISO-8859-2
  if(codesetsFind(csList, "ISO-8859-2") == NULL)
  {
    if((codeset = allocArbitrateVecPooled(sizeof(*codeset))) == NULL)
      goto end;

    codeset->name             = mystrdup("ISO-8859-2");
    codeset->alt_name         = mystrdup("ISO8859-2");
    codeset->characterization = mystrdup("Central/East European");
    codeset->read_only        = 0;

    for(i = 0; i<256; i++)
    {
      UTF32 *src_ptr = &src;
      UTF8 *dest_ptr = &codeset->table[i].utf8[1];

      if(i<0xa0)
        src = i;
      else
        src = iso_8859_2_to_ucs4[i-0xa0];

      codeset->table[i].code = i;
      codeset->table[i].ucs4 = src;
      CodesetsConvertUTF32toUTF8((const UTF32 **)&src_ptr, src_ptr+1, &dest_ptr,dest_ptr+6, CSF_StrictConversion);
      *dest_ptr = 0;
      codeset->table[i].utf8[0] = (IPTR)dest_ptr-(IPTR)&codeset->table[i].utf8[1];
    }
    memcpy(codeset->table_sorted, codeset->table, sizeof(codeset->table));
    qsort(codeset->table_sorted, 256, sizeof(codeset->table[0]), codesetsCmpUnicode);

    D(DBF_STARTUP, "adding internal codeset '%s'", codeset->name);
    AddTail((struct List *)csList, (struct Node *)&codeset->node);
  }

  // ISO-8859-3
  if(codesetsFind(csList, "ISO-8859-3") == NULL)
  {
    if((codeset = allocArbitrateVecPooled(sizeof(*codeset))) == NULL)
      goto end;

    codeset->name             = mystrdup("ISO-8859-3");
    codeset->alt_name         = mystrdup("ISO8859-3");
    codeset->characterization = mystrdup("South European");
    codeset->read_only        = 0;

    for(i = 0; i<256; i++)
    {
      UTF32 *src_ptr = &src;
      UTF8 *dest_ptr = &codeset->table[i].utf8[1];

      if(i<0xa0)
        src = i;
      else
        src = iso_8859_3_to_ucs4[i-0xa0];

      codeset->table[i].code = i;
      codeset->table[i].ucs4 = src;
      CodesetsConvertUTF32toUTF8((const UTF32 **)&src_ptr,src_ptr+1,&dest_ptr,dest_ptr+6,CSF_StrictConversion);
      *dest_ptr = 0;
      codeset->table[i].utf8[0] = (IPTR)dest_ptr-(IPTR)&codeset->table[i].utf8[1];
    }
    memcpy(codeset->table_sorted, codeset->table, sizeof(codeset->table));
    qsort(codeset->table_sorted, 256, sizeof(codeset->table[0]), codesetsCmpUnicode);

    D(DBF_STARTUP, "adding internal codeset '%s'", codeset->name);
    AddTail((struct List *)csList, (struct Node *)&codeset->node);
  }

  // ISO-8859-4
  if(codesetsFind(csList, "ISO-8859-4") == NULL)
  {
    if((codeset = allocArbitrateVecPooled(sizeof(*codeset))) == NULL)
      goto end;

    codeset->name             = mystrdup("ISO-8859-4");
    codeset->alt_name         = mystrdup("ISO8859-4");
    codeset->characterization = mystrdup("North European");
    codeset->read_only        = 0;

    for(i = 0; i<256; i++)
    {
      UTF32 *src_ptr = &src;
      UTF8 *dest_ptr = &codeset->table[i].utf8[1];

      if(i<0xa0)
        src = i;
      else
        src = iso_8859_4_to_ucs4[i-0xa0];

      codeset->table[i].code = i;
      codeset->table[i].ucs4 = src;
      CodesetsConvertUTF32toUTF8((const UTF32 **)&src_ptr,src_ptr+1,&dest_ptr,dest_ptr+6,CSF_StrictConversion);
      *dest_ptr = 0;
      codeset->table[i].utf8[0] = (IPTR)dest_ptr-(IPTR)&codeset->table[i].utf8[1];
    }
    memcpy(codeset->table_sorted, codeset->table, sizeof(codeset->table));
    qsort(codeset->table_sorted, 256, sizeof(codeset->table[0]), codesetsCmpUnicode);

    D(DBF_STARTUP, "adding internal codeset '%s'", codeset->name);
    AddTail((struct List *)csList, (struct Node *)&codeset->node);
  }

  // ISO-8859-5
  if(codesetsFind(csList, "ISO-8859-5") == NULL)
  {
    if((codeset = allocArbitrateVecPooled(sizeof(*codeset))) == NULL)
      goto end;

    codeset->name             = mystrdup("ISO-8859-5");
    codeset->alt_name         = mystrdup("ISO8859-5");
    codeset->characterization = mystrdup("Slavic languages");
    codeset->read_only        = 0;

    for(i = 0; i<256; i++)
    {
      UTF32 *src_ptr = &src;
      UTF8 *dest_ptr = &codeset->table[i].utf8[1];

      if(i<0xa0)
        src = i;
      else
        src = iso_8859_5_to_ucs4[i-0xa0];

      codeset->table[i].code = i;
      codeset->table[i].ucs4 = src;
      CodesetsConvertUTF32toUTF8((const UTF32 **)&src_ptr,src_ptr+1,&dest_ptr,dest_ptr+6,CSF_StrictConversion);
      *dest_ptr = 0;
      codeset->table[i].utf8[0] = (IPTR)dest_ptr-(IPTR)&codeset->table[i].utf8[1];
    }
    memcpy(codeset->table_sorted, codeset->table, sizeof(codeset->table));
    qsort(codeset->table_sorted, 256, sizeof(codeset->table[0]), codesetsCmpUnicode);

    D(DBF_STARTUP, "adding internal codeset '%s'", codeset->name);
    AddTail((struct List *)csList, (struct Node *)&codeset->node);
  }

  // ISO-8859-9
  if(codesetsFind(csList, "ISO-8859-9") == NULL)
  {
    if((codeset = allocArbitrateVecPooled(sizeof(*codeset))) == NULL)
      goto end;

    codeset->name             = mystrdup("ISO-8859-9");
    codeset->alt_name         = mystrdup("ISO8859-9");
    codeset->characterization = mystrdup("Turkish");
    codeset->read_only        = 0;

    for(i = 0; i<256; i++)
    {
      UTF32 *src_ptr = &src;
      UTF8 *dest_ptr = &codeset->table[i].utf8[1];

      if(i<0xa0)
        src = i;
      else
        src = iso_8859_9_to_ucs4[i-0xa0];

      codeset->table[i].code = i;
      codeset->table[i].ucs4 = src;
      CodesetsConvertUTF32toUTF8((const UTF32 **)&src_ptr,src_ptr+1,&dest_ptr,dest_ptr+6,CSF_StrictConversion);
      *dest_ptr = 0;
      codeset->table[i].utf8[0] = (IPTR)dest_ptr-(IPTR)&codeset->table[i].utf8[1];
    }
    memcpy(codeset->table_sorted, codeset->table, sizeof(codeset->table));
    qsort(codeset->table_sorted, 256, sizeof(codeset->table[0]), codesetsCmpUnicode);

    D(DBF_STARTUP, "adding internal codeset '%s'", codeset->name);
    AddTail((struct List *)csList, (struct Node *)&codeset->node);
  }

  // ISO-8859-15
  if(codesetsFind(csList, "ISO-8859-15") == NULL)
  {
    if((codeset = allocArbitrateVecPooled(sizeof(*codeset))) == NULL)
      goto end;

    codeset->name             = mystrdup("ISO-8859-15");
    codeset->alt_name         = mystrdup("ISO8859-15");
    codeset->characterization = mystrdup("West European II");
    codeset->read_only        = 0;

    for(i = 0; i<256; i++)
    {
      UTF32 *src_ptr = &src;
      UTF8 *dest_ptr = &codeset->table[i].utf8[1];

      if(i<0xa0)
        src = i;
      else
        src = iso_8859_15_to_ucs4[i-0xa0];

      codeset->table[i].code = i;
      codeset->table[i].ucs4 = src;
      CodesetsConvertUTF32toUTF8((const UTF32 **)&src_ptr,src_ptr+1,&dest_ptr,dest_ptr+6,CSF_StrictConversion);
      *dest_ptr = 0;
      codeset->table[i].utf8[0] = (IPTR)dest_ptr-(IPTR)&codeset->table[i].utf8[1];
    }
    memcpy(codeset->table_sorted,codeset->table,sizeof (codeset->table));
    qsort(codeset->table_sorted, 256, sizeof(codeset->table[0]), codesetsCmpUnicode);

    D(DBF_STARTUP, "adding internal codeset '%s'", codeset->name);
    AddTail((struct List *)csList, (struct Node *)&codeset->node);
  }

  // ISO-8859-16
  if(codesetsFind(csList, "ISO-8859-16") == NULL)
  {
    if((codeset = allocArbitrateVecPooled(sizeof(*codeset))) == NULL)
      goto end;

    codeset->name             = mystrdup("ISO-8859-16");
    codeset->alt_name         = mystrdup("ISO8869-16");
    codeset->characterization = mystrdup("South-Eastern European");
    codeset->read_only        = 0;

    for(i=0;i<256;i++)
    {
      UTF32 *src_ptr = &src;
      UTF8 *dest_ptr = &codeset->table[i].utf8[1];

      if(i < 0xa0)
        src = i;
      else
        src = iso_8859_16_to_ucs4[i-0xa0];

      codeset->table[i].code = i;
      codeset->table[i].ucs4 = src;
      CodesetsConvertUTF32toUTF8((const UTF32 **)&src_ptr, src_ptr+1, &dest_ptr, dest_ptr+6, CSF_StrictConversion);
      *dest_ptr = 0;
      codeset->table[i].utf8[0] = (IPTR)dest_ptr - (IPTR)&codeset->table[i].utf8[1];
    }
    memcpy(codeset->table_sorted, codeset->table, sizeof(codeset->table));
    qsort(codeset->table_sorted, 256, sizeof(codeset->table[0]), codesetsCmpUnicode);

    D(DBF_STARTUP, "adding internal codeset '%s'", codeset->name);
    AddTail((struct List *)csList, (struct Node *)&codeset->node);
  }

  // KOI8-R
  if(codesetsFind(csList, "KOI8-R") == NULL)
  {
    if((codeset = allocArbitrateVecPooled(sizeof(*codeset))) == NULL)
      goto end;

    codeset->name               = mystrdup("KOI8-R");
    codeset->alt_name           = mystrdup("KOI8R");
    codeset->characterization   = mystrdup("Russian");
    codeset->read_only          = 0;

    for(i = 0; i<256; i++)
    {
      UTF32 *src_ptr = &src;
      UTF8 *dest_ptr = &codeset->table[i].utf8[1];

      if(i<0x80)
        src = i;
      else
        src = koi8r_to_ucs4[i-0x80];

      codeset->table[i].code = i;
      codeset->table[i].ucs4 = src;
      CodesetsConvertUTF32toUTF8((const UTF32 **)&src_ptr,src_ptr+1,&dest_ptr,dest_ptr+6,CSF_StrictConversion);
      *dest_ptr = 0;
      codeset->table[i].utf8[0] = (IPTR)dest_ptr-(IPTR)&codeset->table[i].utf8[1];
    }
    memcpy(codeset->table_sorted, codeset->table, sizeof(codeset->table));
    qsort(codeset->table_sorted, 256, sizeof(codeset->table[0]), codesetsCmpUnicode);

    D(DBF_STARTUP, "adding internal codeset '%s'", codeset->name);
    AddTail((struct List *)csList, (struct Node *)&codeset->node);
  }

  // AmigaPL
  if(codesetsFind(csList, "AmigaPL") == NULL)
  {
    if((codeset = allocArbitrateVecPooled(sizeof(*codeset))) == NULL)
      goto end;

    codeset->name             = mystrdup("AmigaPL");
    codeset->alt_name         = mystrdup("AmiPL");
    codeset->characterization = mystrdup("Polish (Amiga)");
    codeset->read_only        = 1;

    for(i=0; i<256; i++)
    {
      UTF32 *src_ptr = &src;
      UTF8 *dest_ptr = &codeset->table[i].utf8[1];

      if(i<0xa0)
        src = i;
      else
        src = amigapl_to_ucs4[i-0xa0];

      codeset->table[i].code = i;
      codeset->table[i].ucs4 = src;
      CodesetsConvertUTF32toUTF8((const UTF32 **)&src_ptr,src_ptr+1,&dest_ptr,dest_ptr+6,CSF_StrictConversion);
      *dest_ptr = 0;
      codeset->table[i].utf8[0] = (IPTR)dest_ptr-(IPTR)&codeset->table[i].utf8[1];
    }
    memcpy(codeset->table_sorted, codeset->table, sizeof(codeset->table));
    qsort(codeset->table_sorted, 256, sizeof(codeset->table[0]), codesetsCmpUnicode);

    D(DBF_STARTUP, "adding internal codeset '%s'", codeset->name);
    AddTail((struct List *)csList, (struct Node *)&codeset->node);
  }

  // Amiga-1251
  if(codesetsFind(csList, "Amiga-1251") == NULL)
  {
    if((codeset = allocArbitrateVecPooled(sizeof(*codeset))) == NULL)
      goto end;

    codeset->name             = mystrdup("Amiga-1251");
    codeset->alt_name         = mystrdup("Ami1251");
    codeset->characterization = mystrdup("Cyrillic (Amiga)");
    codeset->read_only        = 1;

    for(i=0; i<256; i++)
    {
      UTF32 *src_ptr = &src;
      UTF8 *dest_ptr = &codeset->table[i].utf8[1];

      if(i < 0xa0)
        src = i;
      else
        src = amiga1251_to_ucs4[i-0xa0];

      codeset->table[i].code = i;
      codeset->table[i].ucs4 = src;
      CodesetsConvertUTF32toUTF8((const UTF32 **)&src_ptr, src_ptr+1, &dest_ptr, dest_ptr+6, CSF_StrictConversion);
      *dest_ptr = 0;
      codeset->table[i].utf8[0] = (char*)dest_ptr - (char*)&codeset->table[i].utf8[1];
    }
    memcpy(codeset->table_sorted, codeset->table, sizeof(codeset->table));
    qsort(codeset->table_sorted, 256, sizeof(codeset->table[0]), codesetsCmpUnicode);

    D(DBF_STARTUP, "adding internal codeset '%s'", codeset->name);
    AddTail((struct List *)csList, (struct Node *)&codeset->node);
  }

  success = TRUE;

end:
  RETURN(success);
  return success;
}

///
/// codesetsCleanup()
// Cleanup the memory for the codeset
void codesetsCleanup(struct codesetList *csList)
{
  struct codeset *code;

  ENTER();

  while((code = (struct codeset *)RemHead((struct List *)csList)) != NULL)
  {
    if(code->name != NULL)
      freeArbitrateVecPooled(code->name);
    if(code->alt_name != NULL)
      freeArbitrateVecPooled(code->alt_name);
    if(code->characterization != NULL)
      freeArbitrateVecPooled(code->characterization);

    freeArbitrateVecPooled(code);
  }

  LEAVE();
}

///
/// codesetsFind()
// Returns the given codeset.
struct codeset *codesetsFind(struct codesetList *csList, const char *name)
{
  struct codeset *res = NULL;

  ENTER();

  if(name != NULL && name[0] != '\0')
  {
    struct Node *node;
    char *matchedName = matchCodesetAlias(name);

    if(matchedName != NULL)
      name = matchedName;

    for(node = GetHead((struct List *)csList); node != NULL; node = GetSucc(node))
    {
      struct codeset *mstate = (struct codeset *)node;

      if(stricmp(name, mstate->name) == 0 ||
        (mstate->alt_name != NULL && stricmp(name, mstate->alt_name) == 0))
      {
        // break out
        res = mstate;
        break;
      }
    }
  }

  RETURN(res);
  return res;
}

///
/// checkTextAgainstSingleCodeset
// check how good a text can be represented by a specific codeset
static int checkTextAgainstSingleCodeset(CONST_STRPTR text, ULONG textLen, struct codeset *codeset)
{
  int errors = textLen;

  ENTER();

  if(codeset->read_only == 0 &&
     codeset != CodesetsBase->utf8Codeset &&
     codeset != CodesetsBase->utf16Codeset &&
     codeset != CodesetsBase->utf32Codeset)
  {
    CONST_STRPTR text_ptr = text;
    ULONG i;

    errors = 0;

    // the following identification/detection routine is NOT really smart.
    // we just see how each UTF8 string is the representation of each char
    // in our source text and then check if they are valid or not. As said,
    // not very smart, but we don't have anything better right now :(
    for(i=0; i < textLen; i++)
    {
      unsigned char c = *text_ptr++;

      if(c != '\0')
      {
        struct single_convert *f = &codeset->table[c];

        if(f->utf8[0] == 0x00 || f->utf8[1] == 0x00)
          errors++;
      }
      else
        break;
    }
  }
  else
    W(DBF_STARTUP, "codeset '%s' is either read-only (%ld) or UTF8/16/32 (%ld)", codeset->name, codeset->read_only, codeset == CodesetsBase->utf8Codeset || codeset == CodesetsBase->utf16Codeset || codeset == CodesetsBase->utf32Codeset);

  D(DBF_STARTUP, "tried to identify text as '%s' text with %ld of %ld errors", codeset->name, errors, textLen);

  RETURN(errors);
  return errors;
}

///
/// checkTextAgainstCodesetList
static int checkTextAgainstCodesetList(CONST_STRPTR text, ULONG textLen, struct codesetList *csList, struct codeset **bestCodeset)
{
  struct Node *node;
  int bestErrors = textLen;

  ENTER();

  *bestCodeset = NULL;

  for(node = GetHead((struct List *)csList); node != NULL; node = GetSucc(node))
  {
    struct codeset *codeset = (struct codeset *)node;
    int errors;

    errors = checkTextAgainstSingleCodeset(text, textLen, codeset);
    if(errors < bestErrors)
    {
      *bestCodeset = codeset;
      bestErrors = errors;

      if(bestErrors == 0)
        break;
    }
  }

  RETURN(bestErrors);
  return bestErrors;
}

///
/// codesetsFindBest()
// Returns the best codeset for the given text
static struct codeset *codesetsFindBest(struct TagItem *attrs, ULONG csFamily, CONST_STRPTR text, ULONG textLen, int *errorPtr)
{
  struct codeset *bestCodeset = NULL;
  int bestErrors = textLen;
  BOOL found = FALSE;

  ENTER();

  ObtainSemaphoreShared(&CodesetsBase->libSem);

  // in case the user specified the codeset family as a
  // cyrillic one we go and do our cyrillic specific analysis first
  if(csFamily == CSV_CodesetFamily_Cyrillic)
  {
    #define NUM_CYRILLIC 3

    struct CodesetSearch
    {
      const char *name;
      const char *data;
    };

    struct CodesetSearch search[NUM_CYRILLIC];
    unsigned char *p;
    unsigned char *tp;
    int ctr[NUM_CYRILLIC];
    int Nmax;
    int NGlob = 1;
    int max;
    int gr = 0;
    int lr = 0;

    D(DBF_STARTUP, "performing cyrillic analysis");

    search[0].name = "windows-1251";
    search[0].data = cp1251_data;
    search[1].name = "IBM866";
    search[1].data = cp866_data;
    search[2].name = "KOI8-R";
    search[2].data = koi8r_data;

    memset(&ctr, 0, sizeof(ctr));

    tp = (unsigned char *)text;

    do
    {
      int n;
      int mid = max = -466725766; // TODO: what's the magic behind this constant?
      Nmax = 0;

      for(n=0; n < NUM_CYRILLIC; n++)
      {
        unsigned char la = 0;
        unsigned char *tptr = (unsigned char *)search[n].data;

        p = tp;

        do
        {
          unsigned char lb = (*p++) ^ 128;

          if(!((la | lb) & 128))
            ctr[n] += (signed char)tptr[(la << 7) + lb];

          la = lb;
        }
        while(*p);

        if(max < ctr[n])
        {
          mid = max;
          max = ctr[n];
          Nmax = n+1;
        }
      }

      tp = p;
      if((max >= 500) && ((max-mid) >= 1000))
      {
        lr = gr = 1;
        NGlob = Nmax;
      }
    }
    while((*p) && (!gr));

    if(gr || ((!(*p)) && lr))
      Nmax = NGlob;

    // if our analysis found something, we go and try
    // to find the corresponding codeset in out codeset list
    if(max != 0)
    {
      struct TagItem *tstate = attrs;
      struct TagItem *tag;

      D(DBF_STARTUP, "identified text as '%s", search[Nmax-1].name);

      // now we walk through our taglist and check if the user
      // supplied
      while((tag = NextTagItem((APTR)&tstate)) != NULL)
      {
        if(tag->ti_Tag == CSA_CodesetList && tag->ti_Data != 0)
        {
          struct codesetList *csList = (struct codesetList *)tag->ti_Data;

          if((bestCodeset = codesetsFind(csList, search[Nmax-1].name)) != NULL)
            break;
        }
      }

      // if we still haven't found the matching codeset
      // we search the internal list
      if(bestCodeset == NULL)
        bestCodeset = codesetsFind(&CodesetsBase->codesets, search[Nmax-1].name);

      bestErrors = 0;

      found = TRUE;
    }
  }

  // if we haven't found the best codeset (through the cyrillic analysis)
  // we go and do the dumb latin search in our codesetlist
  if(found == FALSE)
  {
    struct TagItem *tstate = attrs;
    struct TagItem *tag;

    // check text against all codesets in all supplied lists of codesets
    while((tag = NextTagItem((APTR)&tstate)) != NULL)
    {
      switch(tag->ti_Tag)
      {
        case CSA_CodesetList:
        {
          struct codesetList *csList = (struct codesetList *)tag->ti_Data;
          struct codeset *bestCodesetInList;
          int bestErrorsInList;

          D(DBF_STARTUP, "checking against external codeset list");
          bestErrorsInList = checkTextAgainstCodesetList(text, textLen, csList, &bestCodesetInList);
          if(bestErrorsInList < bestErrors && bestCodesetInList != NULL)
          {
            bestCodeset = bestCodesetInList;
            bestErrors = bestErrorsInList;

            if(bestErrors == 0)
              break;
          }
        }
        break;
      }
    }

    // we didn't find a "best" codeset in the supplied codesets lists so far,
    // so now we check against our internal list
    if(bestErrors != 0)
    {
      struct codeset *bestCodesetInList;
      int bestErrorsInList;

      D(DBF_STARTUP, "checking against internal codeset list");
      bestErrorsInList = checkTextAgainstCodesetList(text, textLen, &CodesetsBase->codesets, &bestCodesetInList);
      if(bestErrorsInList < bestErrors && bestCodesetInList != NULL)
      {
        bestCodeset = bestCodesetInList;
        bestErrors = bestErrorsInList;
      }
    }
  }

  ReleaseSemaphore(&CodesetsBase->libSem);

  if(errorPtr != NULL)
    *errorPtr = bestErrors;

  RETURN(bestCodeset);
  return bestCodeset;
}

///

/**************************************************************************/

/// CodesetsSupportedA()
STRPTR * LIBFUNC CodesetsSupportedA(REG(a0, UNUSED struct TagItem *attrs))
{
  STRPTR *array = NULL;
  struct TagItem *tstate = attrs;
  struct TagItem *tag;
  int numCodesets;

  ENTER();

  ObtainSemaphoreShared(&CodesetsBase->libSem);

  // first we need to check how many codesets our supplied
  // lists carry.
  numCodesets = countCodesets(&CodesetsBase->codesets);
  while((tag = NextTagItem((APTR)&tstate)) != NULL)
  {
    switch(tag->ti_Tag)
    {
      case CSA_CodesetList:
      {
        numCodesets += countCodesets((struct codesetList *)tag->ti_Data);
      }
      break;
    }
  }

  // now that we know how many codesets we have in our lists we
  // can put their names into our string arrays
  if(numCodesets > 0)
  {
    if((array = allocArbitrateVecPooled((numCodesets+1)*sizeof(STRPTR))) != NULL)
    {
      struct Node *node;
      int i=0;

      // first we walk through the internal codesets list and
      // add the names
      for(node = GetHead((struct List *)&CodesetsBase->codesets); node != NULL; node = GetSucc(node))
      {
        struct codeset *code = (struct codeset *)node;

        array[i] = code->name;
        i++;
      }

      // reset the tstate
      tstate = attrs;

      // then we also iterate through our private codesets list
      while((tag = NextTagItem((APTR)&tstate)) != NULL)
      {
        switch(tag->ti_Tag)
        {
          case CSA_CodesetList:
          {
            for(node = GetHead((struct List *)tag->ti_Data); node != NULL; node = GetSucc(node))
            {
              struct codeset *code = (struct codeset *)node;

              array[i] = code->name;
              i++;
            }
          }
          break;
        }
      }

      array[i] = NULL;
    }
  }

  ReleaseSemaphore(&CodesetsBase->libSem);

  RETURN(array);
  return array;
}

///
/// CodesetsFreeA()
void LIBFUNC CodesetsFreeA(REG(a0, APTR obj), REG(a1, UNUSED struct TagItem *attrs))
{
  ENTER();

  if(obj != NULL)
    freeArbitrateVecPooled(obj);

  LEAVE();
}

///
/// CodesetsSetDefaultA()
struct codeset * LIBFUNC CodesetsSetDefaultA(REG(a0, STRPTR name), REG(a1, struct TagItem *attrs))
{
  struct codeset *codeset;

  ENTER();

  ObtainSemaphoreShared(&CodesetsBase->libSem);

  if((codeset = codesetsFind(&CodesetsBase->codesets, name)) != NULL)
  {
    ULONG flags;

    flags = GVF_SAVE_VAR;
    if(GetTagData(CSA_Save, FALSE, attrs))
      SET_FLAG(flags, GVF_GLOBAL_ONLY);

    SetVar("codeset_default", codeset->name, strlen(codeset->name), flags);
  }

  ReleaseSemaphore(&CodesetsBase->libSem);

  RETURN(codeset);
  return codeset;
}

///
/// CodesetsFindA()
struct codeset * LIBFUNC CodesetsFindA(REG(a0, STRPTR name), REG(a1, struct TagItem *attrs))
{
  struct codeset *codeset = NULL;

  ENTER();

  ObtainSemaphoreShared(&CodesetsBase->libSem);

  // if no name pointer was supplied we have to return
  // the default codeset only.
  if(name != NULL)
  {
    // we first walk through our internal list and check if we
    // can find the requested codeset
    codeset = codesetsFind(&CodesetsBase->codesets, name);

    if(codeset == NULL)
    {
      struct TagItem *tstate = attrs;
      struct TagItem *tag;

      // now we walk through our taglist and check if the user
      // supplied
      while((tag = NextTagItem((APTR)&tstate)) != NULL)
      {
        if(tag->ti_Tag == CSA_CodesetList && tag->ti_Data != 0)
        {
          struct codesetList *csList = (struct codesetList *)tag->ti_Data;

          if((codeset = codesetsFind(csList, name)) != NULL)
            break;
        }
      }
    }
  }

  // check if we found something or not.
  if(codeset == NULL && GetTagData(CSA_FallbackToDefault, TRUE, attrs))
    codeset = defaultCodeset(FALSE);

  ReleaseSemaphore(&CodesetsBase->libSem);

  RETURN(codeset);
  return codeset;
}

///
/// CodesetsFindBestA()
struct codeset * LIBFUNC CodesetsFindBestA(REG(a0, struct TagItem *attrs))
{
  struct codeset *codeset = NULL;
  char *text;
  ULONG textLen;

  ENTER();

  ObtainSemaphoreShared(&CodesetsBase->libSem);

  text = (char *)GetTagData(CSA_Source, 0, attrs);
  textLen = GetTagData(CSA_SourceLen, text != NULL ? strlen(text) : 0, attrs);

  if(text != NULL && textLen != 0)
  {
    int numErrors = 0;
    ULONG csFamily = GetTagData(CSA_CodesetFamily, CSV_CodesetFamily_Latin, attrs);
    int *errorPtr = (int *)GetTagData(CSA_ErrPtr, 0, attrs);

    codeset = codesetsFindBest(attrs, csFamily, text, textLen, &numErrors);

    if(errorPtr != NULL)
      *errorPtr = numErrors;

    // if we still haven't got the codeset we fallback to the default
    if(codeset == NULL && GetTagData(CSA_FallbackToDefault, FALSE, attrs))
      codeset = defaultCodeset(FALSE);
  }

  ReleaseSemaphore(&CodesetsBase->libSem);

  RETURN(codeset);
  return codeset;
}

///
/// CodesetsUTF8Len()
// Returns the number of characters a utf8 string has. This is not
// identically with the size of memory is required to hold the string.
ULONG LIBFUNC CodesetsUTF8Len(REG(a0, UTF8 *str))
{
  int len = 0;
  unsigned char c;

  ENTER();

  if(str != NULL)
  {
    while((c = *str++))
    {
      len++;
      str += trailingBytesForUTF8[c];
    }
  }

  RETURN((ULONG)len);
  return (ULONG)len;
}

///
/// CodesetsStrLenA()
ULONG LIBFUNC CodesetsStrLenA(REG(a0, STRPTR str), REG(a1, struct TagItem *attrs))
{
  ULONG res = 0;

  ENTER();

  if(str != NULL)
  {
    struct codeset *codeset;
    int            len;
    STRPTR         src;
    int            utf;

    if((codeset = (struct codeset *)GetTagData(CSA_SourceCodeset, 0, attrs)) == NULL)
      codeset = defaultCodeset(TRUE);

    if(codeset == CodesetsBase->utf32Codeset)
    {
      utf = 32;
      len = utf32_strlen((UTF32 *)str);
    }
    else if(codeset == CodesetsBase->utf16Codeset)
    {
      utf = 16;
      len = utf16_strlen((UTF16 *)str);
    }
    else
    {
      utf = 0;
      len = strlen(str);
    }

    len = GetTagData(CSA_SourceLen, len, attrs);

    src = str;

    if(utf != 0)
    {
      void *srcend = src + len;
      UTF8 *dstlen = NULL;
      union TypeAliases srcAlias;
      union TypeAliases dstAlias;

      srcAlias.strptr = &src;
      dstAlias.utf8 = &dstlen;

      switch(utf)
      {
        case 16:
          CodesetsConvertUTF16toUTF8(srcAlias.cutf16, srcend, dstAlias.utf8, NULL, 0);
        break;

        case 32:
          CodesetsConvertUTF32toUTF8(srcAlias.cutf32, srcend, dstAlias.utf8, NULL, 0);
        break;
      }
      res = (IPTR)dstlen;
    }
    else
    {
      UBYTE c;

      res = 0;

      while((c = *src++) != '\0' && len != 0)
      {
        res += codeset->table[c].utf8[0];
        len--;
      }
    }
  }

  RETURN(res);
  return res;
}

///
/// CodesetsUTF8ToStrA()
// Converts an UTF8 string to a given charset. Return the number of bytes
// written to dest excluding the NULL byte (which is always ensured by this
// function; it means a NULL str will produce "" as dest; anyway you should
// check NULL str to not waste your time!).
STRPTR LIBFUNC CodesetsUTF8ToStrA(REG(a0, struct TagItem *attrs))
{
  UTF8 *src;
  ULONG srcLen;
  ULONG destLen = 0;
  ULONG *destLenPtr;
  ULONG n = 0;
  STRPTR dest = NULL;

  ENTER();

  if((src = (UTF8 *)GetTagData(CSA_Source, 0, attrs)) != NULL &&
     (srcLen = GetTagData(CSA_SourceLen, src != NULL ? strlen((char *)src) : 0, attrs)) > 0)
  {
    struct convertMsg msg;
    struct codeset *codeset;
    struct Hook *destHook;
    struct Hook *mapForeignCharsHook;
    char buf[256];
    STRPTR destIter = NULL;
    char *b = NULL;
    int i = 0;
    unsigned char *s = src;
    unsigned char *e = (src+srcLen);
    int numConvErrors = 0;
    int *numConvErrorsPtr;
    BOOL mapForeignChars;
    APTR pool = NULL;
    struct SignalSemaphore *sem = NULL;
    int utf;
    ULONG char_size;

    // get some more optional attributes
    destHook = (struct Hook *)GetTagData(CSA_DestHook, 0, attrs);
    destLen = GetTagData(CSA_DestLen, 0, attrs);
    numConvErrorsPtr = (int *)GetTagData(CSA_ErrPtr, 0, attrs);
    mapForeignChars = (BOOL)GetTagData(CSA_MapForeignChars, FALSE, attrs);
    mapForeignCharsHook = (struct Hook *)GetTagData(CSA_MapForeignCharsHook, 0, attrs);

    // get the destination codeset pointer
    if((codeset = (struct codeset *)GetTagData(CSA_DestCodeset, 0, attrs)) == NULL)
      codeset = defaultCodeset(TRUE);
    if(codeset == CodesetsBase->utf32Codeset)
    {
      utf = 32;
      char_size = 4;
    }
    else if(codeset == CodesetsBase->utf16Codeset)
    {
      utf = 16;
      char_size = 2;
    }
    else
    {
      utf = 0;
      char_size = 1;
    }

    // first we make sure we allocate enough memory
    // for our destination buffer
    if(destHook != NULL)
    {
      if(destLen < 16 || destLen > sizeof(buf))
        destLen = sizeof(buf);

      msg.state = CSV_Translating;
      b = buf;
      i = 0;
    }
    else
    {
      // in case the user wants us to dynamically generate the
      // destination buffer we do it right now
      if((dest = (STRPTR)GetTagData(CSA_Dest, 0, attrs)) == NULL ||
         GetTagData(CSA_AllocIfNeeded, TRUE, attrs) != FALSE)
      {
        ULONG len = 0;

        // calculate the destLen
        if(utf)
        {
          void *dstlen = NULL;
          union TypeAliases srcAlias;
          union TypeAliases dstAlias;

          srcAlias.uchar = &s;
          dstAlias.voidptr = &dstlen;

          switch(utf)
          {
            case 16:
              CodesetsConvertUTF8toUTF16(srcAlias.cutf8, e, dstAlias.utf16, NULL, 0);
            break;

            case 32:
              CodesetsConvertUTF8toUTF32(srcAlias.cutf8, e, dstAlias.utf32, NULL, 0);
            break;
          }
          len = (IPTR)dstlen;
        }
        else
        {
          while(s < e)
          {
            unsigned char c = *s++;

            len++;
            s += trailingBytesForUTF8[c];
          }
        }

        if(dest == NULL || (destLen < len+1))
        {
          if((pool = (APTR)GetTagData(CSA_Pool, 0, attrs)) != NULL)
          {
            if((sem = (struct SignalSemaphore *)GetTagData(CSA_PoolSem, 0, attrs)) != NULL)
              ObtainSemaphore(sem);

            // allocate the destination buffer
            dest = allocVecPooled(pool, len+char_size);

            if(sem != NULL)
              ReleaseSemaphore(sem);
          }
          else
            dest = allocArbitrateVecPooled(len+char_size);

          destLen = len+char_size;
        }

        if(dest == NULL)
        {
          RETURN(NULL);
          return NULL;
        }
      }

      destIter = dest;
    }

    // now we convert the src string to the
    // destination buffer.
    s = src;
    if(utf != 0)
    {
      void *dstend;

      if(destHook != NULL)
      {
        ULONG r = CSR_TargetExhausted;

        dstend = b + destLen - char_size;
        do
        {
          union TypeAliases srcAlias;
          union TypeAliases dstAlias;

          srcAlias.uchar = &s;
          dstAlias.schar = &b;

          switch(utf)
          {
            case 16:
              r = CodesetsConvertUTF8toUTF16(srcAlias.cutf8, e, dstAlias.utf16, dstend, 0);
            break;

            case 32:
              r = CodesetsConvertUTF8toUTF32(srcAlias.cutf8, e, dstAlias.utf32, dstend, 0);
            break;
          }
          b[0] = 0;
          if(char_size > 1)
            b[1] = 0;
          if(r != CSR_TargetExhausted)
            msg.state = CSV_End;
          msg.len = b-buf;
          CallHookPkt(destHook,&msg,buf);

          b  = buf;
          n += msg.len;
        }
        while(r == CSR_TargetExhausted);
      }
      else
      {
        union TypeAliases srcAlias;
        union TypeAliases dstAlias;

        srcAlias.uchar = &s;
        dstAlias.strptr = &destIter;
        dstend = destIter + destLen - char_size;
        switch(utf)
        {
          case 16:
            CodesetsConvertUTF8toUTF16(srcAlias.cutf8, e, dstAlias.utf16, dstend, 0);
          break;

          case 32:
            CodesetsConvertUTF8toUTF32(srcAlias.cutf8, e, dstAlias.utf32, dstend, 0);
          break;
        }
        n = destIter-dest;
      }
    }
    else
    {
      for(;;n++)
      {
        if(destHook == NULL && n >= destLen-1)
          break;

        // convert until we reach the end of the
        // source buffer.
        if(s < e)
        {
          unsigned char c = *s;
          unsigned char d = '?';
          const char *repstr = NULL;
          int replen = 0;

          // check if the char is a >7bit char
          if(c > 127)
          {
            struct single_convert *f;
            int lenAdd = trailingBytesForUTF8[c];
            int lenStr = lenAdd+1;
            unsigned char *src = s;

            do
            {
              // start each iteration with "no replacement found yet"
              repstr = NULL;
              replen = 0;

              // search in the UTF8 conversion table of the current charset if
              // we have a replacement character for the char sequence starting at s
              BIN_SEARCH(codeset->table_sorted, 0, 255, strncmp((char *)src, (char *)codeset->table_sorted[m].utf8+1, lenStr), f);

              if(f != NULL)
              {
                d = f->code;
                replen = -1;

                break;
              }
              else
              {
                // the analysed char sequence (s) is not convertable to a
                // single visible char replacement, so we normally have to put
                // a ? sign as a "unknown char" sign at the very position.
                //
                // For convienence we, however, allow users to replace these
                // UTF8 characters with char sequences that "looklike" the
                // original char.
                if(mapForeignChars == TRUE)
                  replen = mapUTF8toASCII(&repstr, src, lenStr);

                // call the hook only, if the internal table yielded no suitable
                // replacement
                if(replen == 0 && mapForeignCharsHook != NULL)
                {
                  struct replaceMsg rmsg;

                  rmsg.dst = (char **)&repstr;
                  rmsg.src = src;
                  rmsg.srclen = lenStr;
                  replen = CallHookPkt(mapForeignCharsHook, &rmsg, NULL);
                }

                if(replen < 0)
                {
                  D(DBF_UTF, "got UTF8 replacement (%ld)", replen);

                  // stay in the loop as long as one replacement function delivers
                  // further UTF8 replacement sequences
                  src = (unsigned char *)repstr;
                  // remember the length of the replaced string, as we might do another
                  // iteration in the loop which might result in a further replacement
                  lenStr = -replen;
                }
                else if(replen == 0)
                {
                  D(DBF_UTF, "found no ASCII replacement for UTF8 string (%ld)", replen);
                  repstr = NULL;
                }
                else
                  D(DBF_UTF, "got replacement string '%s' (%ld)", repstr ? repstr : "<null>", replen);
              }
            }
            while(replen < 0);

            if(repstr == NULL || replen == 0)
            {
              if(replen >= 0)
              {
                d = '?';
                numConvErrors++;
              }
            }

            s += lenAdd;
          }
          else
            d = c;

          if(destHook != NULL)
          {
            if(replen > 1)
            {
              while(replen > 0)
              {
                *b++ = *repstr;
                repstr++;
                i++;
                replen--;

                if(i%(destLen-1)==0)
                {
                  *b = '\0';
                  msg.len = i;
                  CallHookPkt(destHook, &msg, buf);

                  b  = buf;
                  *b = '\0';
                  i  = 0;
                }
              }
            }
            else
            {
              *b++ = replen > 0 ? *repstr : d;
              i++;
            }

            if(i%(destLen-1)==0)
            {
              *b = '\0';
              msg.len = i;
              CallHookPkt(destHook, &msg, buf);

              b  = buf;
              *b = '\0';
              i  = 0;
            }
          }
          else
          {
            if(replen > 1)
            {
              ULONG destPos = destIter-dest;

              if(pool != NULL)
              {
                if(sem != NULL)
                  ObtainSemaphore(sem);

                // allocate the destination buffer
                dest = reallocVecPooled(pool, dest, destLen, destLen+replen-1);

                if(sem != NULL)
                  ReleaseSemaphore(sem);
              }
              else
                dest = reallocArbitrateVecPooled(dest, destLen, destLen+replen-1);

              if(dest == NULL)
              {
                RETURN(NULL);
                return NULL;
              }

              destIter = dest+destPos;
              memcpy(destIter, repstr, replen);

              // adjust our loop pointer and destination length
              destIter += replen;
              destLen += replen-1;
            }
            else if(replen == 1)
              *destIter++ = *repstr;
            else
              *destIter++ = d;
          }

          s++;
        }
        else
          break;
      }

      if(destHook != NULL)
      {
        msg.state = CSV_End;
        msg.len   = i;
        *b        = '\0';
        CallHookPkt(destHook,&msg,buf);
      }
      else
        *destIter = '\0';
    }

    // let us write the number of conversion errors
    // to the proper variable pointer, if wanted
    if(numConvErrorsPtr != NULL)
      *numConvErrorsPtr = numConvErrors;
  }

  // put the final length of our destination buffer
  // into the destLenPtr
  if((destLenPtr = (ULONG *)GetTagData(CSA_DestLenPtr, 0, attrs)) != NULL)
  {
    if(destLen > 0)
      *destLenPtr = destLen-1;
    else
      *destLenPtr = 0;
  }

  RETURN(dest);
  return dest;
}

///
/// CodesetsUTF8CreateA()
// Converts a string and a charset to an UTF8. Returns the UTF8.
// If a destination hook is supplied always return 0.
// If from is NULL, it returns NULL and doesn't call the hook.
UTF8 * LIBFUNC CodesetsUTF8CreateA(REG(a0, struct TagItem *attrs))
{
  UTF8   *from;
  UTF8   *dest;
  struct codeset *codeset;
  ULONG  fromLen, *destLenPtr;
  ULONG  n;
  int    utf;

  ENTER();

  dest = NULL;
  n    = 0;

  if((codeset = (struct codeset *)GetTagData(CSA_SourceCodeset, 0, attrs)) == NULL)
    codeset = defaultCodeset(TRUE);
  if(codeset == CodesetsBase->utf32Codeset)
    utf = 32;
  else if(codeset == CodesetsBase->utf16Codeset)
    utf = 16;
  else
    utf = 0;

  from = (UTF8 *)GetTagData(CSA_Source, 0, attrs);
  if(from != NULL)
  {
    switch(utf)
    {
      case 32:
        fromLen = utf32_strlen((UTF32 *)from);
      break;

      case 16:
        fromLen = utf16_strlen((UTF16 *)from);
      break;

      default:
        fromLen = strlen((char *)from);
      break;
    }
  }
  else
    fromLen = 0;
  fromLen = GetTagData(CSA_SourceLen, fromLen, attrs);

  if(from != NULL && fromLen != 0)
  {
    struct convertMsg       msg;
    struct Hook    *hook;
    ULONG          destLen;
    int            i = 0;
    TEXT           buf[256];
    STRPTR         src, destPtr = NULL, b = NULL;
    ULONG          c;

    hook    = (struct Hook *)GetTagData(CSA_DestHook, 0, attrs);
    destLen = GetTagData(CSA_DestLen, 0, attrs);

    if(hook != NULL)
    {
      if(destLen<16 || destLen>sizeof(buf))
        destLen = sizeof(buf);

      msg.state = CSV_Translating;
      b = buf;
      i = 0;
    }
    else
    {
      if((dest = (UTF8 *)GetTagData(CSA_Dest, 0, attrs)) != NULL ||
         GetTagData(CSA_AllocIfNeeded, TRUE, attrs))
      {
        ULONG len;

        src = (STRPTR)from;

        if(utf != 0)
        {
          void *srcend = src + fromLen;
          UTF8 *dstlen = NULL;
          union TypeAliases srcAlias;
          union TypeAliases dstAlias;

          srcAlias.strptr = &src;
          dstAlias.utf8 = &dstlen;

          switch(utf)
          {
            case 16:
              CodesetsConvertUTF16toUTF8(srcAlias.cutf16, srcend, dstAlias.utf8, NULL, 0);
            break;

            case 32:
              CodesetsConvertUTF32toUTF8(srcAlias.cutf32, srcend, dstAlias.utf8, NULL, 0);
            break;
          }
          len = (IPTR)dstlen;
        }
        else
        {
          ULONG flen = fromLen;

          len = 0;
          while((c = *src++) != '\0' && flen != 0)
          {
            len += codeset->table[c].utf8[0];
            flen--;
          }
        }
        D(DBF_UTF, "Calculated output UTF-8 buffer length: %lu", len);

        if(dest == NULL || (destLen<len+1))
        {
          APTR                   pool;
          struct SignalSemaphore *sem;

          if((pool = (APTR)GetTagData(CSA_Pool, 0, attrs)) != NULL)
          {
            if((sem = (struct SignalSemaphore *)GetTagData(CSA_PoolSem, 0, attrs)) != NULL)
              ObtainSemaphore(sem);

            // allocate the destination buffer
            dest = allocVecPooled(pool,len+1);

            if(sem != NULL)
              ReleaseSemaphore(sem);
          }
          else
            dest = allocArbitrateVecPooled(len+1);

          destLen  = len;
        }

        if(dest == NULL)
        {
          RETURN(NULL);
          return NULL;
        }
      }

      destPtr = (STRPTR)dest;
    }

    src = (STRPTR)from;
    if(utf != 0)
    {
      void *srcend = src + fromLen;
      UTF8 *dstend;

      if(hook != NULL)
      {
        ULONG r = CSR_TargetExhausted;
        union TypeAliases srcAlias;
        union TypeAliases dstAlias;

        srcAlias.strptr = &src;
        dstAlias.strptr = &b;
        dstend = (UTF8 *)(b + destLen - 1);
        do
        {
          switch(utf)
          {
            case 16:
              r = CodesetsConvertUTF16toUTF8(srcAlias.cutf16, srcend, dstAlias.utf8, dstend, 0);
            break;

            case 32:
              r = CodesetsConvertUTF32toUTF8(srcAlias.cutf32, srcend, dstAlias.utf8, dstend, 0);
            break;
          }
          *b = 0;
          if(r != CSR_TargetExhausted)
            msg.state = CSV_End;
          msg.len = b-buf;
          CallHookPkt(hook,&msg,buf);

          b  = buf;
          n += msg.len;
        }
        while(r == CSR_TargetExhausted);
      }
      else
      {
        union TypeAliases srcAlias;
        union TypeAliases dstAlias;

        srcAlias.strptr = &src;
        dstAlias.strptr = &destPtr;
        dstend = (UTF8 *)(destPtr + destLen);
        switch(utf)
        {
          case 16:
            CodesetsConvertUTF16toUTF8(srcAlias.cutf16, srcend, dstAlias.utf8, dstend, 0);
          break;

          case 32:
            CodesetsConvertUTF32toUTF8(srcAlias.cutf32, srcend, dstAlias.utf8, dstend, 0);
          break;
        }
        n = destPtr-(STRPTR)dest;
      }
    }
    else
    {
      for(; fromLen && (c = *src); src++, fromLen--)
      {
        UTF8 *utf8_seq;

        for(utf8_seq = &codeset->table[c].utf8[1]; (c = *utf8_seq); utf8_seq++)
        {
          if(hook != NULL)
          {
            *b++ = c;
            i++;

            if(i%(destLen-1)==0)
            {
              *b = 0;
              msg.len = i;
              CallHookPkt(hook,&msg,buf);

              b  = buf;
              *b = 0;
              i  = 0;
            }
          }
          else
          {
            if(n>=destLen)
              break;

            *destPtr++ = c;
          }

          n++;
        }
      }

      if(hook != NULL)
      {
        msg.state = CSV_End;
        msg.len   = i;
        *b = 0;
        CallHookPkt(hook,&msg,buf);
      }
      else
      {
        *destPtr = 0;
      }
    }
  }

  if((destLenPtr = (ULONG *)GetTagData(CSA_DestLenPtr, 0, attrs)) != NULL)
    *destLenPtr = n;

  RETURN(dest);
  return dest;
}

///
/// CodesetsIsValidUTF8()
#define GOOD_UCS(c) \
     ((c) >= 160 && ((c) & ~0x3ff) != 0xd800 && \
      (c) != 0xfeff && (c) != 0xfffe && (c) != 0xffff)

BOOL LIBFUNC CodesetsIsValidUTF8(REG(a0, STRPTR s))
{
  STRPTR t = s;
  int n;

  ENTER();

  while((n = parseUtf8(&t)) != 0)
  {
    if(!GOOD_UCS(n))
    {
      RETURN(FALSE);
      return FALSE;
    }
  }

  RETURN(TRUE);
  return TRUE;
}

///
/// CodesetsConvertStrA()
// Converts a given string from one source Codeset to a given destination
// codeset and returns the convert string
STRPTR LIBFUNC CodesetsConvertStrA(REG(a0, struct TagItem *attrs))
{
  struct codeset *srcCodeset;
  STRPTR srcStr = NULL;
  STRPTR dstStr = NULL;
  ULONG srcLen = 0;
  ULONG dstLen = 0;
  ULONG charSize = 0;

  ENTER();

  // get the ptr to the src string we want to convert
  // from the source codeset to the dest codeset.
  srcStr = (STRPTR)GetTagData(CSA_Source, 0, attrs);

  // get the pointer to the codeset in which the src string is encoded
  if((srcCodeset = (struct codeset *)GetTagData(CSA_SourceCodeset, 0, attrs)) == NULL)
    srcCodeset = defaultCodeset(TRUE);

  if(srcStr != NULL)
  {
    if(srcCodeset == CodesetsBase->utf32Codeset)
    {
      srcLen = utf32_strlen((UTF32 *)srcStr);
      charSize = sizeof(UTF32);
    }
    else if(srcCodeset == CodesetsBase->utf16Codeset)
    {
      srcLen = utf16_strlen((UTF16 *)srcStr);
      charSize = sizeof(UTF16);
    }
    else
    {
      srcLen = strlen(srcStr);
      charSize = sizeof(char);
    }
  }
  else
    srcLen = 0;
  srcLen = GetTagData(CSA_SourceLen, srcLen, attrs);

  if(srcStr != NULL && srcLen > 0)
  {
    struct codeset *dstCodeset;

    // get the pointer to the codeset in which the dst string should be encoded
    if((dstCodeset = (struct codeset *)GetTagData(CSA_DestCodeset, 0, attrs)) == NULL)
      dstCodeset = defaultCodeset(TRUE);

    D(DBF_UTF, "srcCodeset: '%s' dstCodeset: '%s'", srcCodeset->name, dstCodeset->name);

    if(srcCodeset != NULL && dstCodeset != NULL)
    {
      // check that the user didn't supplied the very same codeset
      // or otherwise a conversion is not required.
      if(srcCodeset != dstCodeset)
      {
        BOOL utf8Create = FALSE;
        BOOL strCreate = FALSE;
        UTF8 *utf8str;
        ULONG utf8strLen = 0;
        ULONG *destLenPtr = NULL;
        BOOL mapForeignChars;
        struct Hook *mapForeignCharsHook;

        mapForeignChars = (BOOL)GetTagData(CSA_MapForeignChars, FALSE, attrs);
        mapForeignCharsHook = (struct Hook *)GetTagData(CSA_MapForeignCharsHook, 0, attrs);

        // if the source codeset is UTF-8 we don't have to use the UTF8Create()
        // function and can directly call the UTF8ToStr() function
        if(srcCodeset != CodesetsBase->utf8Codeset)
        {
          struct TagItem tags[] = { { CSA_SourceCodeset,  (IPTR)srcCodeset   },
                                    { CSA_Source,         (IPTR)srcStr       },
                                    { CSA_SourceLen,      srcLen             },
                                    { CSA_DestLenPtr,     (IPTR)&utf8strLen  },
                                    { TAG_DONE,           0                  } };

          utf8str = CodesetsUTF8CreateA((struct TagItem *)&tags[0]);

          utf8Create = TRUE;
        }
        else
        {
          utf8str = (UTF8 *)srcStr;
          utf8strLen = srcLen;
        }

        // in case the destination codeset is UTF-8 we don't have to actually
        // use the UTF8ToStr() function and can immediately return our
        // UTF8 string
        if(utf8str != NULL && utf8strLen > 0 && dstCodeset != CodesetsBase->utf8Codeset)
        {
          struct TagItem tags[] = { { CSA_DestCodeset,          (IPTR)dstCodeset           },
                                    { CSA_Source,               (IPTR)utf8str              },
                                    { CSA_SourceLen,            utf8strLen                 },
                                    { CSA_DestLenPtr,           (IPTR)&dstLen              },
                                    { CSA_MapForeignChars,      mapForeignChars            },
                                    { CSA_MapForeignCharsHook,  (IPTR)mapForeignCharsHook  },
                                    { TAG_DONE,                 0                          } };

          dstStr = CodesetsUTF8ToStrA((struct TagItem *)&tags[0]);

          strCreate = TRUE;
        }
        else
        {
          dstStr = (STRPTR)utf8str;
          dstLen = utf8strLen;
        }

        D(DBF_UTF, "srcStr: %lx srcLen: %ld dstStr: %lx dstLen: %ld utf8create: %ld strCreate: %ld", srcStr, srcLen,
                                                                                                     dstStr, dstLen,
                                                                                                     utf8Create,
                                                                                                     strCreate);

        // if everything was successfull we can go and finalize everything
        if(dstStr != NULL && utf8str != NULL)
        {
          // as the conversion was a two way pass we have to either free the
          // memory of the utf8 string or not
          if(utf8Create == TRUE && strCreate == TRUE)
            CodesetsFreeA(utf8str, NULL);

          // if the user wants to be informed abour the length
          // of our destination string we store the length now in the supplied ptr.
          if((destLenPtr = (ULONG *)GetTagData(CSA_DestLenPtr, 0, attrs)) != NULL)
            *destLenPtr = dstLen;

          D(DBF_UTF, "successfully converted string with len %ld", dstLen);
        }
        else
        {
          W(DBF_ALWAYS, "an error occurred while trying to convert a string");

          // free all memory in case the conversion didn't work out
          if(utf8Create == TRUE && utf8str != NULL)
            CodesetsFreeA(utf8str, NULL);

          if(strCreate == TRUE && dstStr != NULL)
            CodesetsFreeA(dstStr, NULL);

          dstStr = NULL;
        }
      }
      else
      {
        // we got the same source and destination codesets passed in
        // instead of failing silently we just create a copy of the source string
        ULONG *destLenPtr = NULL;

        // allocate memory for the destination string, including a trailing NUL byte
        if((dstStr = allocArbitrateVecPooled(srcLen + charSize)) != NULL)
        {
          // just copy the source string without any further modification
          // we must use memcpy() as the source string could be UTF16/32 encoded and
          // thus strcpy() would not do what we want.
          memcpy(dstStr, srcStr, srcLen + charSize);
          dstLen = srcLen;
          D(DBF_UTF, "successfully copied string with len %ld", dstLen);
        }
        else
          W(DBF_ALWAYS, "no memory for dest string");

        // if the user wants to be informed abour the length
        // of our destination string we store the length now in the supplied ptr.
        if((destLenPtr = (ULONG *)GetTagData(CSA_DestLenPtr, 0, attrs)) != NULL)
          *destLenPtr = dstLen;
      }
    }
  }

  RETURN(dstStr);
  return dstStr;
}

///
/// CodesetsFreeVecPooledA()
void LIBFUNC CodesetsFreeVecPooledA(REG(a0, APTR pool), REG(a1, APTR mem), REG(a2, struct TagItem *attrs))
{
  ENTER();

  if(pool != NULL && mem != NULL)
  {
    struct SignalSemaphore *sem;

    if((sem = (struct SignalSemaphore *)GetTagData(CSA_PoolSem, 0, attrs)) != NULL)
      ObtainSemaphore(sem);

    freeVecPooled(pool,mem);

    if(sem != NULL)
      ReleaseSemaphore(sem);
  }

  LEAVE();
}

///
/// CodesetsListCreateA()
struct codesetList * LIBFUNC CodesetsListCreateA(REG(a0, struct TagItem *attrs))
{
  struct codesetList *csList = NULL;

  ENTER();

  // no matter what, we create a codesets list we will return to the user
  if((csList = allocArbitrateVecPooled(sizeof(struct codesetList))) != NULL)
  {
    BOOL scanProgDir = TRUE;
    struct TagItem *tstate = attrs;
    struct TagItem *tag;

    // initialize the new private codeset list and put it into a separate list
    NewList((struct List *)csList);

    // first we get the path of the directory from which we go
    // and scan for charset tables from
    while((tag = NextTagItem((APTR)&tstate)) != NULL)
    {
      switch(tag->ti_Tag)
      {
        case CSA_CodesetDir:
        {
          codesetsScanDir(csList, (STRPTR)tag->ti_Data);

          scanProgDir = FALSE;
        }
        break;

        case CSA_CodesetFile:
        {
          codesetsReadTable(csList, (STRPTR)tag->ti_Data);

          scanProgDir = FALSE;
        }
        break;

        case CSA_SourceCodeset:
        {
          struct codeset *cs = (struct codeset *)tag->ti_Data;

          AddTail((struct List *)csList, (struct Node *)&cs->node);

          scanProgDir = FALSE;
        }
        break;
      }
    }

    // in case the user also wants us to scan PROGDIR:
    // we do so
    if(scanProgDir == TRUE)
      codesetsScanDir(csList, "PROGDIR:Charsets");
  }

  RETURN(csList);
  return csList;
}

///
/// CodesetsListDeleteA()
BOOL LIBFUNC CodesetsListDeleteA(REG(a0, struct TagItem *attrs))
{
  BOOL result = FALSE;
  struct TagItem *tstate = attrs;
  struct TagItem *tag;
  BOOL freeCodesets;

  ENTER();

  // check if the caller wants us also to free the codesets
  freeCodesets = (BOOL)GetTagData(CSA_FreeCodesets, TRUE, attrs);

  // now we iterate through or tagItems and see what the
  // user wants to remove from the list
  while((tag = NextTagItem((APTR)&tstate)) != NULL)
  {
    switch(tag->ti_Tag)
    {
      case CSA_CodesetList:
      {
        struct codesetList *csList = (struct codesetList *)tag->ti_Data;

        if(csList != NULL)
        {
          // cleanup the codesets within the list
          if(freeCodesets == TRUE)
            codesetsCleanup(csList);

          // then free the list itself
          freeArbitrateVecPooled(csList);

          result = TRUE;
        }
      }
      break;
    }
  }

  RETURN(result);
  return result;
}

///
/// CodesetsListAddA()
BOOL LIBFUNC CodesetsListAddA(REG(a0, struct codesetList *csList), REG(a1, struct TagItem *attrs))
{
  BOOL result = FALSE;

  ENTER();

  if(csList != NULL)
  {
    struct TagItem *tstate = attrs;
    struct TagItem *tag;

    // now we iterate through or tagItems and see if the user
    // wants to scan a whole directory or just adds a file.
    while((tag = NextTagItem((APTR)&tstate)) != NULL)
    {
      switch(tag->ti_Tag)
      {
        case CSA_CodesetDir:
        {
          codesetsScanDir(csList, (STRPTR)tag->ti_Data);
          result = TRUE;
        }
        break;

        case CSA_CodesetFile:
        {
          codesetsReadTable(csList, (STRPTR)tag->ti_Data);
          result = TRUE;
        }
        break;

        case CSA_SourceCodeset:
        {
          struct codeset *cs = (struct codeset *)tag->ti_Data;

          AddTail((struct List *)csList, (struct Node *)&cs->node);
          result = TRUE;
        }
        break;
      }
    }
  }

  RETURN(result);
  return result;
}

///
/// CodesetsListRemoveA()
BOOL LIBFUNC CodesetsListRemoveA(REG(a0, struct TagItem *attrs))
{
  BOOL result = FALSE;
  struct TagItem *tstate = attrs;
  struct TagItem *tag;
  BOOL freeCodesets;

  ENTER();

  // check if the caller wants us also to free the codesets
  freeCodesets = (BOOL)GetTagData(CSA_FreeCodesets, TRUE, attrs);

  // now we iterate through or tagItems and see what the
  // user wants to remove from the list
  while((tag = NextTagItem((APTR)&tstate)) != NULL)
  {
    switch(tag->ti_Tag)
    {
      case CSA_SourceCodeset:
      {
        struct codeset *removeCS = (struct codeset *)tag->ti_Data;

        if(removeCS != NULL)
        {
          struct Node *node;
          BOOL isExternalNode = TRUE;

          ObtainSemaphore(&CodesetsBase->libSem);

          // iterate over our internal list an check whether the given
          // node is part of that list
          for(node = GetHead((struct List *)&CodesetsBase->codesets); node != NULL; node = GetSucc(node))
          {
            if((struct codeset *)node == removeCS)
            {
              isExternalNode = FALSE;
              break;
            }
          }

          ReleaseSemaphore(&CodesetsBase->libSem);

          if(isExternalNode == TRUE)
          {
            Remove((struct Node *)removeCS);

            // free all codesets data if requested
            if(freeCodesets == TRUE)
            {
              if(removeCS->name != NULL)
                freeArbitrateVecPooled(removeCS->name);
              if(removeCS->alt_name != NULL)
                freeArbitrateVecPooled(removeCS->alt_name);
              if(removeCS->characterization != NULL)
                freeArbitrateVecPooled(removeCS->characterization);

              freeArbitrateVecPooled(removeCS);
            }

            result = TRUE;
          }
          else
            W(DBF_ALWAYS, "user tried to remove an internal codeset!");
        }
      }
      break;
    }
  }

  RETURN(result);
  return result;
}

///

/**************************************************************************/
