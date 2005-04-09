
/*****************************************************************
**                                                              **
** If you use GoldED or any other text editor featuring folding **
** you may want to set up "///" as fold opening phrase, and     **
** "//|" as closing one, as this source is using it.            **
**                                                              **
**                                                Marcin        **
**                                                              **
*****************************************************************/

/* $Id$ */


//#define __amigados

/// README
/*

    FlexCat.c:  The flexible catalog creator

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

    Ok, this is nothing special. It grabs a catalog translation and a
    catalog description file and produces catalogs and the source to
    handle them. What is it else than lots of other programs?

    The difference is, that YOU determine what source FlexCat produces.
    Another file is scanned by FlexCat to produce code. This file contains
    some c-string like special characters (%v for version for example)
    You can edit this file and modify it as you want. So FlexCat can produce
    C source as well as Assembler, Oberon, Modula 2, E, ...

*/
//|

#define VERSION 2
#define REVISION 4
#define VERS       "FlexCat 2.4"

#ifdef __amigados

 #ifdef _M68060
   #define _CPU "[68060]"
 #else
   #ifdef _M68040
     #define _CPU "[68040]"
   #else
     #ifdef _M68030
       #define _CPU "[68030]"
     #else
       #ifdef _M68020
         #define _CPU "[68020]"
       #else
         #ifdef _M68010
           #define _CPU "[68010]"
         #else
           #define _CPU "[680x0]"
         #endif
       #endif
     #endif
   #endif
 #endif
 
#define VSTRING  VERS " " _CPU " " __AMIGADATE__
#else
#define VSTRING  VERS
#endif

#define VERSTAG  "$VER: " VSTRING

/// Includes and defines

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#ifdef __amigados
        #include <dos.h>
#endif
#include "flexcat_cat.h"

#if ((defined(_DCC) && defined(AMIGA))       ||     \
     (defined(__SASC) && defined(_AMIGA)))      &&  \
    !defined(__amigados)
#define __amigados
#endif

#if defined(__amigados)
#include <exec/types.h>
#if defined(_DCC) || defined(__SASC) || defined(__GNUC__)
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#else
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include <clib/utility_protos.h>
#endif

#ifdef tolower
#undef tolower
#endif
#define tolower         ToLower
#define stricmp(s,t)    Stricmp((char *) (s), (char *) (t))
#define strnicmp(s,t,l) Strnicmp((char *) (s), (char *) (t), l)

#endif


#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE (!FALSE)
#endif


#define MAXPATHLEN 512
#define FLEXCAT_SDDIR "FLEXCAT_SDDIR"
#if defined(__amigados)
#define DEFAULT_FLEXCAT_SDDIR "PROGDIR:lib"
#else
#define DEFAULT_FLEXCAT_SDDIR "lib"
#endif

#if defined(__amigados)
#define MAX_PREFS_LEN 512
#define FLEXCAT_PREFS "flexcat.prefs"
char    prefs_sddir[MAXPATHLEN] = "\0";
#endif


#ifndef MAKE_ID
#define MAKE_ID(a,b,c,d)        \
        ((ULONG) (a)<<24 | (ULONG) (b)<<16 | (ULONG) (c)<<8 | (ULONG) (d))
#endif

#ifndef ULONG
#define ULONG unsigned long
#endif

//|
/// Structs

enum StringTypes {
    TYPE_C,         /*  Produce C strings                   */
    TYPE_ASSEMBLER, /*  Produce Assembler strings           */
    TYPE_OBERON,    /*  Produce Oberon strings              */
    TYPE_E,         /*  Produce E strings. (Oops, thought   */
                    /*  it allows only 32 bit integers? ;-) */
    TYPE_NONE       /*  Simple strings                      */
};


enum OutputModes {
    OutputMode_None,    /*  Nothing written yet                 */
    OutputMode_Bin,     /*  Last character written was binary   */
    OutputMode_Ascii    /*  Last character written was Ascii    */
};

struct CatString
{ struct CatString *Next;
  char *CD_Str;
  char *CT_Str;
  char *ID_Str;
  int MinLen, MaxLen, ID, Nr;
  int NotInCT;          /* If string is not present we write NEW    */
                        /* while updating CT file, for easier work. */

};

struct CDLine
{ struct CDLine *Next;
  char *Line;
};

struct CatalogChunk
{ struct CatalogChunk *Next;            /* struct CatalogChunk *Next */
  ULONG ID;
  char *ChunkStr;
};

struct CatString *FirstCatString = NULL;  /*  First catalog string            */
struct CDLine *FirstCDLine = NULL;        /*  First catalog description line  */
struct CatalogChunk *FirstChunk = NULL;   /*  List of catalog chunks          */

char *BaseName   = "";              /*  Basename of catalog description */
char *Language   = "english";       /*  Language of catalog description */
int  CatVersion  = 0;               /*  Version of catalog to be opened */
int  LengthBytes = 0;               /*  Number of bytes to preceed a    */
                                    /*  created string and containing   */
                                    /*  its length.                     */
char *CatLanguage      = NULL;      /*  Language of catalog translation */
char *CatVersionString = NULL;      /*  version string of catalog       */
                                    /*  translation (## version)        */
char *CatRcsId = NULL;              /*  rcs ID of catalog translation   */
                                    /*  (## rcsid)                      */
char *CatName = NULL;               /*  name of catalog translation     */
int  CodeSet = 0;                   /*  Codeset of catalog translation  */
int  NumStrings = 0;                /*  Number of catalog strings       */
int  LongStrings = TRUE;            /*  Generate long or short strings  */

char *ScanFile;                     /*  File currently scanned          */
int  ScanLine;                      /*  Line currently scanned          */

int  GlobalReturnCode = 0;          /*  Will be 5, if warnings appear    */
int  WarnCTGaps = FALSE;            /*  Warn missing symbols in CT       */
                                    /*  file.                            */
int  NoOptim = FALSE;               /*  Put string into catalog even     */
                                    /*  if translation is equal to       */
                                    /*  description.                     */
int  Fill    = FALSE;               /*  It translation of given string   */
                                    /*  is missing or it's empty, write  */
                                    /*  string descriptor from #?.cd     */
                                    /*  file instead.                    */
int  DoExpunge = FALSE;             /*  If TRUE FlexCat will do AVAIL    */
                                    /*  FLUSH alike after catalog save   */
int  NoBeep = FALSE;                /*  if TRUE, FlexCat won't call      */
                                    /*  DisplayBeep() any longer         */
int  Quiet   = FALSE;               /*  Forces FlexCat to shut up        */

int  NumberOfWarnings = 0;          /* We count warnings to be smart     */
                                    /* and not to do Beep bombing, but   */
                                    /* call DisplayBeep() only once      */
int  CT_Scanned = FALSE;            /* If TRUE, and we are going to      */
                                    /* write new #?.ct file, then user   */
                                    /* surely updates own #?.ct file     */
                                    /* so we should write ***NEW***      */
                                    /* whenever necessary.               */
int  LANGToLower = TRUE;            /* Shall we do ToLower() on lang's   */
                                    /* name? Some #?.language seems to   */
                                    /* be broken, so we allow workaround */
int  NoBufferedIO = FALSE;          /* Shall we do buffered IO           */
int  buffer_size = 2048;            /* Size of the IO buffer             */
int  Modified = FALSE;              /* Shall we write the catalog ONLY   */
                                    /* if #?.catalog is younger than     */
                                    /* #?.c(d|t) files?                  */

#define MAX_NEW_STR_LEN 25
char Msg_New[MAX_NEW_STR_LEN]    = "***NEW***";
                                    /*  new strings in updated #?.ct    */

int  CopyNEWs = FALSE;
char Old_Msg_New[MAX_NEW_STR_LEN] = "; ***NEW***";

                                    /*  old newstring (above) used in old   */
                                    /* CT file. Now we look if it's present */
                                    /* and copy it into new CT if user does */
                                    /* upgrade (flexcat CD CT newctfile CT  */

int  NoSpace = FALSE;               /* do want to strip the space usually  */
                                    /* placed between ';' and original     */
                                    /* string?                             */


char VersTag[] = VERSTAG;
char VString[] = VSTRING " by Jochen Wiedmann and Marcin Orlowski";
char EString[] = "E-mail: carlos@amiga.com.pl  WWW: http://amiga.com.pl/flexcat/";
//|

/// FUNC: ReadPrefs
#if defined(__amigados)

char ReadPrefs(void)
{
enum{ SDDIR,
      MSG_NEW,
      WARNCTGAPS,
      NOOPTIM,
      FILL,
      FLUSH,
      NOBEEP,
      QUIET,
      NOLANGTOLOWER,
      NOBUFFEREDIO,
      MODIFIED,
      COPYMSGNEW,
      OLDMSGNEW,
      NOSPACE,

      ARGS_COUNT
    };

char   template[] = "SDDIR/K,MSG_NEW/K,WARNCTGAPS/S,NOOPTIM/S,FILL/S,FLUSH/S,NOBEEP/S,QUIET/S,NOLANGTOLOWER/S,NOBUFFEREDIO/S,MODIFIED/S,COPYMSGNEW/S,OLDMSGNEW/K,NOSPACE/S";
LONG   Results[ARGS_COUNT] = {0};
char   result = FALSE;
char   *prefs;
struct RDArgs *rda;
struct RDArgs *rdargs;

  if(prefs = getenv(FLEXCAT_PREFS))
    {
    prefs = realloc(prefs, strlen(prefs)+1);
    strcat(prefs, "\n");

    if(rda = AllocDosObject(DOS_RDARGS, TAG_DONE))
       {
       rda->RDA_Source.CS_Buffer = prefs;
       rda->RDA_Source.CS_Length = strlen(prefs);
       rda->RDA_Source.CS_CurChr = 0;
       rda->RDA_Flags |= RDAF_NOPROMPT;

       if(rdargs = ReadArgs(template, Results, rda))
         {
         if(Results[SDDIR])
           strncpy(prefs_sddir, (char *)Results[SDDIR], MAXPATHLEN);

         if(Results[MSG_NEW])
           strncpy(Msg_New, (char *)Results[MSG_NEW], MAX_NEW_STR_LEN);

         WarnCTGaps   = Results[WARNCTGAPS];
         NoOptim      = Results[NOOPTIM];
         Fill         = Results[FILL];
         DoExpunge    = Results[FLUSH];
         NoBeep       = Results[NOBEEP];
         Quiet        = Results[QUIET];
         LANGToLower  = Results[NOLANGTOLOWER];
         Modified     = Results[MODIFIED];
         NoBufferedIO = Results[NOBUFFEREDIO];
         CopyNEWs     = Results[COPYMSGNEW];
         NoSpace      = Results[NOSPACE];
         if(Results[OLDMSGNEW])
           sprintf(Old_Msg_New, "; %s", (char *)Results[OLDMSGNEW]);

         FreeArgs(rdargs);

         result = TRUE;
         }
       else
         {
         fputs((char *)msgPrefsError, stderr);
         fputs((char *)template, stderr);
         fputs((char *)"\n", stderr);
         DisplayBeep(NULL);
         }

       FreeDosObject(DOS_RDARGS, rda);
       }
     else
       {
       fputs("Error processing prefs.\nCan't AllocDosObject()\n", stderr);
       }

    free(prefs);
    }

  return(result);
}

#endif
//|

/// FUNC: MyExit
void MyExit (int Code)
{

#if defined(__amigados)

  if(((NumberOfWarnings > 0) ||(Code !=0)) && (!NoBeep))
     DisplayBeep(NULL);

#endif


#if defined(_DCC)
  //STATIC __autoexit VOID _STDCloseFlexCatCatalog(VOID)
#elif defined(__SASC)
  //VOID _STDCloseFlexCatCatalog(VOID)
#elif defined(__GNUC__)
  //STATIC VOID _STDCloseFlexCatCatalog(VOID)
#elif defined(__INTEL_COMPILER)
  //STATIC VOID _STDCloseFlexCatCatalog(VOID)
#else
  CloseFlexCatCatalog();      /* we need to close something... */
#endif


    exit(Code);
}
//|

/// FUNC: stricmp

// quick stricmp

#if !defined(__amigados) && !defined(__CYGWIN32__)
#    define stricmp strcasecmp
#endif

#if 0
int stricmp( const char *str1, const char *str2 )
{
int i;

    for(i = 0;; i++)
       {
       int a = tolower( (int)*str1 );
       int b = tolower( (int)*str2 );

       if( !a || !b )
           break;

       if( a != b )
           return( 1 );

       str1++;
       str2++;
       }

    return( 0 );
}
#endif

//|
/// FUNC: strnicmp

// quick strnicmp

#if !defined(__amigados) && !defined(__CYGWIN32__)
int strnicmp( const char *str1, const char *str2, size_t len )
{
size_t i;

    for(i = 0; i < len; i++)
       {
       int a = tolower( (int)*str1 );
       int b = tolower( (int)*str2 );

       if( !a || !b )
           break;

       if( a != b )
           return( 1 );

       str1++;
       str2++;
       }

    return( 0 );
}
#endif

//|

/// FUNC: Swappers...


unsigned short (*SwapWord)(unsigned short r) = NULL;
unsigned long  (*SwapLong)(unsigned long r)  = NULL;


unsigned short SwapWord21(unsigned short r)
{
    return (unsigned short)((r>>8) + (r<<8));
}
unsigned short SwapWord12(unsigned short r)
{
    return r;
}
unsigned long SwapLong4321(unsigned long r)
{
    return  ((r>>24) & 0xFF) + (r<<24) + ((r>>8) & 0xFF00) + ((r<<8) & 0xFF0000);
}
unsigned long SwapLong1234(unsigned long r)
{
    return r;
}

//|
/// FUNC: SwapChoose
int SwapChoose(void)
{
unsigned short w;
unsigned int d;

  strncpy((char *)&w, "\1\2", 2);
  strncpy((char *)&d, "\1\2\3\4", 4);

  if (w == 0x0201)
    SwapWord = SwapWord21;
  else if (w == 0x0102)
    SwapWord = SwapWord12;
  else
    return 0;

  if (d == 0x04030201)
    SwapLong = SwapLong4321;
  else if (d == 0x01020304)
    SwapLong = SwapLong1234;
  else
    return 0;

  return 1;
}
//|

/// FUNC: ShowError

/*
    This shows an error message and terminates
*/
void ShowError(const char *msg, ...)
{
char **ptr = (char **) &msg;

//  if(!Quiet)
    {
    fprintf(stderr, ptr[0], ptr[1], ptr[2], ptr[3], ptr[4]);
    putc('\n', stderr);
    }

#if defined(__amigados)
  NumberOfWarnings++;
#endif

  MyExit(10);
}
//|
/// FUNC: MemError

/*
    This shows the message: Memory error.
*/
void MemError(void)

{
  ShowError(msgMemoryError, NULL);
}
//|
/// FUNC: ShowWarn

/*
    This shows a warning
*/
void ShowWarn(const char *msg, ...)

{ char **ptr = (char **) &msg;

  if(!Quiet)
    {
    fprintf(stderr, (char *) msgWarning, ScanFile, ScanLine);
    fprintf(stderr, ptr[0], ptr[1], ptr[2], ptr[3], ptr[4]);
    putc('\n', stderr);
    }

  NumberOfWarnings++;
  GlobalReturnCode = 5;
}
//|
/// FUNC: AllocString

/*
    This allocates a string
*/
char *AllocString(const char *str)

{ char *ptr;

  if (!(ptr = malloc(strlen(str)+1)))
  { MemError();
  }
  strcpy(ptr, str);
  return(ptr);
}
//|
/// FUNC: Add catalog chunk

/*
    This adds a new catalog chunk to the list of catalog
    chunks.
*/
char *AddCatalogChunk(char *ID, const char *string)
{
  struct CatalogChunk *cc, **ccptr;

  if (!(cc = malloc(sizeof(*cc))))
  { MemError();
  }
  cc->Next = NULL;
  cc->ID = *((ULONG *) ID);
  cc->ChunkStr = AllocString(string);

  /*
      Put the new chunk to the end of the chunk list.
  */
  for (ccptr = &FirstChunk;  *ccptr != NULL;  ccptr = &(*ccptr)->Next)
  {
  }
  *ccptr = cc;
  return(cc->ChunkStr);
}
//|
/// FUNC: gethex
/*
    This translates a hex character.
*/
int gethex(int c)
{
  if (c >= '0'  &&  c <= '9')
  { return(c - '0');
  }
  else if (c >= 'a'  &&  c <= 'f')
  { return(c - 'a' + 10);
  }
  else if (c >= 'A'  &&  c <= 'F')
  { return(c - 'A' + 10);
  }
  ShowWarn(msgExpectedHex);
  return(0);
}
//|
/// FUNC: getoctal

/*
    This translates an octal digit.
*/
int getoctal(int c)
{

  if (c >= '0'  &&  c <= '7')
    {
    return(c - '0');
    }

  ShowWarn(msgExpectedOctal);
  return(0);

}
//|
/// FUNC: ReadLine

/*
    Reading a line is somewhat complicated in order to allow lines of any
    length.

    Inputs: fp           - the file, where the input comes from
            AllowComment - TRUE, if a leading semicolon should force to
                           interpret the line as a comment line
*/
#define BUFSIZE 4096
char *ReadLine(FILE *fp, int AllowComment)
{

  char *OldLine, *NewLine = NULL;
  int c = '\0';
  int Len = 0, LineLen = 0;
  int FirstChar = TRUE;
  int BackslashSeen   = FALSE;
  int BackslashSeenOn = 0;     /* position the last backslash was seen on */
  int CommentLine = FALSE;     /* if TRUE we should ignore normally treat trailing \'s */

  while(c != EOF)
    {
    if(Len+10 > LineLen)
      {
      OldLine = NewLine;
      if(!(NewLine = malloc(LineLen+BUFSIZE)))
        MemError();

      strncpy(NewLine, OldLine, LineLen);
      if(OldLine)
        free(OldLine);

      LineLen += BUFSIZE;
      }

    c = getc(fp);

    if(FirstChar)
      {
      if(c == EOF)
        {
        free(NewLine);
        return(NULL);
        }

      if(c == ';')
        {
        CommentLine = TRUE;
        }

      FirstChar = FALSE;
      }

    switch(c)
      {
      case '\r':
        break;

      case '\n':
        ++ScanLine;
        if(BackslashSeen)
          {
          NewLine[Len++] = c;
          BackslashSeen = FALSE;
          break;
          }
        c = EOF;

      case EOF:
        break;

                                 /*  Let's check for trailing \\ */
      case '\\':
        {
        if(!CommentLine)
          {
          if(BackslashSeen)
            {
            if(BackslashSeenOn == (Len-1))
              {
              BackslashSeen = FALSE;
              NewLine[Len++] = c;
              break;
              }
            }

          BackslashSeen = TRUE;
          BackslashSeenOn = Len;
          }

        NewLine[Len++] = c;
        break;
        }


      default:
        BackslashSeen = FALSE;
        NewLine[Len++] = c;
      }
    }

  NewLine[Len] = '\0';

  return(NewLine);

}
//|
/// FUNC: OverSpace

/*
    This removes trailing blanks.
*/
void OverSpace(char **strptr)

{ int c;

  while ((c = **strptr) == ' '  ||  c == '\t')
  { (*strptr)++;
  }
}
//|

/// FUNC: Expunge

void Expunge(void)
{
#if defined(__amigados)


  if(DoExpunge)
    {
#ifdef __EXPUNGE_ALL__
    APTR Memory;

    if(Memory = AllocMem(-1, NULL))
       FreeMem(Memory, -1);                // just in case ;-)
#else

#pragma libcall LocaleBase localeExpunge 12 00
VOID localeExpunge(VOID);

    struct Library    *LocaleBase;

    if(LocaleBase = OpenLibrary("locale.library", 0))
       {
       localeExpunge();
       CloseLibrary(LocaleBase);
       }

#endif
    }
#endif

}

//|

/// FUNC: ReadChar

/*
    ReadChar scans an input line translating the backslash characters.

    Inputs: char *  - a pointer to a stringpointer; the latter points to the
                      next character to be read and points behind the read
                      bytes after executing ReadChar
            dest    - a pointer to a buffer, where the read bytes should be
                      stored

    Result: number of bytes that are written to dest (between 0 and 2)
*/
int ReadChar(char **strptr, char *dest)
{
  char c;
  int i;

  switch(c = *((*strptr)++))
    {
    case '\\':

      switch(c = tolower((int) *((*strptr)++)))
        {
        case '\n':
          return(0);
        case 'b':
          *dest = '\b';
          break;
        case 'c':
          *dest = '\233';
          break;
        case 'e':
          *dest = '\033';
          break;
        case 'f':
          *dest = '\f';
          break;
        case 'g':
          *dest = '\007';
          break;
        case 'n':
          *dest = '\n';
          break;
        case 'r':
          *dest = '\r';
          break;
        case 't':
          *dest = '\t';
          break;
        case 'v':
          *dest = '\013';
          break;
        case 'x':
          *dest = gethex((int) **strptr);
          (*strptr)++;
          if (((c = **strptr) >= '0'  &&  c <= '9')  ||
              (c >= 'a'  &&  c <= 'f')  ||  (c >= 'A'  &&  c <= 'F'))
          { *dest = (*dest << 4) + gethex((int) c);
            (*strptr)++;
          }
          break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':

          *dest = getoctal((int)c);

          for(i = 0;  i < 2;  i++)
            {
            if((c = **strptr) >= '0'  &&  c <= '7')
              {
              *dest = (*dest << 3) + getoctal((int) c);
              (*strptr)++;
              }
            }
          break;
        case ')':
        case '\\':
          *(dest++) = '\\';
          *dest = c;
          return(2);
        default:
          *dest = c;
      }
      break;

    default:
      *dest = c;
  }
  return(1);
}
//|
/// FUNC: ScanCDFile

/*
    This scans the catalog description file.

    Inputs: cdfile  - name of the catalog description file

    Result: TRUE, if successful, FALSE otherwise
*/
int ScanCDFile(char *cdfile)
{
  FILE *fp;
  struct CDLine *cdline, **cdptr = &FirstCDLine;
  struct CatString *cs, **csptr = &FirstCatString;
  char *line, *newline;
  char *ptr;
  int NextID = 0, len;
  int Result = TRUE;

  ScanFile = cdfile;
  ScanLine = 0;

  if(!(fp = fopen(cdfile, "r")))
    {
    ShowError(msgNoCatalogDescription, cdfile);
    }

  if(!NoBufferedIO)
    setvbuf(fp, NULL, _IOFBF, buffer_size);

  /*
      Get the basename
  */
  if ((ptr = strchr(cdfile, ':')))
  { cdfile = ptr+1;
  }
  if ((ptr = strrchr(cdfile, '/')))
  { cdfile = ptr+1;
  }
  if ((ptr = strrchr(cdfile, '.')))
  { len = ptr-cdfile;
  }
  else
  { len = strlen(cdfile);
  }
  if (!(BaseName = malloc(len+1)))
  { MemError();
  }
  strncpy(BaseName, cdfile, len);
  BaseName[len] = '\0';

  while(!feof(fp)  &&  (line = newline = ReadLine(fp, TRUE)))
  {
    if(!(cdline = malloc(sizeof(*cdline))))
      {
      MemError();
      }

    *cdptr = cdline;
    cdptr = &cdline->Next;
    cdline->Next = NULL;
    cdline->Line = line = AllocString(newline);
    free(newline);

    if (*line == ';')
      {
      continue;
      }

    if(*line == '#')
      {
      int CheckExtra = TRUE;

      if (strnicmp(line+1, "language", 8) == 0)
        {
        char *ptr;

        line += 9;
        OverSpace(&line);
        Language = AllocString(line);

        if(LANGToLower)
           {
           for (ptr = Language;  *ptr;  ptr++)
             {
             *ptr = tolower((int) *ptr);
             }
           CheckExtra = FALSE;
           }

        }
      else
        if(strnicmp(line+1, "version", 7) == 0)
          {
          CatVersion = strtol(line+8, &line, 0);
          }
        else
          {
          if(strnicmp(line+1, "basename", 8) == 0)
           {
           line += 9;
           OverSpace(&line);
           free(BaseName);
           BaseName = AllocString(line);
           CheckExtra = FALSE;
           }
          else
           {
           ShowWarn(msgUnknownCDCommand);
           Result = FALSE;
           CheckExtra = FALSE;
           }
         }

      if(CheckExtra)
        {
        OverSpace(&line);
          if(*line)
            {
            ShowWarn(msgExtraCharacters);
            Result = FALSE;
            }
        }
      }
    else
      {
      char *idstr;

      if(*line == ' '  ||  *line == '\t')
        {
        ShowWarn(msgUnexpectedBlanks);
        Result = FALSE;
        OverSpace(&line);
        }

      idstr = line;
      while ((*line >= 'a'  &&  *line <= 'z')  ||
             (*line >= 'A'  &&  *line <= 'Z')  ||
             (*line >= '0'  &&  *line <= '9')  ||
             *line == '_')
        {
        ++line;
        }

      if(idstr == line)
        {
        ShowWarn(msgNoIdentifier);
        Result = FALSE;
        }
      else
        {
        int found;

        if(!(cs = malloc(sizeof(*cs))))
          {
          MemError();
          }

        do
          {
          struct CatString *scs;

          found = TRUE;
          for(scs = FirstCatString;  scs != NULL;  scs = scs->Next)
            {
            if(scs->ID == NextID)
              {
              found = FALSE;
              ++NextID;
              break;
              }
            }
          }
        while(!found);

        cs->Next = NULL;
        cs->ID = NextID;
        cs->MinLen = 0;
        cs->MaxLen = -1;
        cs->CD_Str = "";
        cs->CT_Str = NULL;
        cs->NotInCT= TRUE;

        if(!(cs->ID_Str = malloc((line-idstr)+1)))
          {
          MemError();
          }
        strncpy(cs->ID_Str, idstr, line-idstr);
        cs->ID_Str[line-idstr] = '\0';

        OverSpace(&line);

        if(*line != '(')
          {
          ShowWarn(msgNoLeadingBracket);
          Result = FALSE;
          }
        else
          {
          char *oldstr;
          struct CatString *scs;
          char bytes[10];
          int bytesread, reallen;

          ++line;
          OverSpace(&line);
          if(*line != '/')
            {
            if(*line == '+')
              {
              NextID = cs->ID = NextID + strtol(line, &line, 0);
              }
            else
              {
              cs->ID = NextID = strtol(line, &line, 0);
              }

            OverSpace(&line);
            }

          for(scs = FirstCatString;  scs != NULL;  scs = scs->Next)
          { if (scs->ID == cs->ID)
            { ShowWarn(msgDoubleID);
              Result = FALSE;
            }
            if (strcmp(cs->ID_Str, scs->ID_Str)  ==  0)
            { ShowWarn(msgDoubleIdentifier);
              Result = FALSE;
            }
          }

          if (*line != '/')
          { ShowWarn(msgNoMinLen);
            Result = FALSE;
          }
          else
          { ++line;
            OverSpace(&line);
            if (*line != '/')
            { cs->MinLen = strtol(line, &line, 0);
              OverSpace(&line);
            }
            if (*line != '/')
            { ShowWarn(msgNoMaxLen);
              Result = FALSE;
            }
            else
            { ++line;
              OverSpace(&line);
              if (*line != ')')
              { cs->MaxLen = strtol(line, &line, 0);
                OverSpace(&line);
              }
              if (*line != ')')
              { ShowWarn(msgNoTrailingBracket);
                Result = FALSE;
              }
              else
              { ++line;
                OverSpace(&line);
                if (*line)
                { ShowWarn(msgExtraCharacters);
                }
              }
            }
          }
        if (!(newline = ReadLine(fp, FALSE)))
        { ShowWarn(msgNoString);
          Result = FALSE;
          cs->CD_Str = "";
        }
        else
        { cs->CD_Str = AllocString(newline);
          free(newline);
        }

        /*
            Get stringlen
        */
        oldstr = cs->CD_Str;
        reallen = 0;
        while (*oldstr)
        { bytesread = ReadChar(&oldstr, bytes);
          if (bytesread == 2)
          { bytesread--;
          }
          reallen += bytesread;
        }

        if (cs->MinLen > 0  &&  reallen < cs->MinLen)
        { ShowWarn(msgShortString);
        }
        if (cs->MaxLen > 0  &&  reallen > cs->MaxLen)
        { ShowWarn(msgLongString);
        }

        cs->Nr = NumStrings;

        *csptr = cs;
        csptr = &cs->Next;
        ++NumStrings;
        }
      }
    }
  }
  fclose(fp);
  return(Result);
}
//|
/// FUNC: ScanCTFile

/*
    This scans a catalog translation file.

    Inputs: ctfile      - name of the translation file to scan.

    Result: TRUE, if successful, FALSE otherwise.
*/
int ScanCTFile(char *ctfile)
{
  FILE *fp;
  char *newline, *line, *idstr, *newidstr, *newstr;
  struct CatString *cs=NULL;
  int Result = TRUE;

  ScanFile = ctfile;
  ScanLine = 0;

  if (!(fp = fopen(ctfile, "r")))
    {
    ShowError(msgNoCatalogTranslation, ctfile);
    }

  if(!NoBufferedIO)
    setvbuf(fp, NULL, _IOFBF, buffer_size);


  while (!feof(fp)  &&  (line = newline = ReadLine(fp, TRUE)))
    {
    switch(*line)
      {
      case ';':
        if( CopyNEWs == TRUE )
           {
           if(strnicmp( line, Old_Msg_New, strlen(Old_Msg_New) ) == 0)
             {
             cs->NotInCT = TRUE;
             }
           }
      break;

      case '#':
///       looking for command;
        if(*(++line) != '#')
          {
          ShowWarn(msgNoCTCommand);
          }
        ++line;
        OverSpace(&line);
        if (strnicmp(line, "version", 7) == 0)
        { if (CatVersionString || CatRcsId || CatName)
          { ShowWarn(msgDoubleCTVersion);
          }
          line += 7;
          OverSpace(&line);
          CatVersionString = AllocString(line);
        }
        else if (strnicmp(line, "codeset", 7) == 0)
        { line += 7;
          CodeSet = strtol(line, &line, 0);
          OverSpace(&line);
          if (*line)
          { ShowWarn(msgExtraCharacters);
          }
        }
        else if (strnicmp(line, "language", 8) == 0)
        { char *ptr;

          if (CatLanguage)
          { ShowWarn(msgDoubleCTLanguage);
          }
          line += 8;
          OverSpace(&line);
          CatLanguage = AddCatalogChunk("LANG", line);

          if(LANGToLower)
            for (ptr = CatLanguage;  *ptr;  ptr++)
              *ptr = tolower((int) *ptr);
        }
        else if (strnicmp(line, "chunk", 5) == 0)
        { char *ID;

          line += 5;
          OverSpace(&line);
          ID = line;
          line += sizeof(ULONG);
          OverSpace(&line);

          AddCatalogChunk(ID, AllocString(line));
        }
        else if (strnicmp(line, "rcsid", 5) == 0)
        { if (CatVersionString || CatRcsId)
          { ShowWarn(msgDoubleCTVersion);
          }
          line += 5;
          OverSpace(&line);
          CatRcsId = AllocString(line);
        }
        else if (strnicmp(line, "name", 5) == 0)
        { if (CatVersionString || CatName)
          { ShowWarn(msgDoubleCTVersion);
          }
          line += 4;
          OverSpace(&line);
          CatName = AllocString(line);
        }
        else if (strnicmp(line+1, "lengthbytes", 11) == 0)
        { line += 12;
          if ((LengthBytes = strtol(line, &line, 0))
                           > sizeof(long))
          { ShowWarn(msgNoLengthBytes, sizeof(long));
            LengthBytes = sizeof(long);
          }
        }
        else
        {
        ShowWarn(msgUnknownCTCommand);
        }
//|
        break;

      default:
        if(*line == ' '  ||  *line == '\t')
          {
          ShowWarn( msgUnexpectedBlanks );
          OverSpace( &line );
          }
        idstr = line;

        while ((*line >= 'a'  &&  *line <= 'z')  ||
               (*line >= 'A'  &&  *line <= 'Z')  ||
               (*line >= '0'  &&  *line <= '9')  ||
               *line == '_')
        { ++line;
        }
        if (idstr == line)
          {
          ShowWarn(msgNoIdentifier);
          break;
          }

        if(!(newidstr = malloc(line-idstr+1)))
          {
          MemError();
          }

        strncpy(newidstr, idstr, line-idstr);
        newidstr[line-idstr] = '\0';
        OverSpace(&line);

        if(*line)
          {
          ShowWarn(msgExtraCharacters);
          }

        if((newstr = ReadLine(fp, FALSE)))
          {
          for(cs = FirstCatString;  cs != NULL;  cs = cs->Next)
            {
            if(strcmp(cs->ID_Str, newidstr) == 0)
              {
              break;
              }
            }
          if(cs == NULL)
            {
            ShowWarn(msgUnknownIdentifier, newidstr);
            }
          else
            {
            char *oldstr;
            char bytes[10];
            int bytesread, reallen;

            if(cs->CT_Str)
              {
              ShowWarn(msgDoubleIdentifier);
              Result = FALSE;
              free(cs->CT_Str);
              }
            cs->CT_Str = AllocString(newstr);
            cs->NotInCT = FALSE;

            /*
                Get stringlen
            */
            oldstr = cs->CT_Str;
            reallen = 0;
            while(*oldstr)
              {
              bytesread = ReadChar(&oldstr, bytes);
              if(bytesread == 2)
                {
                bytesread--;
                }
              reallen += bytesread;
              }

            if(cs->MinLen > 0  &&  reallen < cs->MinLen)
              {
              ShowWarn(msgShortString);
              }
            if(cs->MaxLen > 0  &&  reallen > cs->MaxLen)
              {
              ShowWarn(msgLongString);
              }


            // checking for trailing ellipsis...

            if( reallen >= 3 )
               {
               long cd_len = strlen( cs->CD_Str );

               if( cd_len >= 3 )
                   {
                   if( strcmp( &cs->CD_Str[ cd_len - 2 ], "..." ) == 0 )
                       if( strcmp( &cs->CT_Str[ reallen - 2 ], "..." ) != 0 )
                         {
//                         printf("ORG: '%s'\nNEW: '%s'\n", cs->CD_Str, cs->CT_Str);
                         ShowWarn(msgTrailingEllipsis);
                         }
                   }
               }    


            // checking for trailing spaces

            if( reallen >= 1 )
               {
               long cd_len = strlen( cs->CD_Str );

               if( cd_len >= 1 )
                   {
                   if( strcmp( &cs->CD_Str[ cd_len - 1 ], " " ) == 0 )
                       if( strcmp( &cs->CT_Str[ reallen - 1 ], " " ) != 0 )
                         ShowWarn(msgTrailingSpaces);
                   }
               }


            }
          free(newstr);
          }
        else
          {
          ShowWarn(msgNoString);
          if(cs)
              cs->CT_Str = "";
          }
        free(newidstr);
    }
    free(newline);
  }

  fclose(fp);

  if (WarnCTGaps)
  { for (cs = FirstCatString;  cs != NULL;  cs = cs->Next)
    { if (cs->CT_Str == NULL)
      { ShowWarn(msgCTGap, cs->ID_Str);
      }
    }
  }

  if(Result)
    CT_Scanned = TRUE;

  return(Result);
}
//|
/// FUNC: CatPuts


/*
    CatPuts prints a string to a catalog. (The string is preceded by a
    long integer containing its length and probably padded up to word
    boundary or longword boundary, depending on the argument padbytes.)
    The arguments countnul should be TRUE if the NUL byte at the end of
    the string should be counted.
*/
int CatPuts(FILE *fp, char *str, int padbytes, int countnul)
{
  unsigned long reallen, virtuallen, chunklen, swapped_long;
  int bytesread;
  char *oldstr;
  char bytes[10];

  /*      Get Length of string.
  */

  oldstr = str;
  reallen = 0;

  while(*oldstr)
    {
    bytesread = ReadChar(&oldstr, bytes);
    if(bytesread == 2)
      {
      bytesread--;
      }
    reallen += bytesread;
    }

  virtuallen = chunklen = reallen + LengthBytes;
  if(countnul || chunklen % padbytes == 0)
    {
    virtuallen++;
    }

  swapped_long = SwapLong( virtuallen );

  fwrite(&swapped_long, sizeof(virtuallen), 1, fp);
  if(LengthBytes)
    {
    fwrite(((char *) &reallen)+sizeof(reallen)-LengthBytes, LengthBytes, 1, fp);
    }

  while(*str)
    {
    bytesread = ReadChar(&str, bytes);
    if(bytesread)
      {
      fwrite(bytes+bytesread-1, 1, 1, fp);
      }
    }

  do
    {
    putc('\0', fp);
    }
  while(++chunklen % padbytes);

  return((int) chunklen+4);
}
//|
/// FUNC: PutCatalogChunk

/*
    This puts a string chunk into the catalog
*/
int PutCatalogChunk(FILE *fp, struct CatalogChunk *cc)
{
  fwrite(&cc->ID, sizeof(cc->ID), 1, fp);
  return(4 + CatPuts(fp, cc->ChunkStr, 2, TRUE));
}
//|
/// FUNC: CalcRealLength
/*
    This function measures the real (binary) length of source
    string. It correctly process 'slash chars' (\n, \000 etc),
    and gives the real length such source string have.

    Inputs: source - pointer to null terminated source string

    Result: real length
*/

int CalcRealLength(char *source)
{
int  count = 0;
char *src = source;
char bytes[10];

  while(*src)
    {
    count += ReadChar(&src, bytes);
    }

//  printf("%ld: '%s'\n", count, source);

  return(count);

}
//|

/// FUNC: CreateCatalog

/*
    This creates a catalog.
*/
void CreateCat(char *CatFile)
{
  FILE *fp;
  int CatLen, HeadLen;
  struct CatString *cs;
  struct CatalogChunk *cc;
  int i;

  if(!CatVersionString && !CatRcsId)
    {
    ShowError(msgNoCTVersion);
    }

  if(!CatLanguage)
    {
    ShowError(msgNoCTLanguage);
    }

  if(strlen(CatLanguage) == 0)
    {
    ShowError(msgNoCTLanguage);
    }

  if(!(fp = fopen(CatFile, "w")))
    {
    ShowError(msgNoCatalog, CatFile);
    }

  if(!NoBufferedIO)
    setvbuf(fp, NULL, _IOFBF, buffer_size);


  fputs("FORM0000CTLG", fp);
  CatLen = 4;

  if(CatVersionString)
    {
    struct CatalogChunk cc;
    char *verStr = NULL;

    cc.ID = MAKE_ID('F','V','E','R');

    if( strstr(CatVersionString, "$TODAY") )
       {

       if(verStr = malloc(strlen(CatVersionString)+128))
           {
           char *found = strstr(CatVersionString, "$TODAY");
           char dateStr[10];

           long tim;
           struct tm *t;

           time(&tim);
           t = gmtime(&tim);

           *found = 0;
           strftime(dateStr, sizeof(dateStr), "%d.%m.%y", t);

           sprintf(verStr, "%s%s%s", CatVersionString, dateStr, found+strlen("$TODAY"));
           cc.ChunkStr = verStr;
           }
       else
          MemError();

       }
    else
       {
       cc.ChunkStr = CatVersionString;
       }

    cc.ID = SwapLong( cc.ID );
    CatLen += PutCatalogChunk(fp, &cc);

    if( verStr )
       free(verStr);
    }
  else
    {
    struct CatalogChunk cc;
    char* verStr;
    int year = 0, month = 0, day = 0;
    int version = 0, revision = 0;
    char* name = NULL;
    char* ptr;

    if(!CatRcsId)
      {
      ShowError(msgNoCTVersion);
      }
    else
      {
      if(!(ptr = strstr(CatRcsId, "$Date:"))
          || sscanf(ptr+6, " %d/%d/%d", &year, &month, &day) != 3
          || !(ptr = strstr(CatRcsId, "$Revision:"))
          || sscanf(ptr+10, " %d.%d", &version, &revision) != 2)
      {
      ShowError(msgWrongRcsId);
      }
      if ((ptr = strstr(CatRcsId, "$Id:")))
      { int len = 0;
        char* found;

        ptr += 4;
        OverSpace(&ptr);
        found = ptr;

        while(*ptr  &&  *ptr != '$'  &&  *ptr != ' '  &&  *ptr != '\t')
          {
          ++len;
          ++ptr;
          }
        if(!(name = malloc(len+1)))
          {
          MemError();
         }
        strncpy(name, found, len);
        name[len] = '\0';
      }
    }
    if (CatName)
    { name = CatName;
    }
    else if (!name)
      {
      ShowError(msgNoCTVersion);
      name = "";
      }
    if (!(verStr = malloc(strlen(name) + 256)))
      {
      MemError();
      }

    sprintf(verStr, "$V");
    sprintf(verStr, "ER: %s %ld.%ld (%ld.%ld.%ld)", name, version, revision, day, month, year);

    cc.ID = MAKE_ID('F','V','E','R');
    cc.ID = SwapLong( cc.ID );
    cc.ChunkStr = verStr;
    CatLen += PutCatalogChunk(fp, &cc);
  }


  for (cc = FirstChunk;  cc != NULL;  cc = cc->Next)
    {
    CatLen += PutCatalogChunk(fp, cc);
    }

  i = 32;
  fputs("CSET", fp);

  {
  int i_tmp = SwapLong( i );

  fwrite(&i_tmp, sizeof(i_tmp), 1, fp);
  }

  while(i-- > 0)
    {
    putc('\0', fp);
    }

  CatLen += 48;
  fprintf(fp, "STRS0000");
  HeadLen = CatLen;


  for(cs = FirstCatString;  cs != NULL;  cs = cs->Next)
    {
    int FillUsed = FALSE;
    int tmp_ID = SwapLong( cs->ID );

    if(Fill)
      {

      if(cs->CT_Str)
        {
        if(strlen(cs->CT_Str) == 0)
          {
          fwrite(&tmp_ID, sizeof(tmp_ID), 1, fp);
          CatLen += 4 + CatPuts(fp, cs->CD_Str, 4, FALSE);
          FillUsed = TRUE;
          }
        }
      else
        {
        fwrite(&tmp_ID, sizeof(cs->ID), 1, fp);
        CatLen += 4 + CatPuts(fp, cs->CD_Str, 4, FALSE);
        FillUsed = TRUE;
        }
      }

    if((!FillUsed) && cs->CT_Str  &&  (NoOptim ? TRUE : strcmp(cs->CT_Str, cs->CD_Str)))
      {
      fwrite(&tmp_ID, sizeof( tmp_ID ), 1, fp);
      CatLen += 4 + CatPuts(fp, cs->CT_Str, 4, FALSE);
      }
    }


  {
  int tmp_Len;

  fseek(fp, 4, SEEK_SET);

  tmp_Len = SwapLong( CatLen );
  fwrite(&tmp_Len, sizeof(tmp_Len), 1, fp);
  fseek(fp, HeadLen-4, SEEK_CUR);

  CatLen -= HeadLen;
  tmp_Len = SwapLong( CatLen );
  fwrite(&tmp_Len, sizeof(CatLen), 1, fp);
  }

  fclose(fp);

  Expunge();

}
//|
/// FUNC: CreateCTFile

/*
    This creates a new catalog translation file.
*/
void CreateCTFile(char *NewCTFile)
{
  FILE   *fp;
  struct CDLine *cd;
  struct CatString *cs;
  struct CatalogChunk *cc;
  char   *line;

  if(!CatVersionString && !CatRcsId)
    {
    ScanLine = 1;
    ShowWarn(msgNoCTVersion);
    }

  if(!(fp = fopen(NewCTFile, "w")))
    {
    ShowError(msgNoNewCTFile);
    }


  if(!NoBufferedIO)
    setvbuf(fp, NULL, _IOFBF, buffer_size);


  if(CatRcsId)
    {
    fprintf(fp, "## rcsid %s\n",
          CatRcsId ? CatRcsId : "");
    if(CatName)
       fprintf(fp, "## name %s\n", CatName);
    }
  else
    {
    if(CatVersionString)
      fprintf(fp, "## version %s\n", CatVersionString);
    else
      {
      fprintf(fp, "## version $V");
      fprintf(fp, "%c", 50+19);  // E
      fprintf(fp, "R: XX.catalog XX.XX ($TODAY)\n");
      }
    }


  {
  char *lang = NULL;

  if(CatLanguage == NULL)
    if(lang = getenv("ENV:language"))
      {
      int i;

      for(i=0;i<strlen(lang); i++)
        if(lang[i] == '\n')
          {
          lang[i] = 0;
          break;
          }

      CatLanguage = lang;
      }

  fprintf(fp, "## language %s\n## codeset %d\n;\n",
       CatLanguage ? CatLanguage : "X", CodeSet);

  if(lang)
    {
    free(lang);
    CatLanguage = NULL;
    }
  }



  for (cc = FirstChunk;  cc != NULL;  cc = cc->Next)
    {
    if (cc->ChunkStr != CatLanguage)
      {
      fprintf(fp, "## chunk ");
      fwrite((char *) &cc->ID, sizeof(cc->ID), 1, fp);
      fprintf(fp, " %s\n", cc->ChunkStr);
      }
    }

  for(cd = FirstCDLine, cs = FirstCatString;
      cd != NULL;
      cd = cd->Next)
     {
     switch(*cd->Line)
       {
       case '#':
          break;

       case ';':
          fprintf(fp, "%s\n", cd->Line);
          break;

       default:
          if(cs)
            {
/*
            fprintf(fp, "%s\n", cs->ID_Str);
            fprintf(fp, "%s\n", cs->CT_Str ? cs->CT_Str : "");
            putc(';', fp);
            putc(' ', fp);
*/
            fprintf(fp, "%s\n%s\n;", cs->ID_Str, cs->CT_Str ? cs->CT_Str : "");

            if( !NoSpace )
                putc(' ', fp);



            for (line = cs->CD_Str;  *line;  ++line)
              {
              putc((int) *line, fp);
              if(*line == '\n')
                {
                putc(';', fp);
                if( !NoSpace )
                   putc(' ', fp);
                }
              }
            putc('\n', fp);

            if(cs->NotInCT && CT_Scanned)
              fprintf(fp, ";\n; %s\n", Msg_New);

            cs = cs->Next;
            }
       }
     }

  fclose(fp);
}
//|

/// FUNC: InitCatStringOutput

/*
    InitCatStringOutput gets called before writing a catalog string as
    source.

    Inputs: fp   = file pointer to the output file
            type = one of   TYPE_C          create C strings
                            TYPE_ASSEMBLER  create Assembler strings
                            TYPE_OBERON     create Oberon strings
                            TYPE_E          create E strings
                            TYPE_NONE       create simple strings
*/
int  OutputMode = OutputMode_None;
int  OutputType = TYPE_C;
FILE *OutputFile;
int  OutputLen;

void InitCatStringOutput(FILE *fp)
{
  OutputLen = 0;
  OutputFile = fp;
  OutputMode = OutputMode_None;
  switch(OutputType)
  { case TYPE_C:
    case TYPE_OBERON:
      putc('\"', fp);
      OutputMode = OutputMode_Ascii;
      break;
    case TYPE_E:
      putc('\'', fp);
    case TYPE_ASSEMBLER:
    case TYPE_NONE:
      break;
  }
}
//|
/// FUNC: SeparateCatStringOutput

/*
    SeparateCatStringOutput gets called to split a catalog into separate
    lines.
*/
void SeparateCatStringOutput(void)
{
    switch(OutputType)
    { case TYPE_C:
        if (!LongStrings)
        { fputs("\"\\\n\t\"", OutputFile);
        }
        break;
      case TYPE_E:
        if (!LongStrings)
        { fputs("\' +\n\t\'", OutputFile);
        }
        break;
      case TYPE_OBERON:
        if (!LongStrings)
        { fputs("\"\n\t\"", OutputFile);
        }
        break;
      case TYPE_ASSEMBLER:
        if (!LongStrings)
        { if (OutputMode == OutputMode_Ascii)
          { putc('\'', OutputFile);
          }
          putc('\n', OutputFile);
          OutputMode = OutputMode_None;
        }
        break;
      case TYPE_NONE:
        break;
    }
}
//|
/// FUNC: WriteBinChar

/*
    WriteBinChar writes one binary character into the source file
*/
void WriteBinChar(int c)
{

  switch(OutputType)
  { case TYPE_C:
    case TYPE_E:
    case TYPE_OBERON:
      switch(c)
      { case '\b':
          fputs("\\b", OutputFile);
          break;
        case '\n':
          fputs("\\n", OutputFile);
          break;
        case '\r':
          fputs("\\r", OutputFile);
          break;
        case '\t':
          fputs("\\t", OutputFile);
          break;
        case '\f':
          fputs("\\f", OutputFile);
          break;
        case '\0':
          fputs("\\000", OutputFile);
          break;
        default:
          if(OutputType == TYPE_E  &&  c == '\033')
            {
            fputs("\\e", OutputFile);
            }
          else
           {
           fprintf(OutputFile, "\\%c%c%c", ((c >> 6) & 3) + '0',
                    ((c >> 3) & 7) + '0', (c & 7) + '0');
           }
          break;
      }
      ++OutputLen;
      OutputMode = OutputMode_Bin;
      break;
    case TYPE_ASSEMBLER:
      switch(OutputMode)
      { case OutputMode_None:
          fprintf(OutputFile, "\tdc.b\t$%02x", c & 0xff);
          break;
        case OutputMode_Ascii:
          putc('\'', OutputFile);
        case OutputMode_Bin:
          fprintf(OutputFile, ",$%02x", c & 0xff);
          break;
      }
      ++OutputLen;
      OutputMode = OutputMode_Bin;
      break;
    case TYPE_NONE:
      ShowWarn(msgNoBinChars);
      break;
  }
}
//|
/// FUNC: WriteAsciiChar

/*
    WriteAsciiChar writes one ascii character into the source file.
*/
void WriteAsciiChar(int c)
{

  switch(OutputType)
  { case TYPE_C:
    case TYPE_OBERON:
      switch(c)
      { case '\"':
          fputs("\\\"", OutputFile);
          break;
        default:
          putc(c, OutputFile);
          break;
      }
      ++OutputLen;
      OutputMode = OutputMode_Ascii;
      break;
    case TYPE_E:
      switch(c)
      { case '\'':
          fputs("''", OutputFile);
          break;
        default:
          putc(c, OutputFile);
          break;
      }
      ++OutputLen;
      OutputMode = OutputMode_Ascii;
      break;
    case TYPE_ASSEMBLER:
      if (c == '\'')
      { WriteBinChar(c);
      }
      else
      { switch (OutputMode)
        { case OutputMode_None:
            fprintf(OutputFile, "\tdc.b\t\'%c", c);
            break;
          case OutputMode_Ascii:
            putc(c, OutputFile);
            break;
          case OutputMode_Bin:
            fprintf(OutputFile, ",\'%c", c);
            break;
        }
        ++OutputLen;
        OutputMode = OutputMode_Ascii;
      }
      break;
    case TYPE_NONE:
      putc(c, OutputFile);
      break;
  }
}
//|
/// FUNC: TerminateCatStringOutput

/*
    TerminateCatStringOutput finishs the output of a catalog string.
*/
void TerminateCatStringOutput(void)
{
  switch(OutputType)
  { case TYPE_C:
    case TYPE_OBERON:
      putc('\"', OutputFile);
      break;
    case TYPE_E:
      putc('\'', OutputFile);
      break;
    case TYPE_ASSEMBLER:
      switch(OutputMode)
      { case OutputMode_Ascii:
          putc('\'', OutputFile);
        case OutputMode_Bin:
          break;
        case OutputMode_None:
          break;
      }
    case TYPE_NONE:
      break;
  }
}
//|

/// FUNC: WriteString
/*
    This writes a sourcestring.
*/
void WriteString(FILE *fpout, char *str, long Len)
{
  char bytes[10];
  int bytesread;
  int needseparate = FALSE;

  InitCatStringOutput(fpout);
  if (Len >= 0)
  { int i;

    for(i = LengthBytes;  i >= 1;  i--)
    { WriteBinChar((int) ((char *) &Len)[sizeof(Len)-i]);
    }
  }

  while (*str)
  { bytesread = ReadChar(&str, bytes);
    if (bytesread)
    { unsigned char c;

      if (needseparate)
      { SeparateCatStringOutput();
        needseparate = FALSE;
      }

      c = bytes[bytesread-1];
      if ((c >= 0x20  &&  c < 0x7f)  ||  c >= 0xa0)
      { WriteAsciiChar((int) c);
      }
      else
      { WriteBinChar((int) c);
      }
    }
    else
    { needseparate = TRUE;
    }
  }
  TerminateCatStringOutput();
}
//|
/// FUNC: AllocFileName
/*
    This function creates a copy of a filename, removes an
    optional ending and pathname components, if desired.

    Inputs: filename - the filename to copy
            howto - a set of bits
                        bit 0: 1 = remove ending, 0 = leave it
                        bit 1: 1 = remove pathname, 0 = leave it

    Result: The copy of the filename
*/
char *AllocFileName(char *filename, int howto)
{
  char *tempstr, *ptr;

  if (!(tempstr = strdup(filename)))
  { MemError();
    MyExit(10);
  }

  /*  Remove pathname components, if desired    */
  if (howto & 2)
  { if ((ptr = strchr(tempstr, ':')))
    { tempstr = ptr+1;
    }
    if ((ptr = strrchr(tempstr, '/')))
    { tempstr = ptr+1;
    }
  }

  /*  Remove ending, if desired.                */
  if (howto & 1)
  { if ((ptr = strrchr(tempstr, '.')))
    { *ptr = '\0';
    }
  }

  return(tempstr);
}
//|
/// FUNC: AddFileName
/*
    This function adds a pathname and a filename to a full
    filename.

    Inputs: pathname - the leading pathname
            filename - the filename

    Result: The new filename
*/
char *AddFileName(char *pathname, char *filename)
{
  char *buffer;
  int size = strlen(pathname) + strlen(filename) + 2;

  if (!(buffer = malloc(size)))
  { MemError();
    MyExit(10);
  }

#if defined(__amigados)
  strcpy(buffer, pathname);
  AddPart((char *) buffer, (char *) filename, size);
#else
  sprintf(buffer, "%s/%s", pathname, filename);
#endif

  return(buffer);
}
//|

/// FUNC: CreateSourceFile

/*
    Finally the source creation.
*/
void CreateSourceFile(char *SourceFile, char *TemplateFile, char *CDFile)
{

  FILE *fpin, *fpout;
  char *line;
  char *OrigTemplateFile = TemplateFile;

  ScanFile = SourceFile;
  ScanLine = 0;

  /*
    Open the source file. This may be found in various places
  */
  if(!(fpin = fopen(TemplateFile, "r")))
    {
#ifdef __amigados
    if(*prefs_sddir != 0)
      {
      TemplateFile = AddFileName(prefs_sddir, OrigTemplateFile);
      fpin = fopen(TemplateFile, "r");
      }
#endif
    }

  if(!fpin)
    {
    char *sddir;

    if((sddir = getenv(FLEXCAT_SDDIR)))
      {
      TemplateFile = AddFileName(sddir, OrigTemplateFile);
      fpin = fopen(TemplateFile, "r");
      }
    }

  if (!fpin)
  { TemplateFile = AddFileName(DEFAULT_FLEXCAT_SDDIR, OrigTemplateFile);
    fpin = fopen(TemplateFile, "r");
  }

  if (!fpin)
    {
      ShowError(msgNoSourceDescription, OrigTemplateFile);
      return;
    }

  if (!(fpout = fopen(SourceFile, "w")))
  {
    ShowError(msgNoSource, SourceFile);
    return;
  }

  if(!NoBufferedIO)
    setvbuf(fpin, NULL, _IOFBF, buffer_size);
  if(!NoBufferedIO)
    setvbuf(fpout, NULL, _IOFBF, buffer_size);


  while(!feof(fpin)  &&  (line = ReadLine(fpin, FALSE)))
  { struct CatString *cs;
    int NeedRepeat;
    char bytes[10];
    int bytesread;

    cs = FirstCatString;
    do
    { char *currentline = line;
      NeedRepeat = FALSE;

      if (*currentline == '#'  &&  *(++currentline) == '#')
      { ++currentline;
        OverSpace(&currentline);

        if(strnicmp( currentline, "rem", 3 ) == 0)
           {
           // we just skip this line
           continue;
           }

        if (strnicmp(currentline, "stringtype", 10) == 0)
        { currentline += 10;
          OverSpace(&currentline);
          if (strnicmp(currentline, "c", 1) == 0)
          { OutputType = TYPE_C;
            ++currentline;
          }
          else if (strnicmp(currentline, "assembler", 9) == 0)
          { OutputType = TYPE_ASSEMBLER;
            currentline += 9;
          }
          else if (strnicmp(currentline, "oberon", 6) == 0)
          { OutputType = TYPE_OBERON;
            currentline += 6;
          }
          else if (strnicmp(currentline, "e", 1)  ==  0)
          { OutputType = TYPE_E;
            ++currentline;
          }
          else if (strnicmp(currentline, "none", 4)  ==  0)
          { OutputType = TYPE_NONE;
            currentline += 4;
          }
          else
          { ShowWarn(msgUnknownStringType);
            currentline += strlen(currentline);
          }
          OverSpace(&currentline);
          if (*currentline)
          { ShowWarn(msgExtraCharacters);
          }
          continue;
        }
        else if (strnicmp(currentline, "shortstrings", 12) == 0)
        { currentline += 12;
          LongStrings = FALSE;
          OverSpace(&currentline);
          if (*currentline)
          { ShowWarn(msgExtraCharacters);
          }
          continue;
        }
      }

      currentline = line;
      while(*currentline)
      { bytesread = ReadChar(&currentline, bytes);
        if (bytesread)
        { if (*bytes == '%')
          { char c;

            switch(c = *(currentline++))
            { case 'b':
                fputs(BaseName, fpout);
                break;
              case 'n':
                fprintf(fpout, "%d", NumStrings);
                break;
              case 'v':
                fprintf(fpout, "%d", CatVersion);
                break;
              case 'l':
                WriteString(fpout, Language, -1);
                break;
              case 'f':
                { char *tempstr;

                  if ((c = *currentline++) == 'v')
                  { fputs(VERS, fpout);
                  }
                  else
                  { tempstr = AllocFileName(CDFile, c - '0');
                    fputs(tempstr, fpout);
                  }
                }
                break;
              case 'o':
                { char *tempstr;

                  tempstr = AllocFileName(SourceFile, *currentline++ - '0');
                  fputs(tempstr, fpout);
                }
                break;
              case 'i':
                NeedRepeat = TRUE;
                if (cs) fputs(cs->ID_Str, fpout);
                break;

              case 'a':
              case 't':

              case 'd':
              case 'x':
              case 'c':
              case '0':
              case '1':
              case '2':
              case '3':
              case '4':
              case '5':
              case '6':
              case '7':
              case '8':
              case '9':
                  {
                  int len = 0;

                  while(c >= '0'  &&  c <= '9')
                    {
                    len = (c - '0') + len * 10;
                    c = *currentline++;
                    }


                  if(c ==  'a')
                    {
                    int _len = len ? len : 4;
                    char *start;
                    char _StrLen[20 + 1];

                    sprintf(_StrLen, "%020lx", cs->ID);

                    start = &_StrLen[20-_len*2];
                    while(_len>0)
                       {
                       fprintf(fpout, "\\x%.2s", start);
                       start+=2;
                       _len--;
                       }
                    }

                  if(c ==  't')
                    {
                    int _len = len ? len : 4;
                    char *start;
                    char _StrLen[20 + 1];

                    sprintf(_StrLen, "%020lx", ((CalcRealLength(cs->CD_Str) + 1) & 0xfffffe));

                    start = &_StrLen[20-_len*2];
                    while(_len>0)
                       {
                       fprintf(fpout, "\\x%.2s", start);
                       start+=2;
                       _len--;
                       }
                    }

                  if(c == 'c' || c == 'd' || c == 'x')
                    {
                    char buffer[20];

                    if(c == 'c') c = 'o';

                    if(len)
                      sprintf(buffer, "%%0%d%c", len, c);
                    else
                      sprintf(buffer, "%%%c", c);

                    if(cs) fprintf(fpout, buffer, cs->ID);
                    }


                  NeedRepeat = TRUE;
                  }
                  break;

              case 'e':
                NeedRepeat = TRUE;
                if (cs) fprintf(fpout, "%d", cs->Nr);
                break;
              case 's':
                NeedRepeat = TRUE;
                if (cs)
                { char *idstr;
                  unsigned long len = 0;

                  if (LengthBytes)
                  { idstr = cs->CD_Str;
                    while(*idstr)
                    { bytesread = ReadChar(&idstr, bytes);
                      if (bytesread)
                      { ++len;
                      }
                    }
                  }
                  WriteString(fpout, cs->CD_Str, LengthBytes ? len : -1);
                }
                break;
              case '(':
                NeedRepeat = TRUE;
                while(*currentline  &&  *currentline != ')')
                { bytesread = ReadChar(&currentline, bytes);
                  if (bytesread  &&  cs  &&  cs->Next)
                  { putc((int) bytes[bytesread-1], fpout);
                  }
                }
                if (!*currentline)
                { ShowWarn(msgNoTerminateBracket);
                }
                else
                { ++currentline;
                }
                break;

// !!!! FIX !!!!

              case 'z':
                {
                int diff = (((CalcRealLength(cs->CD_Str) + 1) & 0xfffffe) - (CalcRealLength(cs->CD_Str)));

                NeedRepeat = TRUE;

                while(diff > 0)
                   {
                   fprintf(fpout, "\\x00");
                   diff--;
                   }

                break;
                }

              default:
                { int c = *currentline++;

                  putc(c, fpout);
                }
            }
          }
          else
          { putc((int) bytes[bytesread-1], fpout);
          }
        }
      }
      putc('\n', fpout);
    }
    while(NeedRepeat  &&  cs  &&  (cs = cs->Next));

    free(line);
  }

  fclose(fpin);
  fclose(fpout);
}
//|

/// FUNC: Usage
/*
    The Usage function describes the programs calling syntax.
*/
void Usage(void)

{
  fputs((char *) msgUsageHead, stderr);
  fprintf(stderr, ": FlexCat CDFILE/A,CTFILE,CATALOG/K,NEWCTFILE/K,SOURCES/M,\n                WARNCTGAPS/S,NOOPTIM/S,FILL/S,FLUSH/S,NOBEEP/S,\n                QUIET/S,NOLANGTOLOWER/S,NOBUFFEREDIO/S,\n                MODIFIED/S,COPYMSGNEW/S,OLDMSGNEW/K, NOSPACE/S\n\n", VString);
  fprintf(stderr, "%s\n%s\n%s\n%s\n", msgUsage, msgUsage_2, msgUsage_3, msgUsage_4 );
  fprintf(stderr, "\n\n%s"
/*
  #ifdef _M68040
    " [040]"
  #else
    #ifdef _M68060
      " [060]"
    #else
      #ifdef _M68030
        " [030]"
      #else
        #ifdef _M68020
          " [020]"
        #else
          #ifdef _M68010
            " [010]"
          #endif
        #endif
      #endif
    #endif
  #endif
*/
  "\n", VString);


  fprintf(stderr, "%s\n", EString);
  MyExit(5);
}
//|
/// FUNC: main
/*
    Finally the main function. Does nothing special except for scanning
    the arguments.
*/
int main(int argc, char *argv [])
{
  char *cdfile, *ctfile, *newctfile, *catalog;
  char *source, *template;
  int i;

  if(argc == 0)    /*  Aztec's entry point for workbench programs  */
   {
   fprintf(stderr, "FlexCat can't be run from Workbench!\n\n");
   fprintf(stderr, "Open a Shell session and type FlexCat ?\n");
   fprintf(stderr, "for more information\n");
   exit(5);
   }

  cdfile = ctfile = newctfile = catalog = NULL;


  /* let's open catalog files by hand if necessary */
  /* should be done automatically anyway for most  */
  /* cases, but, that depends on compiler...       */

#if defined(_DCC)
  // STATIC __autoinit VOID _STIOpenFlexCatCatalog(VOID)
#elif defined(__SASC)
  // VOID _STIOpenFlexCatCatalog(VOID)
#elif defined(__GNUC__)
  // VOID _STIOpenFlexCatCatalog(VOID)
#elif defined(__INTEL_COMPILER)
  // VOID _STIOpenFlexCatCatalog(VOID)
#else
   OpenFlexCatCatalog();   /* no autoopen. we do it then */
#endif


    // Big Endian vs Little Endian (both supported ;-)
    if( !SwapChoose() )
       {
       fprintf(stderr, "FlexCat is unable to determine the\n");
       fprintf(stderr, "the byte order used by your system.\n");
       fprintf(stderr, "It's neither Little nor Big Endian?!.\n");
       exit(5);
       }



#if defined(__amigados)
  ReadPrefs();
#endif

  if(argc == 1)
    {
    Usage();
    }

  for (i = 1;  i < argc;  i++)
    {
    if(strnicmp (argv[i], "catalog=", 8) == 0)
      {
      catalog = argv[i] + 8;
      }
    else
    if(stricmp (argv[i], "catalog") == 0)
      {
      if(i+1 == argc)
        Usage();
      catalog = argv[++i];
      }
    else
    if(stricmp(argv[i], "nooptim") == 0)
     {
     NoOptim = TRUE;
     }
    else
    if(stricmp(argv[i], "fill") == 0)
     {
     Fill = TRUE;
     }
    else
    if(stricmp(argv[i], "quiet") == 0)
     {
     Quiet = TRUE;
     }
    else
    if(stricmp(argv[i], "flush") == 0)
     {
     DoExpunge = TRUE;
     }
    else
    if(stricmp(argv[i], "nobeep") == 0)
     {
     NoBeep = TRUE;
     }
    else
    if(stricmp(argv[i], "nobufferedio") == 0)
     {
     NoBufferedIO = TRUE;
     }
    else
    if (strnicmp (argv[i], "newctfile=", 10) == 0)
      {
      newctfile = argv[i] + 10;
      }
    else
    if(strnicmp (argv[i], "newctfile", 10) == 0)
      {
      if (i+1 == argc)
        Usage();
      newctfile = argv[++i];
      }
    else
    if(stricmp(argv[i], "nolangtolower") == 0)
      {
      LANGToLower = FALSE;
      }
    else
    if(stricmp(argv[i], "modified") == 0)
      {
      Modified = TRUE;
      }
    else
    if(stricmp(argv[i], "warnctgaps") == 0)
      {
      WarnCTGaps = TRUE;
      }
    else
    if(stricmp(argv[i], "copymsgnew") == 0)
      {
      CopyNEWs = TRUE;
      }
    else
    if(stricmp(argv[i], "nospace") == 0)
      {
      NoSpace = TRUE;
      }
    else
    if(stricmp(argv[i], "oldmsgnew") == 0)
      {
      sprintf( Old_Msg_New, "; %s", argv[++i] );
      }
    else
      if(cdfile == NULL)
        {
        if(stricmp(argv[i], "?") == 0 || stricmp(argv[i], "-h") == 0 || stricmp(argv[i], "help") == 0 || stricmp(argv[i], "--help") == 0)
          {
          Usage();
          }
        if(!ScanCDFile(cdfile = argv[i]))
          {
          MyExit(10);
          }
        }
    else
    if(strchr(argv[i], '='))
      {
      source = AllocString(argv[i]);
      *(template = strchr(source, '=')) = '\0';
      ++template;

      CreateSourceFile(source, template, cdfile);
      }
    else
      {
      if (ctfile)
        {
        Usage();
        }
      ctfile = argv[i];
      }
    }

    
#if defined(__amigados)
  if(Modified)
    {
    if(cdfile && ctfile && catalog)
       {
       long cd_time, ct_time, cat_time;

       if((cd_time = getft(cdfile)) != -1)
           {
           if((ct_time = getft(ctfile)) != -1)
              {
              if((cat_time = getft(catalog)) == -1)
                cat_time = 0;

                if((cat_time > ct_time) &&
                   (cat_time > cd_time))
                       {
                       if(!Quiet)
                         {
                         fprintf(stderr, (char *) msgUpToDate, catalog);
                         putc('\n', stderr);
                         }

                       MyExit(GlobalReturnCode);
                       }
                  else
                    {
                    if(!Quiet)
                      {
                      fprintf(stderr, "--> %s", catalog);
                      putc('\n', stderr);
                      }
                    }
              }
            else
              {
              ShowError(msgCantCheckDate, ctfile);
              }
           }
        else
           {
           ShowError(msgCantCheckDate, cdfile);
           }
       }
    }
#endif

  if(ctfile)
    {
    if(!ScanCTFile(ctfile))
      MyExit(10);
    }

  if(catalog)
    {
    if(!ctfile)
      {
      fprintf(stderr, (char *) msgNoCTArgument);
      Usage();
      }
    CreateCat(catalog);
    }

  if(newctfile)
    {
    CreateCTFile(newctfile);
    }

  MyExit(GlobalReturnCode);
}
//|
/// FUNC: wbmain

/*
    Dice's entry point for workbench programs
*/
#if defined(__amigados)  &&  defined(_DCC)
void wbmain(struct WBStartup *wbmsg)
{
   fprintf(stderr, "FlexCat can't be run from Workbench!\n\n");
   fprintf(stderr, "Open a Shell session and type FlexCat\n");
   fprintf(stderr, "for syntax and more information\n");

   exit(5);
}
#endif
//|

