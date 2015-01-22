/******************************************************************************
 *
 * fd2inline
 *
 * Should be able to parse CBM fd files and generate vanilla inline calls
 * for gcc. Works as a filter.
 *
 * by Wolfgang Baron, all rights reserved.
 *
 * improved, updated, simply made workable by Rainer F. Trunz 1/95
 *
 * Completely reworked Version, cleaned up and removed a whole lot of bugs
 * found by Kamil Iskra.
 *
 * Expect miracles now (hopefully...). Ok, I am just kidding :)
 *
 * Version 0.99a by Rainer F. Trunz 6/95
 *
 * Version 0.99b and later by Kamil Iskra.
 *
 * Version 1.3x	by Martin Blom
 *              See fd2inline.guide/fd2inline.info for details.
 *
 * version 1.38 by AROS development team
 *
 *****************************************************************************/

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/******************************************************************************
 * The program has a few sort of class definitions, which are the result of
 * object oriented thinking, to be imlemented in plain C. I just haven't
 * had the time to learn C++ or install the compiler. The design does however
 * improve robustness, which allows the source to be used over and over again.
 * if you use this code, please leave a little origin note.
 ******************************************************************************/

#if defined(__AROS__)
const static char version_str[]="$VER: fd2inline " VERSION " (24.2.2002)";
#endif

/******************************************************************************
 * These are general definitions including types for defining registers etc.
 ******************************************************************************/

#ifdef DEBUG
#define DBP(a) a
#else /* !DEBUG */
#define DBP(a)
#endif /* !DEBUG */

#if (defined(__GNUC__) || defined(__SASC)) && 0
#define INLINE __inline /* Gives 20% *larger* executable with GCC?! */
#else
#define INLINE
#endif

#define REGS 16	 /* d0=0,...,a7=15 */
#define FDS 1000

/* To prevent the enum below from getting broken when building on AROS */
#undef AROS

typedef enum
{
   d0, d1, d2, d3, d4, d5, d6, d7, a0, a1, a2, a3, a4, a5, a6, a7, illegal
} regs;

typedef unsigned char shortcard;

typedef enum { false, nodef, real_error } Error;

enum { NEW, OLD, STUBS, PROTO, GATESTUBS, GATEPROTO } output_mode=NEW;
enum { IX86BE_AMITHLON, AROS, M68K_AMIGAOS, M68K_POS, PPC_POWERUP, PPC_MORPHOS } target = M68K_AMIGAOS;

int Quiet = 0;
int DirectVarargsCalls = 0;
int RegLibFlag = 0;
int PreLibFlag = 0;
int PostLibFlag = 0;
char *gateprefix = "";
char *libprefix = "";

char BaseName[64], BaseNamU[64], BaseNamL[64], BaseNamC[64];
char Buffer[512];

const static char *LibExcTable[]=
{
   "BattClockBase",	   "Node",
   "BattMemBase",	   "Node",
   "ConsoleDevice",	   "Device",
   "DiskBase",		   "DiskResource",
   "DOSBase",		   "DosLibrary",
   "SysBase",		   "ExecBase",
   "ExpansionBase",	   "ExpansionBase",
   "GfxBase",		   "GfxBase",
   "InputBase",		   "Device",
   "IntuitionBase",	   "IntuitionBase",
   "LocaleBase",	   "LocaleBase",
   "MathIeeeDoubBasBase",  "MathIEEEBase",
   "MathIeeeDoubTransBase","MathIEEEBase",
   "MathIeeeSingBasBase",  "MathIEEEBase",
   "MathIeeeSingTransBase","MathIEEEBase",
   "MiscBase",		   "Node",
   "PotgoBase",		   "Node",
   "RamdriveDevice",	   "Device",
   "RealTimeBase",	   "RealTimeBase",
   "RexxSysBase",	   "RxsLib",
   "TimerBase",		   "Device",
   "UtilityBase",	   "UtilityBase"
};
const char *StdLib; /* global lib-name ptr */

/*******************************************
 * just some support functions, no checking
 *******************************************/

char*
NewString(char** new, const char* old)
{
   const char *high;
   unsigned long len;

   while (*old && (*old==' ' || *old=='\t'))
      old++;
   len=strlen(old);
   for (high=old+len-1; high>=old && (*high==' ' || *high=='\t'); high--);
   high++;
   len=high-old;
   *new=malloc(1+len);
   if (*new)
   {
      strncpy(*new, old, len);
      (*new)[len]='\0';
   }
   else
      fprintf(stderr, "No mem for string\n");
   return *new;
}

static INLINE void
illparams(const char* funcname)
{
   fprintf(stderr, "%s: illegal Parameters\n", funcname);
}

static INLINE const char*
RegStr(regs reg)
{
   const static char *aosregs[]=
   {
      "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7",
      "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "illegal"
   },
   *posregs[]=
   {
      "__INLINE_REG_D0",
      "__INLINE_REG_D1",
      "__INLINE_REG_D2",
      "__INLINE_REG_D3",
      "__INLINE_REG_D4",
      "__INLINE_REG_D5",
      "__INLINE_REG_D6",
      "__INLINE_REG_D7",
      "__INLINE_REG_A0",
      "__INLINE_REG_A1",
      "__INLINE_REG_A2",
      "__INLINE_REG_A3",
      "__INLINE_REG_A4",
      "__INLINE_REG_A5",
      "__INLINE_REG_A6",
      "__INLINE_REG_A7",
      "illegal"
   };

   if (reg>illegal)
      reg=illegal;
   if (reg<d0)
      reg=d0;
   return (target!=M68K_POS ? aosregs[reg] : posregs[reg]);
}

static INLINE const char*
RegStrU(regs reg)
{
   const static char *aosregs[]=
   {
      "D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7",
      "A0", "A1", "A2", "A3", "A4", "A5", "A6", "A7", "illegal"
   };

   if (reg>illegal)
      reg=illegal;
   if (reg<d0)
      reg=d0;
   return (target!=M68K_POS ? aosregs[reg] : RegStr(reg));
}

static INLINE 

/******************************************************************************
 *    StrNRBrk
 *
 * searches string in from position at downwards, as long as in does not
 * contain any character in not.
 *
 ******************************************************************************/

const char*
StrNRBrk(const char* in, const char* not, const char* at)
{
   const char *chcheck;
   Error ready;

   chcheck=""; /* if at<in, the result will be NULL */
   for (ready=false; ready==false && at>=in;)
   {
      for (chcheck=not; *chcheck && *chcheck != *at; chcheck++);
      if (*chcheck)
	 ready=real_error;
      else
	 at--;
   }
   DBP(fprintf(stderr, "{%c}", *chcheck));
   return *chcheck ? at : NULL;
}

/*
  Our own "strupr", since it is a non-standard function.
*/
void
StrUpr(char* str)
{
   while (*str)
   {
      *str=toupper(*str);
      str++;
   }
}

int
MatchGlob( char* glob, char* str )
{
  while( *glob )
  {
    char c = *glob++;

    switch( c )
    {
      case '?':
	if( *str == 0 )
	{
	  return 0;
	}
	break;

      case '\\':
	c = *glob++;
	if( c == 0 || *str != c )
	{
	  return 0;
	}
	break;
	
      case '*':
	if( *glob == 0 )
	{
	  return 1;
	}

	while( *str )
	{
	  if( MatchGlob( glob, str ) )
	  {
	    return 1;
	  }
	  ++str;
	}
	return 0;

      default:
	if( *str != c )
	{
	  return 0;
	}
	break;
    }

    ++str;
  }

  return *str == 0;
}


/******************************************************************************
 *    CLASS fdFile
 *
 * stores a file with a temporary buffer (static length, sorry), a line number,
 * an offset (used for library offsets and an error field.
 * When there's no error, line will contain line #lineno and offset will be
 * the last offset set by the interpretation of the last line. If there's been
 * no ##bias line, this field assumes a bias of 30, which is the standard bias.
 * It is assumed offsets are always negative.
 ******************************************************************************/

#define fF_BUFSIZE 1024

/* all you need to know about an fdFile you parse */

typedef enum {FD_PRIVATE=1, FD_SHADOW=2} fdflags;

typedef struct
{
   FILE*         file;	      /* the file we're reading from	  */
   char	         line[fF_BUFSIZE]; /* the current line		  */
   unsigned long lineno;      /* current line number		  */
   long	         offset;      /* current fd offset (-bias)	  */
   Error         error;	      /* is everything o.k.		  */
   fdflags       flags;	      /* for ##private, ##shadow (p.OS)	  */
} fdFile;

fdFile*
fF_ctor	       (const char* fname);
static void
fF_dtor	       (fdFile* obj);
static void
fF_SetError    (fdFile* obj, Error error);
static void
fF_SetOffset   (fdFile* obj, long at);
Error
fF_readln      (fdFile* obj);
static Error
fF_GetError    (const fdFile* obj);
static long
fF_GetOffset   (const fdFile* obj);
char*
fF_FuncName    (fdFile* obj); /* return name or null */
static void
fF_SetFlags    (fdFile* obj, fdflags flags);
static fdflags
fF_GetFlags    (const fdFile* obj);

static INLINE void
fF_dtor(fdFile* obj)
{
  fclose(obj->file);
  free(obj);
}

static INLINE void
fF_SetError(fdFile* obj, Error error)
{
   if (obj)
      obj->error=error;
   else
      illparams("fF_SetError");
}

#define FUNCTION_GAP (target!=M68K_POS ? 6 : 12)

static INLINE void
fF_SetOffset(fdFile* obj, long at)
{
   if (obj)
      obj->offset= at;
   else
      illparams("fFSetOffset");
}

static INLINE void
fF_SetFlags(fdFile* obj, fdflags flags)
{
  if (obj)
    obj->flags=flags;
  else
    illparams("fF_SetFlags");
}

fdFile*
fF_ctor(const char* fname)
{
   fdFile *result;

   if (fname)
   {
      result=malloc(sizeof(fdFile));
      if (result)
      {
	 result->file=fopen(fname, "r");
	 if (result->file)
	 {
	    result->lineno=0;
	    fF_SetOffset(result, -30);
	    fF_SetError(result, false);
	    fF_SetFlags(result, 0);
	    result->line[0]='\0';
	 }
	 else
	 {
	    free(result);
	    result=NULL;
	 }
      }
   }
   else
   {
      result=NULL;
      illparams("fF_ctor");
   }
   return result;
}

Error
fF_readln(fdFile* obj)
{
   char *low, *bpoint;
   long glen,  /* the length we read until now */
   len;	       /* the length of the last segment */

   if (obj)
   {
      low=obj->line;
      glen=0;

      for (;;)
      {
	 obj->lineno++;
	 if (!fgets(low, fF_BUFSIZE-1-glen, obj->file))
	 {
	    fF_SetError(obj, real_error);
	    obj->line[0]='\0';
	    return real_error;
	 }
	 if (low==strpbrk(low, "*#/"))
	 {
	    DBP(fprintf(stderr, "in# %s\n", obj->line));
	    return false;
	 }
	 len=strlen(low);
	 bpoint=low+len-1;
	 while (len && isspace(*bpoint))
	 {
	    bpoint--;
	    len--;
	 }
	 if (*bpoint==';' || *bpoint==')')
	 {
	    DBP(fprintf(stderr, "\nin: %s\n", obj->line));
	    return false;
	 }
	 glen+=len;
	 low+=len;
	 if (glen>=fF_BUFSIZE-10) /* somewhat pessimistic? */
	 {
	    fF_SetError(obj, real_error);
	    fprintf(stderr, "Line %lu too long.\n", obj->lineno);
	    return real_error;
	 }
	 DBP(fprintf(stderr, "+"));
      }
   }
   illparams("fF_readln");
   return real_error;
}

static INLINE Error
fF_GetError(const fdFile* obj)
{
   if (obj)
      return obj->error;
   illparams("fF_GetError");
   return real_error;
}

static INLINE long
fF_GetOffset(const fdFile* obj)
{
   if (obj)
      return obj->offset;
   illparams("fF_GetOffset");
   return -1;
}

/******************************************************************************
 * fF_FuncName
 *
 * checks if it can find a function-name and return it's address, or NULL
 * if the current line does not seem to contain one. The return value will
 * be a pointer into a malloced buffer, thus the caller will have to free().
 ******************************************************************************/

char*
fF_FuncName(fdFile* obj)
{
   const char *lower;
   const char *upper;
   char *buf;
   long obraces;  /* count of open braces */
   Error ready;	  /* ready with searching */

   if (!obj || fF_GetError(obj)==real_error)
   {
      illparams("fF_FuncName");
      return NULL;
   }

   lower=obj->line;
   while (*lower && (*lower==' ' || *lower=='\t'))
      lower++;

   if (!*lower || (!isalpha(*lower) && *lower!='_'))
   {
      fF_SetError(obj, nodef);
      return NULL;
   }

   while (*lower)
   {
      if (!isalnum(*lower) && !isspace(*lower) && *lower!='*' && *lower!=','
      && *lower!='.' && *lower!=';' && *lower!='(' && *lower!=')' &&
      *lower!='[' && *lower!=']' && *lower!='_' && *lower!='\\')
      {
	 fF_SetError(obj, nodef);
	 return NULL;
      }
      lower++;
   }

   lower=NULL;
   buf=NULL;

   if (obj && fF_GetError(obj)==false)
   {
      if ((upper=strrchr(obj->line, ')'))!=0)
      {
	 DBP(fprintf(stderr, "end:%s:", upper));

	 for (obraces=1, ready=false; ready==false; upper=lower)
	 {
	    lower=StrNRBrk(obj->line, "()", --upper);
	    if (lower)
	    {
	       switch (*lower)
	       {
		  case ')':
		     obraces++;
		     DBP(fprintf(stderr, " )%ld%s", obraces, lower));
		     break;
		  case '(':
		     obraces--;
		     DBP(fprintf(stderr, " (%ld%s", obraces, lower));
		     if (!obraces)
			ready=nodef;
		     break;
		  default:
		     fprintf(stderr, "Faulty StrNRBrk\n");
	       }
	    }
	    else
	    {
	       fprintf(stderr, "'(' or ')' expected in line %lu.\n",
		  obj->lineno);
	       ready=real_error;
	    }
	 }
	 if (ready==nodef) /* we found the matching '(' */
	 {
	    long newlen;
	    const char* name;

	    upper--;

	    while (upper>=obj->line && (*upper==' ' || *upper=='\t'))
	       upper--;

	    lower=StrNRBrk(obj->line, " \t*)", upper);

	    if (!lower)
	       lower=obj->line;
	    else
	       lower++;

	    for (name=lower; name<=upper; name++)
	       if (!isalnum(*name) && *name!='_')
	       {
		  fF_SetError(obj, nodef);
		  return NULL;
	       }

	    newlen=upper-lower+1;
	    buf=malloc(newlen+1);

	    if (buf)
	    {
	       strncpy(buf, lower, newlen);
	       buf[newlen]='\0';
	    }
	    else
	       fprintf(stderr, "No mem for fF_FuncName");
	 }
      }
   }
   else
      illparams("fF_FuncName");
   return buf;
}

static INLINE fdflags
fF_GetFlags(const fdFile* obj)
{
   if (obj)
      return obj->flags;
   illparams("fF_GetFlags");
   return 0;
}

/*********************
 *    CLASS fdDef    *
 *********************/

typedef struct
{
   char* name;
   char* type;
   long	 offset;
   regs	 reg[REGS];
   char* param[REGS];
   char* proto[REGS];
   regs	 funcpar; /* number of argument that has type "pointer to function" */
} fdDef;

fdDef*
fD_ctor		  (void);
void
fD_dtor		  (fdDef* obj);
static void
fD_NewName	  (fdDef* obj, const char* newname);
void
fD_NewParam	  (fdDef* obj, shortcard at, const char* newstr);
int
fD_NewProto	  (fdDef* obj, shortcard at, char* newstr);
static void
fD_NewReg	  (fdDef* obj, shortcard at, regs reg);
static void
fD_NewType	  (fdDef* obj, const char* newstr);
static void
fD_SetOffset	  (fdDef* obj, long off);
Error
fD_parsefd	  (fdDef* obj, fdFile* infile);
Error
fD_parsepr	  (fdDef* obj, fdFile* infile);
static const char*
fD_GetName	  (const fdDef* obj);
static long
fD_GetOffset	  (const fdDef* obj);
static const char*
fD_GetParam	  (const fdDef* obj, shortcard at);
static regs
fD_GetReg	  (const fdDef* obj, shortcard at);
static const char*
fD_GetRegStr	  (const fdDef* obj, shortcard at);
static const char*
fD_GetRegStrU     (const fdDef* obj, shortcard at);
static const char*
fD_GetType	  (const fdDef* obj);
static shortcard
fD_ParamNum	  (const fdDef* obj);
static shortcard
fD_ProtoNum	  (const fdDef* obj);
static shortcard
fD_RegNum	  (const fdDef* obj);
int
fD_cmpName	  (const void* big, const void* small);
void
fD_write	  (FILE* outfile, const fdDef* obj);
static shortcard
fD_GetFuncParNum  (const fdDef* obj);
static void
fD_SetFuncParNum  (fdDef* obj, shortcard at);
static void
fD_adjustargnames(fdDef *obj);

fdDef **arrdefs;
long fds;

char *fD_nostring="";

fdDef*
fD_ctor(void)
{
   fdDef *result;
   regs count;

   result=malloc(sizeof(fdDef));

   if (result)
   {
      result->name=fD_nostring;
      result->type=fD_nostring;
      result->funcpar=illegal;

      for (count=d0; count<illegal; count++ )
      {
	 result->reg[count]=illegal;
	 result->param[count]=fD_nostring; /* if (!strlen) dont't free() */
	 result->proto[count]=fD_nostring;
      }
   }
   return result;
}

/* free all resources and make the object as illegal as possible */

void
fD_dtor(fdDef* obj)
{
   regs count;

   if (obj)
   {
      if (!obj->name)
	 fprintf(stderr, "fD_dtor: null name");
      else
	 if (obj->name!=fD_nostring)
	    free(obj->name);

      if (!obj->type)
	 fprintf(stderr, "fD_dtor: null type");
      else
	 if (obj->type!=fD_nostring)
	    free(obj->type);

      obj->name=obj->type=NULL;

      for (count=d0; count<illegal; count++)
      {
	 obj->reg[count]=illegal;

	 if (!obj->param[count])
	    fprintf(stderr, "fD_dtor: null param");
	 else
	    if (obj->param[count]!=fD_nostring)
	       free(obj->param[count]);

	 if (!obj->proto[count])
	    fprintf(stderr, "fD_dtor: null proto");
	 else
	    if (obj->proto[count]!=fD_nostring)
	       free(obj->proto[count]);

	 obj->param[count]=obj->proto[count]=NULL;
      }

      free(obj);
   }
   else
      fprintf(stderr, "fd_dtor(NULL)\n");
}

static INLINE void
fD_NewName(fdDef* obj, const char* newname)
{
   if (obj && newname)
   {
      if (obj->name && obj->name!=fD_nostring)
	 free(obj->name);
      if (!NewString(&obj->name, newname))
	 obj->name=fD_nostring;
   }
   else
      illparams("fD_NewName");
}

void
fD_NewParam(fdDef* obj, shortcard at, const char* newstr)
{
   char *pa;

   if (newstr && obj && at<illegal)
   {
      pa=obj->param[at];

      if (pa && pa!=fD_nostring)
	 free(pa);

      while (*newstr==' ' || *newstr=='\t')
	 newstr++;

      if (NewString(&pa, newstr))
      {
	 char* prefix_pa;

	 prefix_pa = malloc( strlen( pa ) + 4 );

	 if( prefix_pa == NULL )
	 {
	   fprintf(stderr, "No mem for string\n");
	 }
	 else
	 {
	   sprintf( prefix_pa, "___%s", pa );
	   obj->param[at]=prefix_pa;
	   free( pa );
	 }
      }
      else
	 obj->param[at]=fD_nostring;
   }
   else
      illparams("fD_NewParam");
}

/* get first free *reg or illegal */

static INLINE shortcard
fD_RegNum(const fdDef* obj)
{
   shortcard count;

   if (obj)
   {
      for (count=d0; count<illegal && obj->reg[count]!=illegal; count++);
      return count;
   }
   else
   {
      illparams("fD_RegNum");
      return illegal;
   }
}

static INLINE void
fD_NewReg(fdDef* obj, shortcard at, regs reg)
{
   if (obj && at<illegal && reg>=d0 && reg<=illegal)
      obj->reg[at]=reg;
   else
      illparams("fD_NewReg");
}

static INLINE regs
fD_GetReg(const fdDef* obj, shortcard at)
{
   if (obj && at<illegal)
      return obj->reg[at];
   else
   {
      illparams("fD_GetReg");
      return illegal;
   }
}

static INLINE shortcard
fD_GetFuncParNum(const fdDef* obj)
{
   if (obj)
      return (shortcard)obj->funcpar;
   else
   {
      illparams("fD_GetFuncParNum");
      return illegal;
   }
}

static INLINE void
fD_SetFuncParNum(fdDef* obj, shortcard at)
{
   if (obj && at<illegal)
      obj->funcpar=at;
   else
      illparams("fD_SetFuncParNum");
}

int
fD_NewProto(fdDef* obj, shortcard at, char* newstr)
{
   char *pr;

   if (newstr && obj && at<illegal)
   {
      char *t, arr[200]; /* I hope 200 will be enough... */
      int numwords=1;
      pr=obj->proto[at];

      if (pr && pr!=fD_nostring)
	 free(pr);

      while (*newstr==' ' || *newstr=='\t')
	 newstr++; /* Skip leading spaces */

      t=arr;
      while ((*t++=*newstr)!=0)
      {
	 /* Copy the rest, counting number of words */
	 if ((*newstr==' ' || *newstr=='\t') && newstr[1] && newstr[1]!=' ' &&
	 newstr[1]!='\t')
	    numwords++;
	 newstr++;
      }

      t=arr+strlen(arr)-1;
      while (*t==' ' || *t=='\t')
	 t--;
      t[1]='\0'; /* Get rid of tailing spaces */

      if (at!=fD_GetFuncParNum(obj))
      {
	 if (numwords>1) /* One word - must be type */
	    if (*t!='*')
	    {
	       /* '*' on the end - no parameter name used */
	       while (*t!=' ' && *t!='\t' && *t!='*')
		  t--;
	       t++;
	       if (strcmp(t, "char") && strcmp(t, "short") && strcmp(t, "int")
	       && strcmp(t, "long") && strcmp(t, "APTR"))
	       {
		  /* Not one of applicable keywords - must be parameter name.
		     Get rid of it. */
		  t--;
		  while (*t==' ' || *t=='\t')
		     t--;
		  t[1]='\0';
	       }
	    }
      }
      else
      {
	 /* Parameter of type "pointer to function". */
	 char *end;
	 t=strchr(arr, '(');
	 t++;
	 while (*t==' ' || *t=='\t')
	    t++;
	 if (*t!='*')
	    return 1;
	 t++;
	 end=strchr(t, ')');
	 if (target!=M68K_POS)
	 {
	    memmove(t+2, end, strlen(end)+1);
	    *t='%';
	    t[1]='s';
	 }
	 else
	    memmove(t, end, strlen(end)+1);
      }

      if (NewString(&pr, arr))
      {
	 obj->proto[at]=pr;
	 while (*pr==' ' || *pr=='\t')
	    pr++;
	 if (!strcasecmp(pr, "double"))
	 {
	    /* "double" needs two data registers */
	    int count, regs=fD_RegNum(obj);
	    for (count=at+1; count<regs; count++)
	       fD_NewReg(obj, count, fD_GetReg(obj, count+1));
	 }
      }
      else
	 obj->proto[at]=fD_nostring;
   }
   else
      illparams("fD_NewProto");

   return 0;
}

static INLINE void
fD_NewType(fdDef* obj, const char* newtype)
{
   if (obj && newtype)
   {
      if (obj->type && obj->type!=fD_nostring)
	 free(obj->type);
      if (!NewString(&obj->type, newtype))
	 obj->type=fD_nostring;
   }
   else
      illparams("fD_NewType");
}

static INLINE void
fD_SetOffset(fdDef* obj, long off)
{
   if (obj)
      obj->offset=off;
   else
      illparams("fD_SetOffset");
}

static INLINE const char*
fD_GetName(const fdDef* obj)
{
   if (obj && obj->name)
      return obj->name;
   else
   {
      illparams("fD_GetName");
      return fD_nostring;
   }
}

static INLINE long
fD_GetOffset(const fdDef* obj)
{
   if (obj)
      return obj->offset;
   else
   {
      illparams("fD_GetOffset");
      return 0;
   }
}

static INLINE const char*
fD_GetProto(const fdDef* obj, shortcard at)
{
   if (obj && at<illegal && obj->proto[at])
      return obj->proto[at];
   else
   {
      illparams("fD_GetProto");
      return fD_nostring;
   }
}

static INLINE const char*
fD_GetParam(const fdDef* obj, shortcard at)
{
   if (obj && at<illegal && obj->param[at])
      return obj->param[at];
   else
   {
      illparams("fD_GetParam");
      return fD_nostring;
   }
}

static INLINE const char*
fD_GetRegStr(const fdDef* obj, shortcard at)
{
   if (obj && at<illegal)
      return RegStr(obj->reg[at]);
   else
   {
      illparams("fD_GetReg");
      return RegStr(illegal);
   }
}

static INLINE const char*
fD_GetRegStrU(const fdDef* obj, shortcard at)
{
   if (obj && at<illegal)
      return RegStrU(obj->reg[at]);
   else
   {
      illparams("fD_GetReg");
      return RegStrU(illegal);
   }
}

static INLINE const char*
fD_GetType(const fdDef* obj)
{
   if (obj && obj->type)
      return obj->type;
   else
   {
      illparams("fD_GetType");
      return fD_nostring;
   }
}

/* get first free param or illegal */

static INLINE shortcard
fD_ParamNum(const fdDef* obj)
{
   shortcard count;

   if (obj)
   {
      for (count=d0; count<illegal && obj->param[count]!=fD_nostring;
      count++);
      return count;
   }
   else
   {
      illparams("fD_ParamNum");
      return illegal;
   }
}

static INLINE shortcard
fD_ProtoNum(const fdDef* obj)
{
   shortcard count;

   if (obj)
   {
      for (count=d0; count<illegal && obj->proto[count]!=fD_nostring;
      count++);
      return count;
   }
   else
   {
      illparams("fD_ProtoNum");
      return illegal;
   }
}

/******************************************************************************
 *    fD_parsefd
 *
 *  parse the current line. Needs to copy input, in order to insert \0's
 *  RETURN
 *    fF_GetError(infile):
 * false = read a definition.
 * nodef = not a definition on line (so try again)
 * error = real error
 ******************************************************************************/

Error
fD_parsefd(fdDef* obj, fdFile* infile)
{
   enum parse_info { name, params, regs, ready } parsing;
   char *buf, *bpoint, *bnext;
   unsigned long index;

   if (obj && infile && fF_GetError(infile)==false)
   {
      parsing=name;

      if (!NewString(&buf, infile->line))
      {
	 fprintf(stderr, "No mem for line %lu\n", infile->lineno);
	 fF_SetError(infile, real_error);
      }
      bpoint=buf; /* so -Wall keeps quiet */

      /* try to parse the line until there's an error or we are done */

      while (parsing!=ready && fF_GetError(infile)==false)
      {
	 switch (parsing)
	 {
	    case name:
	       switch (buf[0])
	       {
		  case '#':
		     if (strncmp("##base", buf, 6)==0)
		     {
			bnext=buf+6;
			while (*bnext==' ' || *bnext=='\t' || *bnext=='_')
			   bnext++;
			strcpy(BaseName, bnext);
			BaseName[strlen(BaseName)-1]='\0';
		     }
		     else
			if (strncmp("##bias", buf, 6)==0)
			{
			   if (!sscanf(buf+6, "%ld", &infile->offset))
			   {
			      fprintf(stderr, "Illegal ##bias in line %lu: %s\n",
				 infile->lineno, infile->line);
			      fF_SetError(infile, real_error);
			      break; /* avoid nodef */
			   }
			   else
			   {
			      if (fF_GetOffset(infile)>0)
				 fF_SetOffset(infile, -fF_GetOffset(infile));
			      DBP(fprintf(stderr, "set offset to %ld\n",
				 fF_GetOffset(infile)));
			   }
			}
			else
			{
			   if (strncmp("##private", buf, 9)==0)
			      fF_SetFlags(infile, fF_GetFlags(infile) |
				 FD_PRIVATE);
			   else if (strncmp("##public", buf, 8)==0)
			      fF_SetFlags(infile, fF_GetFlags(infile) &
				 ~FD_PRIVATE);
			   else if (strncmp("##shadow", buf, 8)==0)
			      fF_SetFlags(infile, fF_GetFlags(infile) |
				 FD_SHADOW);
			}
		     /* drop through for error comment */

		  case '*':
		     /* try again somewhere else */
		     fF_SetError(infile, nodef);
			break;

		  default:
		     /* assume a regular line here */
		     if (fF_GetFlags(infile) & (FD_PRIVATE | FD_SHADOW))
		     {
			/* don't store names of privates */
			fF_SetError(infile, nodef);
			if (!(fF_GetFlags(infile) & FD_SHADOW))
			   fF_SetOffset(infile,
			      fF_GetOffset(infile)-FUNCTION_GAP);
			else
			   /* Shadow is valid for one line only. */
			   fF_SetFlags(infile, fF_GetFlags(infile) &
			      ~FD_SHADOW);
			break;
		     }
		     parsing=name; /* switch (parsing) */
		     for (index=0; buf[index] && buf[index]!='('; index++);

		     if (!buf[index])
		     {
			/* oops, no fd ? */
			fprintf(stderr, "Not an fd, line %lu: %s\n",
			   infile->lineno, buf /* infile->line */);
			fF_SetError(infile, nodef);
		     } /* maybe next time */
		     else
		     {
			buf[index]=0;

			fD_NewName(obj, buf);
			fD_SetOffset(obj, fF_GetOffset(infile));

			bpoint=buf+index+1;
			parsing=params; /* continue the loop */
		     }
	       }
	       break;

	    case params:
	    {
	       char *bptmp; /* needed for fD_NewParam */

	       /* look for parameters now */

	       for (bnext = bpoint; *bnext && *bnext!=',' && *bnext!=')';
	       bnext++);

	       if (*bnext)
	       {
		  bptmp=bpoint;

		  if (*bnext == ')')
		  {
		     if (bnext[1] != '(')
		     {
			fprintf(stderr, "Registers expected in line %lu: %s\n",
			   infile->lineno, infile->line);
			fF_SetError(infile, nodef);
		     }
		     else
		     {
			parsing=regs;
			bpoint=bnext+2;
		     }
		  }
		  else
		     bpoint = bnext+1;

		  /* terminate string and advance to next item */

		  *bnext='\0';
		  fD_NewParam(obj, fD_ParamNum(obj), bptmp);
	       }
	       else
	       {
		  fF_SetError(infile, nodef);
		  fprintf(stderr, "Param expected in line %lu: %s\n",
		     infile->lineno, infile->line);
	       }
	       break;  /* switch parsing */
	    }

	    case regs:
	       /* look for parameters now */

	       for (bnext=bpoint; *bnext && *bnext!='/' && *bnext!=',' &&
	       *bnext!=')'; bnext++);

	       if (*bnext)
	       {
		  if (')'==*bnext)
		  {
		     /* wow, we've finished */
		     fF_SetOffset(infile, fF_GetOffset(infile)-FUNCTION_GAP);
		     parsing=ready;
		  }
		  *bnext = '\0';

		  bpoint[0]=tolower(bpoint[0]);

		  if ((bpoint[0]=='d' || bpoint[0]=='a') && bpoint[1]>='0' &&
		  bpoint[1]<='8' && bnext==bpoint+2)
		     fD_NewReg(obj, fD_RegNum(obj),
			bpoint[1]-'0'+(bpoint[0]=='a'? 8 : 0));
		  else
		     if (bnext!=bpoint)
		     {
			/* it is when our function is void */
			fprintf(stderr, "Illegal register %s in line %ld\n",
			   bpoint, infile->lineno);
			fF_SetError(infile, nodef);
		     }
		  bpoint = bnext+1;
	       }
	       else
	       {
		  fF_SetError(infile, nodef);
		  fprintf(stderr, "Reg expected in line %lu\n",
		     infile->lineno);
	       }
	       break; /* switch parsing */

	    case ready:
	       fprintf(stderr, "Internal error, use another compiler.\n");
	       break;
	 }
      }

      free(buf);
      return fF_GetError(infile);
   }
   else
   {
      illparams("fD_parsefd");
      return real_error;
   }
}

static void
fD_adjustargnames(fdDef *obj)
{
   int parnum;

   if (output_mode!=NEW)
      return;

   /* For #define-base output mode, we have to check if argument names are not
      the same as some words in type names. We check from the first argument
      to the last, resolving conflicts by changing argument names, if
      necessary. */

   for (parnum=0; parnum<fD_ParamNum(obj); parnum++)
   {
      const char *parname=fD_GetParam(obj, parnum);
      int finished;
      do
      {
	 int num;
	 const char *type=fD_GetType(obj);
	 char *str;

	 finished=1;

	 if ((str=strstr(type, parname))!=0 && (str==type ||
	 (!isalnum(str[-1]) && str[-1]!='_')) &&
	 (!*(str+=strlen(parname)) || (!isalnum(*str) && *str!='_')))
	 {
	    char buf[300]; /* Hope will be enough... */
	    strcpy(buf, parname);
	    strcat(buf, "_");
	    fD_NewParam(obj, parnum, buf);
	    parname=fD_GetParam(obj, parnum);	
	    finished=0;
	 }
	 else
	    for (num=0; num<fD_ParamNum(obj); num++)
	    {
	       const char *name=fD_GetParam(obj, num);
	       const char *proto=fD_GetProto(obj, num);
	       if ((num<parnum && strcmp(name, parname)==0) ||
	       ((str=strstr(proto, parname))!=0 && (str==proto ||
	       (!isalnum(str[-1]) && str[-1]!='_')) &&
	       (!*(str+=strlen(parname)) || (!isalnum(*str) && *str!='_'))))
	       {
		  char buf[300]; /* Hope will be enough... */
		  strcpy(buf, parname);
		  strcat(buf, "_");
		  fD_NewParam(obj, parnum, buf);
		  parname=fD_GetParam(obj, parnum);   
// lcs		  finished=0;
		  break;
	       }
	    }
      } while (!finished);
   }
}

Error
fD_parsepr(fdDef* obj, fdFile* infile)
{
   char	 *buf;	  /* a copy of infile->line		       */
   char	 *bpoint, /* cursor in buf			       */
	 *bnext,  /* looking for the end		       */
	 *lowarg; /* beginning of this argument		       */
   long	 obraces; /* count of open braces		       */
   regs	 count,	  /* count parameter number		       */
	 args;	  /* the number of arguments for this function */

   if (!(obj && infile && fF_GetError(infile)==false))
   {
      illparams("fD_parsepr");
      fF_SetError(infile, real_error);
      return real_error;
   }
   if (!NewString(&buf, infile->line))
   {
      fprintf(stderr, "No mem for fD_parsepr\n");
      fF_SetError(infile, real_error);
      return real_error;
   }
   fF_SetError(infile, false);

   bpoint=strchr(buf, '(');
   while (--bpoint>=buf && strstr(bpoint, fD_GetName(obj))!=bpoint);
   if (bpoint>=buf)
   {
      while (--bpoint >= buf && (*bpoint==' ' || *bpoint=='\t'));
      *++bpoint='\0';

      fD_NewType(obj, buf);

      while (bpoint && *bpoint++!='('); /* one beyond '(' */

      lowarg=bpoint;
      obraces=0;

      for (count=0, args=fD_RegNum(obj); count<args; bpoint=bnext+1)
      {
	 while (*bpoint && (*bpoint==' ' || *bpoint=='\t')) /* ignore spaces */
	    bpoint++;

	 if (!obraces && target==M68K_POS && strncmp(bpoint, "_R_", 3)==0 &&
	 isalnum(bpoint[3]) && isalnum(bpoint[4]) && isspace(bpoint[5]))
	    lowarg=bpoint+5;

	 bnext=strpbrk(bpoint, "(),");

	 if (bnext)
	 {
	    switch (*bnext)
	    {
	       case '(':
		  if (!obraces)
		  {
		     if (target==M68K_AMIGAOS || target==M68K_POS)
		     {
			if (fD_GetFuncParNum(obj)!=illegal &&
			    fD_GetFuncParNum(obj)!=count &&
			    !Quiet)
			   fprintf(stderr, "Warning: two parameters of type "
				   "pointer to function are used.\n"
				   "This is not supported!\n");
		     }

		     fD_SetFuncParNum(obj, count);
		  }
		  obraces++;
		  DBP(fprintf(stderr, "< (%ld%s >", obraces, bnext));
		  break;

	       case ')':
		  if (obraces)
		  {
		     DBP(fprintf(stderr, "< )%ld%s >", obraces, bnext));
		     obraces--;
		  }
		  else
		  {
		     *bnext='\0';
		     DBP(fprintf(stderr, "< )0> [LAST PROTO=%s]", lowarg));
		     if (fD_NewProto(obj, count, lowarg))
			fprintf(stderr, "Parser confused in line %ld\n",
			      infile->lineno);
		     lowarg=bnext+1;

		     if (count!=args-1)
		     {
			DBP(fprintf(stderr, "%s needs %u arguments and got %u.\n",
			   fD_GetName(obj), args, count+1));
			fF_SetError(infile, nodef);
		     }
		     count++;
		  }
		  break;

	       case ',':
		  if (!obraces)
		  {
		     *bnext='\0';
		     DBP(fprintf(stderr, " [PROTO=%s] ", lowarg));
		     if (fD_NewProto(obj, count, lowarg))
			fprintf(stderr, "Parser confused in line %ld\n",
			      infile->lineno);
		     lowarg=bnext+1;
		     count++;
		  }
		  break;

	       default:
		  fprintf(stderr, "Faulty strpbrk in line %lu.\n",
		     infile->lineno);
	    }
	 }
	 else
	 {
	    DBP(fprintf(stderr, "Faulty argument %u in line %lu.\n", count+1,
	       infile->lineno));
	    count=args; /* this will effectively quit the for loop */
	    fF_SetError(infile, nodef);
	 }
      }
      if (fD_ProtoNum(obj)!=fD_RegNum(obj))
	 fF_SetError(infile, nodef);
   }
   else
   {
      fprintf(stderr, "fD_parsepr was fooled in line %lu\n", infile->lineno);
      fprintf(stderr, "function , definition %s.\n",
	 /* fD_GetName(obj),*/ infile->line);
      fF_SetError(infile, nodef);
   }

   free(buf);

   fD_adjustargnames(obj);

   return fF_GetError(infile);
}

int
fD_cmpName(const void* big, const void* small) /* for qsort and bsearch */
{
   return strcmp(fD_GetName(*(fdDef**)big), fD_GetName(*(fdDef**)small));
}

const static char *TagExcTable[]=
{
   "BuildEasyRequestArgs", "BuildEasyRequest",
   "DoDTMethodA",	   "DoDTMethod",
   "DoGadgetMethodA",	   "DoGadgetMethod",
   "EasyRequestArgs",	   "EasyRequest",
   "MUI_MakeObjectA",	   "MUI_MakeObject",
   "MUI_RequestA",	   "MUI_Request",
   "PrintDTObjectA",	   "PrintDTObject",
   "RefreshDTObjectA",     "RefreshDTObjects",
   "UMSVLog",		   "UMSLog",
   "VFWritef",		   "FWritef",
   "VFPrintf",		   "FPrintf",
   "VPrintf",		   "Printf",
};

const char*
getvarargsfunction(const fdDef * obj)
{
   unsigned int count;
   const char *name = fD_GetName(obj);
    
   for (count=0; count<sizeof TagExcTable/sizeof TagExcTable[0]; count+=2)
   {
      if (strcmp(name, TagExcTable[count])==0)
      {
	 return TagExcTable[count+1];
      }
   }
   return(NULL);
}

const char*
taggedfunction(const fdDef* obj)
{
   shortcard numregs=fD_RegNum(obj);
   unsigned int count;
   int aos_tagitem;
   const char *name=fD_GetName(obj);
   static char newname[200];  /* Hope will be enough... static because used
				 out of the function. */
   const char *lastarg;
   const static char *TagExcTable2[]=
   {
      "ApplyTagChanges",
      "CloneTagItems",
      "FindTagItem",
      "FreeTagItems",
      "GetTagData",
      "PackBoolTags",
      "PackStructureTags",
      "RefreshTagItemClones",
      "UnpackStructureTags",
   };

   if (!numregs)
      return NULL;

   for (count=0; count<sizeof TagExcTable/sizeof TagExcTable[0]; count+=2)
      if (strcmp(name, TagExcTable[count])==0)
	 return NULL;
// lcs	 return TagExcTable[count+1];

   for (count=0; count<sizeof TagExcTable2/sizeof TagExcTable2[0]; count++)
      if (strcmp(name, TagExcTable2[count])==0)
	 return NULL;

   lastarg=fD_GetProto(obj, numregs-1);
   if (strncmp(lastarg, "const", 5)==0 || strncmp(lastarg, "CONST", 5)==0)
      lastarg+=5;
   while (*lastarg==' ' || *lastarg=='\t')
      lastarg++;
   if (strncmp(lastarg, "struct", 6))
      return NULL;
   lastarg+=6;
   while (*lastarg==' ' || *lastarg=='\t')
      lastarg++;
   aos_tagitem=1;
   if (strncmp(lastarg, "TagItem", 7) &&
   (target!=M68K_POS || ((aos_tagitem=strncmp(lastarg, "pOS_TagItem", 11))!=0)))
      return NULL;
   lastarg+=(aos_tagitem ? 7 : 11);
   while (*lastarg==' ' || *lastarg=='\t')
      lastarg++;
   if (strcmp(lastarg, "*"))
      return NULL;

   strcpy(newname, name);
   if (newname[strlen(newname)-1]=='A')
      newname[strlen(newname)-1]='\0';
   else
      if (strlen(newname)>7 && !strcmp(newname+strlen(newname)-7, "TagList"))
	 strcpy(newname+strlen(newname)-4, "s");
      else
	 strcat(newname, "Tags");
   return newname;
}

const char*
aliasfunction(const char* name)
{
   const static char *AliasTable[]=
   {
      "AllocDosObject", "AllocDosObjectTagList",
      "CreateNewProc",	"CreateNewProcTagList",
      "NewLoadSeg",	"NewLoadSegTagList",
      "System",		"SystemTagList",
   };
   unsigned int count;
   for (count=0; count<sizeof AliasTable/sizeof AliasTable[0]; count++)
      if (strcmp(name, AliasTable[count])==0)
	 return AliasTable[count+(count%2 ? -1 : 1)];
   return NULL;
}

void
fD_write(FILE* outfile, const fdDef* obj)
{
   shortcard count, numregs;
   const char *chtmp, *tagname, *varname, *name, *rettype;
   int vd=0, a45=0, d7=0;

   DBP(fprintf(stderr, "func %s\n", fD_GetName(obj)));

   numregs=fD_RegNum(obj);

   if ((rettype=fD_GetType(obj))==fD_nostring)
   {
      fprintf(stderr, "%s has no prototype.\n", fD_GetName(obj));
      return;
   }
   if (!strcasecmp(rettype, "void"))
      vd = 1; /* set flag */
   for (count=d0; count<numregs; count++)
   {
      const char *reg=fD_GetRegStr(obj, count);
      if (!((output_mode == NEW) && (target == PPC_POWERUP)))
      {
	 if (strcmp(reg, "a4")==0 || strcmp(reg, "a5")==0)
	 {
	    if (!a45)
	       a45=(strcmp(reg, "a4") ? 5 : 4); /* set flag */
	    else /* Security check */
	       if (!Quiet)
	          fprintf(stderr, "Warning: both a4 and a5 are used. "
			  "This is not supported!\n");
	 }
      }
      if (strcmp(reg, "d7")==0) /* Used only when a45!=0 */
	 d7=1;
   }
   
   if (!((output_mode == NEW) && (target == PPC_POWERUP)))
   {
      if (a45 && d7) /* Security check */
	 if (!Quiet)
	    fprintf(stderr, "Warning: d7 and a4 or a5 are used. This is not "
		    "supported!\n");
   }

   name=fD_GetName(obj);

   if (fD_ProtoNum(obj)!=numregs)
   {
      fprintf(stderr, "%s gets %d fd args and %d proto%s.\n", name, numregs,
	 fD_ProtoNum(obj), fD_ProtoNum(obj)!= 1 ? "s" : "");
      return;
   }

   if (output_mode==NEW)
   {
      fprintf(outfile, "#define %s(", name);

      if (numregs>0)
      {
	 for (count=d0; count<numregs-1; count++)
	    fprintf(outfile, "%s, ", fD_GetParam(obj, count));
	 fprintf(outfile, "%s", fD_GetParam(obj, count));
      }
      
      if (target==M68K_AMIGAOS)
      {
	 fprintf(outfile, ") \\\n\tLP%d%s%s%s%s(0x%lx, ", numregs,
		 (vd ? "NR" : ""), (a45 ? (a45==4 ? "A4" : "A5") : ""),
		 (BaseName[0] ? "" : "UB"),
		 (fD_GetFuncParNum(obj)==illegal ? "" : "FP"), -fD_GetOffset(obj));
	 if (!vd)
	    fprintf(outfile, "%s, ", rettype);
	 fprintf(outfile, "%s, ", name);

	 for (count=d0; count<numregs; count++)
	 {
	    chtmp=fD_GetRegStr(obj, count);
	    if (a45 && (strcmp(chtmp, "a4")==0 || strcmp(chtmp, "a5")==0))
	       chtmp="d7";
	    fprintf(outfile, "%s, %s, %s%s", (fD_GetFuncParNum(obj)==count ?
					      "__fpt" : fD_GetProto(obj, count)),
		    fD_GetParam(obj, count),
		    chtmp, (count==numregs-1 && !BaseName[0] ? "" : ", "));
	 }

	 if (BaseName[0]) /* was "##base" used? */
	    fprintf(outfile, "\\\n\t, %s_BASE_NAME", BaseNamU);
	 if (fD_GetFuncParNum(obj)!=illegal)
	 {
	    fprintf(outfile, ", ");
	    fprintf(outfile, fD_GetProto(obj, fD_GetFuncParNum(obj)), "__fpt");
	 }
	 fprintf(outfile, ")\n\n");
      }
      else if(target==M68K_POS)
      {
	 fprintf(outfile, ") \\\n\t__INLINE_FUN_%d(", numregs);
	 fprintf(outfile, "__%s_BASE_NAME, __%s_LIB_NAME, 0x%lx, %s, %s%s",
		 BaseNamU, BaseNamU, -fD_GetOffset(obj), rettype, name,
		 (numregs ? ", \\\n\t" : ""));

	 for (count=d0; count<numregs; count++)
	    fprintf(outfile, "%s, %s, %s%s", fD_GetProto(obj, count),
		    fD_GetParam(obj, count), fD_GetRegStr(obj, count),
		    (count==numregs-1 ? "" : ", "));
	 fprintf(outfile, ")\n\n");
      }
      else if (target==PPC_POWERUP || target==PPC_MORPHOS)
      {
	 fprintf(outfile, ") \\\n\tLP%d%s%s(0x%lx, ",
		 numregs,
		 (vd ? "NR" : ""),
		 (BaseName[0] ? "" : "UB"),
		 -fD_GetOffset(obj));
	 
	 if (!vd)
	    fprintf(outfile, "%s, ", rettype);
	 fprintf(outfile, "%s, ", name);

	 for (count=d0; count<numregs; count++)
	 {
	    chtmp=fD_GetRegStr(obj, count);

	    if (strchr(fD_GetProto(obj, count),'%'))
	    {
	       sprintf(Buffer,
		       fD_GetProto(obj, count),
		       "");

	       fprintf(outfile, "%s, %s, %s%s",
		       Buffer,
		       fD_GetParam(obj, count),
		       chtmp,
		       (count == numregs - 1 && !BaseName[0] ? "" : ", "));
	    }
	    else
	    {
	       fprintf(outfile, "%s, %s, %s%s",
		       fD_GetProto(obj, count),//(fD_GetFuncParNum(obj) == count ? "__fpt" : fD_GetProt\o(obj, count)),
		       fD_GetParam(obj, count),
		       chtmp,
		       (count == numregs - 1 && !BaseName[0] ? "" : ", "));
	    }
	 }
	 
	 if (BaseName[0])
	    fprintf(outfile, "\\\n\t, %s_BASE_NAME", BaseNamU);

	 /*
	  * Here it would make sense to create a database file to
	  * integrate optimizations automaticly into every new
	  * build. Not every function needs a complete flush. For
	  * example functions with no parameter wouldn`t need a
	  * PPC flush normally. Or Read(File,Addr,Size); would
	  * only need a flush for Addr with the Size
	  */
	 
	 fprintf(outfile, ", IF_CACHEFLUSHALL, NULL, 0, IF_CACHEFLUSHALL, NULL, 0");
	 fprintf(outfile, ")\n\n");
      }
      else if (target==IX86BE_AMITHLON)
      {
	 fprintf(outfile, ") \\\n\tLP%d%s%s(0x%lx, ",
		 numregs,
		 (vd ? "NR" : ""),
		 (BaseName[0] ? "" : "UB"),
		 -fD_GetOffset(obj));
	 
	 if (!vd)
	    fprintf(outfile, "%s, ", rettype);
	 fprintf(outfile, "%s, ", name);

	 for (count=d0; count<numregs; count++)
	 {
	    chtmp=fD_GetRegStr(obj, count);

	    if (strchr(fD_GetProto(obj, count),'%'))
	    {
	       sprintf(Buffer,
		       fD_GetProto(obj, count),
		       "");

	       fprintf(outfile, "%s, %s, %s%s",
		       Buffer,
		       fD_GetParam(obj, count),
		       chtmp,
		       (count == numregs - 1 && !BaseName[0] ? "" : ", "));
	    }
	    else
	    {
	       fprintf(outfile, "%s, %s, %s%s",
		       fD_GetProto(obj, count),
		       fD_GetParam(obj, count),
		       chtmp,
		       (count == numregs - 1 && !BaseName[0] ? "" : ", "));
	    }
	 }
	 
	 if (BaseName[0])
	    fprintf(outfile, "\\\n\t, %s_BASE_NAME", BaseNamU);

	 fprintf(outfile, ")\n\n");
      }
      else if (target==AROS)
      {
	 fprintf(outfile, ") \\\n\tAROS_LC%d%s(%s, %s, \\\n",
		 numregs,
		 (BaseName[0] ? "" : "I"),
		 (vd ? "void" : rettype),
		 name);
	 
	 for (count=d0; count<numregs; count++)
	 {
	    chtmp=fD_GetRegStrU(obj, count);
	    fprintf(outfile, "\tAROS_LCA(%s, (%s), %s), \\\n",
		    fD_GetProto(obj, count),
		    fD_GetParam(obj, count),
		    chtmp);
	 }

	 if (BaseName[0]) /* was "##base" used? */
	 {
	    fprintf(outfile, "\tstruct %s *, %s_BASE_NAME, ", StdLib, BaseNamU);
	 }
	 else
	 {
	   fprintf(outfile, "\t/* bt */, /* bn */, ");
	 }
	 fprintf(outfile,
		 "%ld, /* s */)\n\n",
		 -fD_GetOffset(obj) / 6);
      }
      else
      {
	 fprintf(stderr, "Internal error: Unknown target in fD_write().\n");
	 return;
      }
   }
   else if (output_mode==OLD || output_mode==STUBS)
   {
      fprintf(outfile, "%s__inline %s\n%s(%s",
	      (output_mode==STUBS ? "" : "extern "), rettype, name,
	      (BaseName[0] ? (numregs ? "BASE_PAR_DECL " : "BASE_PAR_DECL0") : ""));
      
      if (target==M68K_AMIGAOS)
      {
	 for (count=d0; count<numregs; count++)
	 {
	    chtmp=fD_GetProto(obj, count);
	    if (fD_GetFuncParNum(obj)==count)
	       fprintf(outfile, chtmp, fD_GetParam(obj, count));
	    else
	       fprintf(outfile, "%s%s%s", chtmp, (*(chtmp+strlen(chtmp)-1)=='*' ?
						  "" : " "), fD_GetParam(obj, count));
	    if (count<numregs-1)
	       fprintf(outfile, ", ");
	 }

	 fprintf(outfile, ")\n{\n%s", (BaseName[0] ? "   BASE_EXT_DECL\n" : ""));
	 if (!vd)
	    fprintf(outfile, "   register %s%sres __asm(\"d0\");\n", rettype,
		    (*(rettype+strlen(rettype)-1)=='*' ? "" : " "));

	 if (BaseName[0])
	    fprintf(outfile, "   register struct %s *a6 __asm(\"a6\") = BASE_NAME;\n",
		    StdLib);

	 for (count=d0; count<numregs; count++)
	 {
	    chtmp=fD_GetRegStr(obj, count);
	    if (a45 && (strcmp(chtmp, "a4")==0 || strcmp(chtmp, "a5")==0))
	       chtmp="d7";
	    if (fD_GetFuncParNum(obj)==count)
	    {
	       fprintf(outfile, "	 register ");
	       fprintf(outfile, fD_GetProto(obj, count), chtmp);
	       fprintf(outfile, " __asm(\"%s\") = %s;\n", chtmp, fD_GetParam(obj,
									     count));
	    }
	    else
	    {
	       const char *proto=fD_GetProto(obj, count);
	       fprintf(outfile, "	 register %s%s%s __asm(\"%s\") = %s;\n",
		       proto, (*(proto+strlen(proto)-1)=='*' ? "" : " "), chtmp,
		       chtmp, fD_GetParam(obj, count));
	    }
	 }
	 if (a45)
	    fprintf(outfile, "   __asm volatile (\"exg d7,%s\\n\\t"
		    "jsr a6@(-0x%lx:W)\\n\\texg d7,%s\"\n", (a45==4 ? "a4" : "a5"),
		    -fD_GetOffset(obj), (a45==4 ? "a4" : "a5"));
	 else
	    fprintf(outfile, "   __asm volatile (\"jsr a6@(-0x%lx:W)\"\n",
		    -fD_GetOffset(obj));

	 fprintf(outfile, (vd ? "	 : /* No Output */\n" : "   : \"=r\" (res)\n"));

	 fprintf(outfile, "   : ");
	 if (BaseName[0])
	    fprintf(outfile, "\"r\" (a6)%s", (numregs ? ", ": ""));

	 for (count=d0; count<numregs; count++)
	 {
	    chtmp=fD_GetRegStr(obj, count);
	    if (a45 && (strcmp(chtmp, "a4")==0 || strcmp(chtmp, "a5")==0))
	       chtmp="d7";
	    fprintf(outfile, "\"r\" (%s)%s", chtmp, (count<numregs-1 ? ", " : ""));
	 }
	 fprintf(outfile, "\n   : \"d0\", \"d1\", \"a0\", \"a1\", \"fp0\", \"fp1\"");

	 if (vd)
	    fprintf(outfile, ", \"cc\", \"memory\");\n}\n\n"); /* { */
	 else
	    fprintf(outfile, ", \"cc\", \"memory\");\n   return res;\n}\n\n");
      
      }
      else if (target==PPC_POWERUP)
      {
	 for (count = d0; count < numregs; count++)
	 {
	    chtmp = fD_GetProto(obj, count);
	    if (fD_GetFuncParNum(obj) == count)
	       fprintf(outfile, chtmp, fD_GetParam(obj, count));
	    else
	       fprintf(outfile, "%s%s%s", chtmp, (*(chtmp + strlen(chtmp) - 1) == '*' ?
						  "" : " "), fD_GetParam(obj, count));
	    if (count < numregs - 1)
	       fprintf(outfile, ", ");
	 }
	
	 fprintf(outfile, ")\t\n");
	 fprintf(outfile, "{\t\n");
	 fprintf(outfile, "struct Caos\tMyCaos;\n");
	 fprintf(outfile, "\tMyCaos.M68kCacheMode\t=\tIF_CACHEFLUSHALL;\t\n");
	 fprintf(outfile, "//\tMyCaos.M68kStart\t=\tNULL;\t\n");
	 fprintf(outfile, "//\tMyCaos.M68kSize\t\t=\t0;\t\n");
	 fprintf(outfile, "\tMyCaos.PPCCacheMode\t=\tIF_CACHEFLUSHALL;\t\n");
	 fprintf(outfile, "//\tMyCaos.PPCStart\t\t=\tNULL;\t\n");
	 fprintf(outfile, "//\tMyCaos.PPCSize\t\t=\t0;\t\n");

	 if (numregs > 0)
	 {
	    for (count = d0; count < numregs; count++)
            {
               fprintf(outfile, "\tMyCaos.%s\t\t=(ULONG) %s;\t\n",
		       fD_GetRegStr(obj, count),
		       fD_GetParam(obj, count));
            }
         }

         fprintf(outfile, "\tMyCaos.caos_Un.Offset\t=\t(%ld);\t\n", fD_GetOffset(obj));
         if (BaseName[0])                   /*
					     * was "##base" used? 
					     */
         {
	    fprintf(outfile, "\tMyCaos.a6\t\t=\t(ULONG) BASE_NAME;\t\n");
         }
         if (vd)
         {
            fprintf(outfile, "\tPPCCallOS(&MyCaos);\t\n}\n\n");
         }
         else
         {
            fprintf(outfile, "\treturn((%s)PPCCallOS(&MyCaos));\n}\n\n",
		    rettype);
         }
      }
      else if (target==PPC_MORPHOS)
      {
         for (count = d0; count < numregs; count++)
         {
            chtmp = fD_GetProto(obj, count);
            if (fD_GetFuncParNum(obj) == count)
               fprintf(outfile, chtmp, fD_GetParam(obj, count));
            else
               fprintf(outfile, "%s%s%s", chtmp, (*(chtmp + strlen(chtmp) - 1) == '*' ?
						  "" : " "), fD_GetParam(obj, count));
            if (count < numregs - 1)
               fprintf(outfile, ", ");
         }
	 
         fprintf(outfile, ")\t\n{\t\n");

         if (numregs > 0)
         {
            for (count = d0; count < numregs; count++)
            {
               fprintf(outfile, "\tREG_%s\t\t=\t(ULONG) %s;\n",
		       fD_GetRegStrU(obj, count), fD_GetParam(obj, count));
            }
         }
	 
         if (BaseName[0])     /*
			       * was "##base" used?
			       */
         {
            fprintf(outfile, "\tREG_A6\t\t=\t(ULONG) BASE_NAME;\n");
         }
         if (vd)
         {
            fprintf(outfile, "\t(*MyEmulHandle->EmulCallDirectOS)(%ld);\n}\n\n", fD_GetOffset(obj));
         }
         else
         {
            fprintf(outfile, "\treturn((%s)(*MyEmulHandle->EmulCallDirectOS)(%ld));\n}\n\n",
		    rettype, fD_GetOffset(obj));
         }
      }
      else if (target==IX86BE_AMITHLON)
      {
#if 0
         for (count = d0; count < numregs; count++)
         {
            chtmp = fD_GetProto(obj, count);
            if (fD_GetFuncParNum(obj) == count)
               fprintf(outfile, chtmp, fD_GetParam(obj, count));
            else
               fprintf(outfile, "%s%s%s", chtmp, (*(chtmp + strlen(chtmp) - 1) == '*' ?
						  "" : " "), fD_GetParam(obj, count));
            if (count < numregs - 1)
               fprintf(outfile, ", ");
         }

         fprintf(outfile, ")\n{\n");
	 fprintf(outfile, "\tstruct _Regs _regs;\n");

	 if (numregs > 0)
	 {
	    for (count = d0; count < numregs; count++)
            {
               fprintf(outfile, "\t_regs.reg_%s    = (ULONG) (%s);\n",
		       fD_GetRegStr(obj, count),
		       fD_GetParam(obj, count));
            }
         }

         if (BaseName[0])
         {
	   fprintf(outfile, "\t_regs.reg_a6    = (ULONG) (BASE_NAME);\n");
         }
	 
         if (vd)
         {
	    fprintf(outfile, "\t_CallOS68k(%ld,&_regs);\n}\n\n",
		    -fD_GetOffset(obj));
         }
         else
         {
	    fprintf(outfile, "\treturn (%s) _CallOS68k(%ld,&_regs);\n}\n\n",
		    rettype,-fD_GetOffset(obj));
         }
#else
         for (count = d0; count < numregs; count++)
         {
            chtmp = fD_GetProto(obj, count);
            if (fD_GetFuncParNum(obj) == count)
               fprintf(outfile, chtmp, fD_GetParam(obj, count));
            else
               fprintf(outfile, "%s%s%s", chtmp, (*(chtmp + strlen(chtmp) - 1) == '*' ?
						  "" : " "), fD_GetParam(obj, count));
            if (count < numregs - 1)
               fprintf(outfile, ", ");
         }

         fprintf(outfile, ")\n{\n");
	 fprintf(outfile, "\t%s LP%d%s%s(0x%lx, ",
		 (vd ? "" : "return "),
		 numregs,
		 (vd ? "NR" : ""),
		 (BaseName[0] ? "" : "UB"),
		 -fD_GetOffset(obj));
	 
	 if (!vd)
	    fprintf(outfile, "%s, ", rettype);
	 fprintf(outfile, "%s, ", name);

	 for (count=d0; count<numregs; count++)
	 {
	    chtmp=fD_GetRegStr(obj, count);

	    if (strchr(fD_GetProto(obj, count),'%'))
	    {
	       sprintf(Buffer,
		       fD_GetProto(obj, count),
		       "");

	       fprintf(outfile, "%s, %s, %s%s",
		       Buffer,
		       fD_GetParam(obj, count),
		       chtmp,
		       (count == numregs - 1 && !BaseName[0] ? "" : ", "));
	    }
	    else
	    {
	       fprintf(outfile, "%s, %s, %s%s",
		       fD_GetProto(obj, count),
		       fD_GetParam(obj, count),
		       chtmp,
		       (count == numregs - 1 && !BaseName[0] ? "" : ", "));
	    }
	 }
	 
	 if (BaseName[0])
	    fprintf(outfile, "\\\n\t, BASE_NAME");

	 fprintf(outfile, ");\n}\n\n");
#endif	 
      }
      else
      {
	 fprintf(stderr, "Internal error: Unknown target in fD_write().\n");
	 return;
      }
   }
   else if (output_mode==GATESTUBS || output_mode==GATEPROTO)
   {
      int has_base = (BaseName[0] && fD_GetOffset(obj) != 0);
      
      //lcs
      if (target==AROS)
      {
	 for (count=d0; count<numregs; count++)
	 {
	    if (fD_GetFuncParNum(obj) == count)
	    {
	       char funcproto[200]; /* Hope will be enough... */
	       sprintf(funcproto, "%s_%s_funcproto_%d",
		       BaseNamC, name, count );
	       fprintf(outfile, "typedef ");
	       fprintf(outfile, fD_GetProto(obj, count), funcproto);
	       fprintf(outfile, ";\n");
	    }
	 }
      }

      if (output_mode==GATESTUBS)
      {	 
	 fprintf(outfile, "%s %s%s(",
		 rettype,
		 libprefix,
		 name);
     
	 for (count=d0; count<numregs; count++)
	 {
	    chtmp = fD_GetProto(obj, count);

	    fprintf(outfile, chtmp, "");
	    fprintf(outfile, "%s",
		    (count == numregs - 1 && !has_base ? ");\n" : ", "));
	 }

	 if (has_base)
	    fprintf(outfile, "struct %s *);\n", StdLib);
      }
      
      if (target==M68K_AMIGAOS)
      {
	 fprintf(outfile, "%s %s%s(\n",
		 rettype,
		 gateprefix,
		 name);
	 
	 for (count=d0; count<numregs; count++)
	 {
	    chtmp = fD_GetProto(obj, count);

	    if (fD_GetFuncParNum(obj) == count)
	    {
	       fprintf(outfile, "\t");
	       fprintf(outfile, chtmp,
		       fD_GetParam(obj, count));
	       fprintf(outfile, " __asm(\"%s\")%s",
		       fD_GetRegStr(obj, count),
		       (count == numregs - 1 && !has_base ? ")\n" : ",\n"));
	    }
	    else
	    {
	       fprintf(outfile, "\t%s %s __asm(\"%s\")%s",
		       chtmp,
		       fD_GetParam(obj, count),
		       fD_GetRegStr(obj, count),
		       (count == numregs - 1 && !has_base ? ")\n" : ",\n"));
	    }
	 }

	 if (has_base)
	    fprintf(outfile, "\tstruct %s * BASE_NAME __asm(\"a6\") )\n", StdLib);

	 if (output_mode==GATESTUBS)
	    fprintf(outfile, "{\n");
      }
      else if (target==AROS)
      {
	 fprintf(outfile, "AROS_LH%d%s(%s, %s%s,\n",
		 numregs,
		 has_base ? "" : "I",
		 rettype,
		 gateprefix,
		 name);

	 for (count=d0; count<numregs; count++)
	 {
	    char funcproto[200]; /* Hope will be enough... */

	    if (fD_GetFuncParNum(obj) == count)
	    {
	       sprintf(funcproto, "%s_%s_funcproto_%d",
		       BaseNamC, name, count );
	    }
	    
	    fprintf(outfile, "\tAROS_LHA(%s, %s, %s),\n",
		    fD_GetFuncParNum(obj) == count ? funcproto : fD_GetProto(obj, count),
		    fD_GetParam(obj, count),
		    fD_GetRegStrU(obj, count));
	 }

	 fprintf(outfile, "\tstruct %s *, BASE_NAME, %ld, %s)\n",
		 StdLib,
		 -fD_GetOffset(obj) / 6,
		 BaseNamC);

	 if (output_mode==GATESTUBS)
	    fprintf(outfile, "{\n");
      }
      else if (target==PPC_MORPHOS)
      {
 	 fprintf(outfile, "%s %s%s(void)\n",
		 rettype,
		 gateprefix,
		 name);

	 if (output_mode==GATESTUBS)
	 {
	    fprintf(outfile, "{\n");

	    for (count=d0; count<numregs; count++)
	    {
	       chtmp = fD_GetProto(obj, count);

	       if (fD_GetFuncParNum(obj) == count)
	       {
		  fprintf(outfile, "\t");
		  fprintf(outfile, chtmp,
			  fD_GetParam(obj, count));
		  fprintf(outfile, " = (");
		  fprintf(outfile, chtmp, "");
		  fprintf(outfile, ") REG_%s;\n",
			  fD_GetRegStrU(obj, count));
	       }
	       else
	       {
		  fprintf(outfile, "\t%s %s = (%s) REG_%s;\n",
			  fD_GetProto(obj, count),
			  fD_GetParam(obj, count),
			  fD_GetProto(obj, count),
			  fD_GetRegStrU(obj, count));
	       }
	    }

	    if (has_base)
	       fprintf(outfile,
		       "\tstruct %s * BASE_NAME = (struct %s *) REG_A6;\n",
		       StdLib, StdLib);

	    fprintf(outfile, "\n");
	 }
      }
      else if (target==IX86BE_AMITHLON)
      {
	 fprintf(outfile, "%s %s%s( struct _Regs _regs )\n",
		 rettype,
		 gateprefix,
		 name);

	 if (output_mode==GATESTUBS)
	 {
	    fprintf(outfile, "{\n");

	    for (count=d0; count<numregs; count++)
	    {
	       chtmp = fD_GetProto(obj, count);

	       if (fD_GetFuncParNum(obj) == count)
	       {
		  fprintf(outfile, "\t");
		  fprintf(outfile, chtmp,
			  fD_GetParam(obj, count));
		  fprintf(outfile, " = (");
		  fprintf(outfile, chtmp, "");
		  fprintf(outfile, ") _regs.%s;\n",
			  fD_GetRegStr(obj, count));
	       }
	       else
	       {
		  fprintf(outfile, "\t%s %s = (%s) _regs.%s;\n",
			  fD_GetProto(obj, count),
			  fD_GetParam(obj, count),
			  fD_GetProto(obj, count),
			  fD_GetRegStr(obj, count));
	       }
	    }

	    if (has_base)
	       fprintf(outfile,
		       "\tstruct %s * BASE_NAME = (struct %s *) _regs.a6;\n",
		       StdLib, StdLib);

	    fprintf(outfile, "\n");
	 }
      }
      else
      {
	 fprintf(stderr, "Internal error: Unknown target in fD_write().\n");
	 return;
      }

      if (output_mode==GATESTUBS)
      {
	 fprintf(outfile,"\treturn %s%s(",
		 libprefix,
		 name);	

	 for (count=d0; count<numregs; count++)
	 {
	    fprintf(outfile, "%s%s",
		    fD_GetParam(obj, count),
		    (count == numregs - 1 && !has_base ? ");" : ", "));
	 }

	 if (has_base)
	    fprintf(outfile, "BASE_NAME);");

	 fprintf(outfile,"\n}\n\n");
      }
      else
      {
	 fprintf(outfile,";\n");

	 if (target==AROS)
	 {
	    fprintf(outfile, "#define %s%s AROS_SLIB_ENTRY(%s%s,%s,%ld)\n",
		    gateprefix, name,
		    gateprefix, name,
		    BaseNamC,fD_GetOffset(obj));
	 }

	 fprintf(outfile,"\n");
      }
   }
   else
   {
      fprintf(stderr, "Internal error: Unknown output mode in fD_write().\n");
      return;
   }

   if ((tagname=aliasfunction(fD_GetName(obj)))!=0 &&
       output_mode!=GATESTUBS && output_mode!=GATEPROTO)
   {
      fprintf(outfile, "#define %s(", tagname);
      for (count=d0; count<numregs-1; count++)
	 fprintf(outfile, "a%d, ", count);
      fprintf(outfile, "a%d) %s (", count, name);
      for (count=d0; count<numregs-1; count++)
	 fprintf(outfile, "(a%d), ", count);
      fprintf(outfile, "(a%d))\n\n", count);
   }

   if ((tagname=taggedfunction(obj))!=0 &&
       output_mode!=GATESTUBS && output_mode!=GATEPROTO)
   {
      if (output_mode!=STUBS)
      {
	 fprintf( outfile,
		  "#ifndef %sNO_INLINE_STDARG\n"
		  "#define %s(",
		  (target==M68K_POS ? "__" : ""),
		  tagname);

	 for (count=d0; count<numregs-1; count++)
	    fprintf(outfile, "a%d, ", count);

	 fprintf(outfile, "...) \\\n\t({ULONG _tags[] = { __VA_ARGS__ }; %s(",
	    name);

	 for (count=d0; count<numregs-1; count++)
	    fprintf(outfile, "(a%d), ", count);

	 fprintf(outfile, "(%s)_tags);})\n#endif /* !%sNO_INLINE_STDARG */\n\n",
		 fD_GetProto(obj, fD_RegNum(obj)-1),
		 (target==M68K_POS ? "__" : ""));

      }
      else
      {
	 if (target==M68K_AMIGAOS || target==IX86BE_AMITHLON)
	 {
	    fprintf(outfile, "__inline %s\n%s(", rettype, tagname);

	    for (count=d0; count<numregs-1; count++)
	    {
	       chtmp=fD_GetProto(obj, count);
	       if (count==fD_GetFuncParNum(obj))
		  fprintf(outfile, chtmp, fD_GetParam(obj, count));
	       else
		  fprintf(outfile, "%s%s%s", chtmp,
			  (*(chtmp+strlen(chtmp)-1)=='*' ? "" : " "),
			  fD_GetParam(obj, count));
	       fprintf(outfile, ", ");
	    }

	    fprintf(outfile, "int tag, ...)\n{\n	");
	    if (!vd)
	       fprintf(outfile, "return ");

	    fprintf(outfile, "%s(", name);
	    for (count=d0; count<numregs-1; count++)
	       fprintf(outfile, "%s, ", fD_GetParam(obj, count));

	    fprintf(outfile, "(%s)&tag);\n}\n\n", fD_GetProto(obj, fD_RegNum(obj)-1));
	 }
	 else if (target==PPC_MORPHOS)
	 {
            int n = 9 - numregs; /* number of regs that contain varargs */
            int d = n & 1 ? 4 : 0; /* add 4 bytes if that's an odd number, to avoid splitting a tag */
            int taglist = 8; /* offset of the start of the taglist */
            int local = (taglist + n * 4 + d + 8 + 15) & ~15;   /* size of the stack frame */

           /*
            *  Stack frame:
            *
            *   0 -  3: next frame ptr
            *   4 -  7: save lr
            *   8 -  8+n*4+d+8-1: tag list start
            *   ? - local-1: padding
            */

            fprintf(outfile,
		    "asm(\"\n"
		    " .align  2         \n"
		    " .globl  %s        \n"
		    " .type   %s,@function\n"
		    "%s:                \n"
		    " stwu    1,-%d(1)  \n" /* create stack frame */
		    " mflr    0         \n"
		    " stw     0,%d(1)   \n",
		    tagname, tagname, tagname, local, local + 4);

	    /*
	     * If n is odd, one tag is split between regs and stack.
	     * Copy its ti_Data together with the ti_Tag.
	     */
            if (d)
               fprintf(outfile, " lwz 0,%d(1)\n", local + 8); /* read ti_Data */
	    /*
	     * Save the registers 
	     */
            for (count = numregs; count <= 8; count++)
               fprintf(outfile, " stw %d,%d(1)\n", count + 2, (count - numregs) * 4 + taglist);
	    
            if (d)
               fprintf(outfile, " stw 0,%d(1)\n", taglist + n * 4); /* write ti_Data */

	    /*
	     * Add TAG_MORE
	     */
            fprintf(outfile, " li   11,2      \n"
		    " addi 0,1,%d    \n"
		    " stw  11,%d(1)  \n"  /* add TAG_MORE */
		    " stw  0,%d(1)   \n", /* ti_Data = &stack_params */
		    local + 8 + d,
		    taglist + n * 4 + d,
		    taglist + n * 4 + d + 4);


            if (DirectVarargsCalls)
            {
               fprintf(outfile,
		       " addi %d,1,%d \n" /* vararg_reg = &saved regs */
		       " bl   %s      \n",
		       numregs + 2, taglist, name);
            }
            else
            {
               /*
                * Save the non-varargs registers in the EmulHandle struct.
                */
               for (count = 0; count < numregs - 1; count++)
               {
                  int r = fD_GetReg(obj, count);

                  fprintf(outfile, " stw %d,%d(2)\n", count + 3, r * 4);
               }

               fprintf(outfile,
		       " lis  12,%s@ha  \n"
		       " addi 0,1,%d    \n"
		       " lwz  11,0x64(2)\n" /* r11 = EmulCallDirectOS */
		       " stw  0,%d(2)   \n" /* REG_?? = taglist */
		       " mtctr 11       \n"
		       " lwz  12,%s@l(12)\n"
		       " li   3,%ld     \n" /* r3 = lvo */
		       " stw  12,56(2)  \n" /* REG_A6 = libbase */
		       " bctrl          \n",/* EmulCallOS() */
		       BaseName, taglist, 4 * fD_GetReg(obj, numregs - 1), BaseName,
		       fD_GetOffset(obj));
            }

            fprintf(outfile," lwz  0,%d(1)   \n" /* clear stack frame & return */
		    " mtlr 0         \n"
		    " addi 1,1,%d    \n"
		    " blr            \n"
		    ".L%se1:         \n"
		    " .size\t%s,.L%se1-%s\n"
		    "\");\n\n",
		    local + 4, local,
		    tagname, tagname, tagname, tagname);
	 }
	 else
	 {
	    fprintf(stderr, "Internal error: Unknown target in fD_write().\n");
	    return;
	 }
      }
   }
   else if ((varname = getvarargsfunction(obj)) != 0 &&
	    output_mode!=GATESTUBS && output_mode!=GATEPROTO)
   {
      if (output_mode != STUBS)
      {
	 fprintf(outfile,
		 "#ifndef NO_INLINE_VARARGS\n"
		 "#define %s(", varname);

	 for (count = d0; count < numregs - 1; count++)
	    fprintf(outfile, "a%d, ", count);

	 fprintf(outfile,
		 "...) \\\n"
		 "\t({ULONG _tags[] = { __VA_ARGS__ }; %s(",
		 name);

	 for (count = d0; count < numregs - 1; count++)
	    fprintf(outfile, "(a%d), ", count);

	 fprintf(outfile,
		 "(%s)_tags);})\n"
		 "#endif /* !NO_INLINE_VARARGS */\n\n",
		 fD_GetProto(obj, fD_RegNum(obj) - 1));
      }
      else
      {
	 fprintf(stderr, "can`t create a varargs stub function for %s\n",
		 varname);
      }
   }

   if (strcmp(name, "DoPkt")==0 &&
       output_mode!=GATESTUBS && output_mode!=GATEPROTO)
   {
      fdDef *objnc=(fdDef*)obj;
      char newname[7]="DoPkt0";
      objnc->name=newname;
      for (count=2; count<7; count++)
      {
	 regs reg=objnc->reg[count];
	 char *proto=objnc->proto[count];
	 objnc->reg[count]=illegal;
	 objnc->proto[count]=fD_nostring;
	 fD_write(outfile, objnc);
	 objnc->reg[count]=reg;
	 objnc->proto[count]=proto;
	 newname[5]++;
      }
      objnc->name=(char*)name;
   }
}

int
varargsfunction(const char* proto, const char* funcname)
{
   const char *end=proto+strlen(proto)-1;
   while (isspace(*end))
      end--;
   if (*end--==';')
   {
      while (isspace(*end))
	 end--;
      if (*end--==')')
      {
	 while (isspace(*end))
	    end--;
	 if (!strncmp(end-2, "...", 3))
	 {
	    /* Seems to be a varargs function. Check if it will be recognized
	       as "tagged". */
	    unsigned int count;
	    char fixedname[200]; /* Hope will be enough... */
	    fdDef *tmpdef;

	    for (count=0; count<sizeof TagExcTable/sizeof TagExcTable[0];
	    count+=2)
	       if (strcmp(funcname, TagExcTable[count+1])==0)
		  return 1;

	    if (!(tmpdef=fD_ctor()))
	    {
	       fprintf(stderr, "No mem for FDs\n");
	       exit(EXIT_FAILURE);
	    }

	    strcpy(fixedname, funcname);
	    if (strlen(funcname)>4 &&
	    !strcmp(funcname+strlen(funcname)-4, "Tags"))
	    {
	       /* Might be either nothing or "TagList". */
	       fixedname[strlen(fixedname)-4]='\0';
	       fD_NewName(tmpdef, fixedname);
	       if (bsearch(&tmpdef, arrdefs, fds, sizeof arrdefs[0],
	       fD_cmpName))
		  return 1;

	       strcat(fixedname, "TagList");
	       fD_NewName(tmpdef, fixedname);
	       if (bsearch(&tmpdef, arrdefs, fds, sizeof arrdefs[0],
	       fD_cmpName))
		  return 1;
	    }
	    else
	    {
	       strcat(fixedname, "A");
	       fD_NewName(tmpdef, fixedname);
	       if (bsearch(&tmpdef, arrdefs, fds, sizeof arrdefs[0],
	       fD_cmpName))
		  return 1;
	    }
	 }
      }
   }
   return 0;
}

int
ishandleddifferently(const char* proto, const char* funcname)
{
   /* First check if this is a vararg call? */
   if (varargsfunction(proto, funcname))
      return 1;

   /* It might be a dos.library "alias" name. */
   if (aliasfunction(funcname))
      return 1;

   /* It might be one from dos.library/DoPkt() family. */
   if (strlen(funcname)==6 && !strncmp(funcname, "DoPkt", 5) &&
   funcname[5]>='0' && funcname[6]<='4')
      return 1;

   /* Finally, it can be intuition.library/ReportMouse1(). */
   return !strcmp(funcname, "ReportMouse1");
}

void
printusage(const char* exename)
{
   fprintf(stderr,
      "Usage: %s [options] fd-file clib-file [[-o] output-file]\n"
      "Options:\n"

      "--mode=MODE\t\tMODE is one of the following:\n"
      "\tnew\t\t\tPreprocessor based (default)\n"
      "\told\t\t\tInline based\n"
      "\tstubs\t\t\tLibrary stubs\n"
      "\tgatestubs\t\tLibrary gate stubs\n"
      "\tgateproto\t\tLibrary gate prototypes\n"
      "\tproto\t\t\tBuild proto files (no clib-file required)\n"

      "--target=OS\t\tOS is one of the following: \n"
      "\t*-aros\t\t\tAROS (any CPU)\n"
      "\ti?86be*-amithlon\tAmithlon (Intel x86)\n"
      "\tm68k*-amigaos\t\tAmigaOS (Motorola 68000)\n"
      "\tm68k*-pos\t\tPOS (Motorola 68000)\n"
      "\tpowerpc*-powerup\tPowerUp (PowerPC)\n"
      "\tpowerpc*-morphos\tMorphOS (PowerPC)\n"

      "--direct-varargs-calls\tUse direct varargs call for MorphOS stubs\n"
      "--gateprefix=PREFIX\tLibrary gate function name prefix\n"
      "--libprefix=PREFIX\tLocal function name prefix\n"
      "--local\t\t\tUse local includes\n"
      "--quiet\t\t\tDon't display warnings\n"
      "--version\t\tPrint version number and exit\n\n"
      "Compatibility options:\n"
      "--new\t\t\tSame as --mode=new\n"
      "--old\t\t\tSame as --mode=old\n"
      "--stubs\t\t\tSame as --mode=stubs\n"
      "--gatestubs\t\tSame as --mode=gatestubs\n"
      "--proto\t\t\tSame as --mode=prot\n"
      "--pos\t\t\tSame as --target=m68k-pos\n"
      "--morphos\t\tSame as --target=powerpc-morphos\n"
      "--powerup\t\tSame as --target=powerpc-powerup\n"
	   , exename);
}

void output_proto(FILE* outfile)
{
   fprintf(outfile,
      "/* Automatically generated header! Do not edit! */\n\n"
      "#ifndef PROTO_%s_H\n"
      "#define PROTO_%s_H\n\n",
      BaseNamU, BaseNamU);
      
   if (BaseName[0])
       fprintf(outfile,
	 "#ifndef __NOLIBBASE__\n"
	 "extern struct %s *\n"
	 "#ifdef __CONSTLIBBASEDECL__\n"
	 "__CONSTLIBBASEDECL__\n"
	 "#endif /* __CONSTLIBBASEDECL__ */\n"
	 "%s;\n"
	 "#endif /* !__NOLIBBASE__ */\n\n",
	 StdLib, BaseName);
      
   fprintf(outfile,
      "#ifdef __amigaos4__\n"
      "#include <interfaces/%s.h>\n"
      "#ifdef __USE_INLINE__\n"
      "#include <inline4/%s.h>\n"
      "#endif /* __USE_INLINE__ */\n"
      "#ifndef CLIB_%s_PROTOS_H\n"
      "#define CLIB_%s_PROTOS_H\n"
      "#endif /* CLIB_%s_PROTOS_H */\n"
      "#ifndef __NOGLOBALIFACE__\n"
      "extern struct %sIFace *I%s;\n"
      "#endif /* __NOGLOBALIFACE__ */\n"
      "#else /* __amigaos4__ */\n"
      "#include <clib/%s_protos.h>\n"
      "#ifdef __GNUC__\n"
      "#ifdef __AROS__\n"
      "#ifndef NOLIBDEFINES\n"
      "#ifndef %s_NOLIBDEFINES\n"
      "#include <defines/%s.h>\n"
      "#endif /* %s_NOLIBDEFINES */\n"
      "#endif /* NOLIBDEFINES */\n"
      "#else\n"
      "#ifdef __PPC__\n"
      "#ifndef _NO_PPCINLINE\n"
      "#include <ppcinline/%s.h>\n"
      "#endif /* _NO_PPCINLINE */\n"
      "#else\n"
      "#ifndef _NO_INLINE\n"
      "#include <inline/%s.h>\n"
      "#endif /* _NO_INLINE */\n"
      "#endif /* __PPC__ */\n"
      "#endif /* __AROS__ */\n"
      "#else\n"
      "#include <pragmas/%s_pragmas.h>\n"
      "#endif /* __GNUC__ */\n"
      "#endif /* __amigaos4__ */\n\n"
      "#endif /* !PROTO_%s_H */\n",
      BaseNamL, BaseNamL, BaseNamU, BaseNamU, BaseNamU, BaseNamC, BaseNamC,
      BaseNamL,
      BaseNamU, BaseNamL, BaseNamU,
      BaseNamL, BaseNamL, BaseNamL,
      BaseNamU);
}

/******************************************************************************/

int
main(int argc, char** argv)
{
   fdDef *tmpdef,	/* a dummy to contain the name to look for */
	 *founddef;	/* the fdDef for which we found a prototype */
   fdFile *myfile;
   char *tmpstr;
   FILE *outfile;
   int   closeoutfile=0;
   char *fdfilename=0, *clibfilename=0, *outfilename=0;

   int count;
   Error lerror;

   for (count=1; count<argc; count++)
   {
      char *option=argv[count];
      if (*option=='-')
      {
	 option++;
	 if (strcmp(option, "o")==0)
	 {
	    if (count==argc-1 || outfilename)
	    {
	       printusage(argv[0]);
	       return EXIT_FAILURE;
	    }
	    if (strcmp(argv[++count], "-"))
	       outfilename=argv[count];
	 }
	 else
	 {
	    if (*option=='-') /* Accept GNU-style '--' options */
	       option++;
	    if (strncmp(option, "mode=", 5)==0)
	    {
	      if (strcmp(option+5, "new")==0)
		output_mode=NEW;
	      else if (strcmp(option+5, "old")==0)
		output_mode=OLD;
	      else if (strcmp(option+5, "stubs")==0)
		output_mode=STUBS;
	      else if (strcmp(option+5, "gatestubs")==0)
		output_mode=GATESTUBS;
	      else if (strcmp(option+5, "gateproto")==0)
		output_mode=GATEPROTO;
	      else if (strcmp(option+5, "proto")==0)
		output_mode=PROTO;
	    }
	    else if (strncmp(option, "target=", 7)==0)
	    {
	       if (MatchGlob("*-aros",option+7))
		  target=AROS;
	       else if (MatchGlob("i?86be*-amithlon",option+7))
		  target=IX86BE_AMITHLON;
	       else if (MatchGlob("m68k*-amigaos",option+7))
		  target=M68K_AMIGAOS;
	       else if (MatchGlob("m68k*-pos",option+7))
		  target=M68K_POS;
	       else if (MatchGlob("powerpc*-powerup",option+7))
		  target=PPC_POWERUP;
	       else if (MatchGlob("powerpc*-morphos",option+7))
		  target=PPC_MORPHOS;
	       else
	       {
		  printusage(argv[0]);
		  return EXIT_FAILURE;
	       }
	    }
            else if (strcmp(option, "direct-varargs-calls") == 0)
	       DirectVarargsCalls = 1;
	    else if (strncmp(option, "gateprefix=", 11)==0)
	        gateprefix = option+11;
	    else if (strncmp(option, "libprefix=", 10)==0)
	        libprefix = option+10;
            else if (strcmp(option, "quiet") == 0)
	       Quiet = 1;
	    else if (strcmp(option, "version")==0)
	    {
	       fprintf(stderr, "fd2inline version " VERSION "\n");
	       return EXIT_SUCCESS;
	    }
	    /* Compatibility options */
	    else if (strcmp(option, "new")==0)
	       output_mode=NEW;
	    else if (strcmp(option, "old")==0)
	       output_mode=OLD;
	    else if (strcmp(option, "stubs")==0)
	       output_mode=STUBS;
	    else if (strcmp(option, "gatestubs")==0)
	       output_mode=GATESTUBS;
	    else if (strcmp(option, "proto")==0)
	       output_mode=PROTO;
	    else if (strcmp(option, "pos")==0)
	       target=M68K_POS;
	    else if (strcmp(option, "powerup")==0)
	       target=PPC_POWERUP;
	    else if (strcmp(option, "morphos")==0)
	       target=PPC_MORPHOS;
	    /* Unknown option */
	    else
	    {
	       printusage(argv[0]);
	       return EXIT_FAILURE;
	    }
	 }
      }
      else
      {
	 /* One of the filenames */
	 if (!fdfilename)
	    fdfilename=option;
	 else if (!clibfilename)
	    clibfilename=option;
	 else if (!outfilename)
	    outfilename=option;
	 else
	 {
	    printusage(argv[0]);
	    return EXIT_FAILURE;
	 }
      }
   }

   if (!fdfilename || (!clibfilename && output_mode!=PROTO))
   {
      printusage(argv[0]);
      return EXIT_FAILURE;
   }

   if (target==M68K_POS && output_mode!=NEW)
   {
      fprintf(stderr, "Target is not compatible with the mode.\n");
      return EXIT_FAILURE;
   }
   
   if (!(arrdefs=malloc(FDS*sizeof(fdDef*))))
   {
      fprintf(stderr, "No mem for FDs\n");
      return EXIT_FAILURE;
   }
   for (count=0; count<FDS; count++)
      arrdefs[count]=NULL;

   if (!(myfile=fF_ctor(fdfilename)))
   {
      fprintf(stderr, "Couldn't open file '%s'.\n", fdfilename);
      return EXIT_FAILURE;
   }

   lerror=false;

   for (count=0; count<FDS && lerror==false; count++)
   {
      if (!(arrdefs[count]=fD_ctor()))
      {
	 fprintf(stderr, "No mem for FDs\n" );
	 return EXIT_FAILURE;
      }
      do
      {
	 if ((lerror=fF_readln(myfile))==false)
	 {
	    fF_SetError(myfile, false);
	    lerror=fD_parsefd(arrdefs[count], myfile);
	 }
      }
      while (lerror==nodef);
   }
   if (count<FDS)
   {
      count--;
      fD_dtor(arrdefs[count]);
      arrdefs[count]=NULL;
   }
   fds=count;

   qsort(arrdefs, count, sizeof arrdefs[0], fD_cmpName);

   if (output_mode!=NEW || target==AROS)
   {
      unsigned int count2;
      StdLib="Library";

      for (count2=0; count2<sizeof LibExcTable/sizeof LibExcTable[0]; count2+=2)
	 if (strcmp(BaseName, LibExcTable[count2])==0)
	 {
	    StdLib=LibExcTable[count2+1];
	    break;
	 }
   }

   fF_dtor(myfile);

   if (output_mode!=PROTO)
   {
      if (!(myfile=fF_ctor(clibfilename)))
      {
	 fprintf(stderr, "Couldn't open file '%s'.\n", clibfilename);
	 return EXIT_FAILURE;
      }

      if (!(tmpdef=fD_ctor()))
      {
	 fprintf(stderr, "No mem for FDs\n");
	 return EXIT_FAILURE;
      }

      for (lerror=false; lerror==false || lerror==nodef;)
	 if ((lerror=fF_readln(myfile))==false)
	 {
	    fF_SetError(myfile, false); /* continue even on errors */
	    tmpstr=fF_FuncName(myfile);

	    if (tmpstr)
	    {
	       fdDef **res;
	       fD_NewName(tmpdef, tmpstr);
	       res=(fdDef**)bsearch(&tmpdef, arrdefs, fds, sizeof arrdefs[0],
		  fD_cmpName);

	       if (res)
	       {
		  founddef=*res;
		  DBP(fprintf(stderr, "found (%s).\n", fD_GetName(founddef)));
		  fF_SetError(myfile, false);
		  lerror=fD_parsepr(founddef, myfile);
	       }
	       else
		  if (!ishandleddifferently(myfile->line, tmpstr))
		     if (!Quiet)
		        fprintf(stderr, "Don't know what to do with <%s> in line %lu.\n",
				tmpstr, myfile->lineno);
	       free(tmpstr);
	    }
	 }

      fD_dtor(tmpdef);

      fF_dtor(myfile);
   }

   if (strlen(fdfilename)>7 &&
   !strcmp(fdfilename+strlen(fdfilename)-7, "_lib.fd"))
   {
      char *str=fdfilename+strlen(fdfilename)-8;
      while (str!=fdfilename && str[-1]!='/' && str[-1]!=':')
	 str--;
//lcs      strncpy(BaseNamL, str, strlen(str)-7);
      strncpy(BaseNamU, str, strlen(str)-7);
      BaseNamU[strlen(str)-7]='\0';
      strcpy(BaseNamL, BaseNamU);
      strcpy(BaseNamC, BaseNamU);
   }
   else
   {
      strcpy(BaseNamU, BaseName);
      if (strlen(BaseNamU)>4 && strcmp(BaseNamU+strlen(BaseNamU)-4, "Base")==0)
	 BaseNamU[strlen(BaseNamU)-4]='\0';
      if (target==M68K_POS && strncmp(BaseNamU, "gb_", 3)==0)
	 memmove(BaseNamU, &BaseNamU[3], strlen(&BaseNamU[3])+1);
      strcpy(BaseNamL, BaseNamU);
      strcpy(BaseNamC, BaseNamU);
   }
   StrUpr(BaseNamU);
   BaseNamC[0]=toupper(BaseNamC[0]);

   if (outfilename)
   {
      if (!(outfile=fopen(outfilename, "w")))
      {
	 fprintf(stderr, "Couldn't open output file.\n");
	 return EXIT_FAILURE;
      }
      else
      {
	 closeoutfile=1;
      }
   }
   else
      outfile=stdout;

   if (output_mode==PROTO)
      output_proto(outfile);
   else
   {
      if (output_mode==NEW || output_mode==OLD || output_mode==STUBS ||
	  output_mode==GATESTUBS || output_mode==GATEPROTO)
      {
	 if (output_mode==GATESTUBS || output_mode==GATEPROTO)
	 {
 	    fprintf(outfile,
		    "/* Automatically generated stubs! Do not edit! */\n\n");
	 }
	 else
	 {
	    fprintf(outfile,
		    "/* Automatically generated header! Do not edit! */\n\n"
		    "#ifndef %sINLINE_%s_H\n"
		    "#define %sINLINE_%s_H\n\n",
		    (target==M68K_POS ? "__INC_POS_P" : "_"),
		    BaseNamU,
		    (target==M68K_POS ? "__INC_POS_P" : "_"),
		    BaseNamU );
	 }

	 if (output_mode==NEW)
	 {
	    if(target==M68K_POS)
	    {
	       fprintf(outfile,
		       "#ifndef __INC_POS_PINLINE_MACROS_H\n"
		       "#include <pInline/macros.h>\n"
		       "#endif /* !__INC_POS_PINLINE_MACROS_H */\n\n" );
	    }
	    else if(target==AROS)
	    {
	       fprintf(outfile,
		       "#ifndef AROS_LIBCALL_H\n"
		       "#include <aros/libcall.h>\n"
		       "#endif /* !AROS_LIBCALL_H */\n\n");
	    }
	    else
	    {
	       fprintf(outfile,
		       "#ifndef __INLINE_MACROS_H\n"
		       "#include <inline/macros.h>\n"
		       "#endif /* !__INLINE_MACROS_H */\n\n");
	    }
	 }
	 else
	 {
	    FILE* clib;
	    
	    fprintf(outfile,
		    "#ifndef __INLINE_STUB_H\n"
		    "#include <inline/stubs.h>\n"
		    "#endif /* !__INLINE_STUB_H */\n\n");

	    fprintf(outfile, "#ifdef __CLIB_TYPES__\n" );

	    clib = fopen( clibfilename, "r" );

	    if( clib == NULL )
	    {
	       fprintf(stderr, "Couldn't open file '%s'.\n", clibfilename);
	    }
	    else
	    {
	      char* buffer = malloc( 1024 );

	      if( buffer == NULL )
	      {
		fprintf(stderr, "No memory for line buffer.\n " );
	      }
	      else
	      {
		while( fgets( buffer, 1023, clib ) != NULL )
		{
		  if( buffer[ 0 ] == '#' /* Pre-processor instruction */ ||
		      strncmp( buffer, "typedef", 7 ) == 0 )
		  {
		    fputs(buffer, outfile );
		  }
		}
		
		free( buffer );
	      }

	      fclose( clib );
	    }
	    
	    fprintf(outfile, "#endif /* __CLIB_TYPES__ */\n\n" );
	    
	    if(target==AROS)
	    {
	       fprintf(outfile,
		       "#include <aros/libcall.h>\n\n" );
	    }
	    else if(target==IX86BE_AMITHLON)
	    {
	       fprintf(outfile,
		       "#ifndef __INLINE_MACROS_H\n"
		       "#include <inline/macros.h>\n"
		       "#endif /* __INLINE_MACROS_H */\n\n");
	    }
	    else if (target == PPC_MORPHOS)
	    {
	       fprintf(outfile,
		       "#include <emul/emulregs.h>\n\n" );
	    }
	 }
      }
      else
      {
	 fprintf(stderr, "Internal error: Unknown output mode in main().\n");

	 if (closeoutfile)
	 {
	    fclose(outfile);
	 }
	 
         return EXIT_FAILURE;
      }
      
      if (BaseName[0])
      {
	 if (output_mode==NEW)
	 {
	    fprintf(outfile,
	       "#ifndef %s%s_BASE_NAME\n"
	       "#define %s%s_BASE_NAME %s\n"
	       "#endif /* !%s%s_BASE_NAME */\n\n",
	       (target==M68K_POS ? "__" : ""), BaseNamU,
	       (target==M68K_POS ? "__" : ""), BaseNamU, BaseName,
	       (target==M68K_POS ? "__" : ""), BaseNamU);
	    if (target==M68K_POS)
	       fprintf(outfile,
		  "#ifndef __%s_LIB_NAME\n"
		  "#define __%s_LIB_NAME %s\n"
		  "#endif /* !__%s_LIB_NAME */\n\n",
		  BaseNamU, BaseNamU,
		  (strcmp(BaseName, "gb_ExecBase") ? BaseName : "gb_ExecLib"),
		  BaseNamU);
	 }
	 else
	    fprintf(outfile,
	       "#ifndef BASE_EXT_DECL\n"
	       "#define BASE_EXT_DECL\n"
	       "#define BASE_EXT_DECL0 extern struct %s *%s;\n"
	       "#endif /* !BASE_EXT_DECL */\n"
	       "#ifndef BASE_PAR_DECL\n"
	       "#define BASE_PAR_DECL\n"
	       "#define BASE_PAR_DECL0 void\n"
	       "#endif /* !BASE_PAR_DECL */\n"
	       "#ifndef BASE_NAME\n"
	       "#define BASE_NAME %s\n"
	       "#endif /* !BASE_NAME */\n\n"
	       "BASE_EXT_DECL0\n\n", StdLib, BaseName, BaseName);
      }

      for (count=0; count<FDS && arrdefs[count]; count++)
      {
	 DBP(fprintf(stderr, "outputting %ld...\n", count));

	 fD_write(outfile, arrdefs[count]);
	 fD_dtor(arrdefs[count]);
	 arrdefs[count]=NULL;
      }

      if (output_mode!=NEW)
	 if (BaseName[0])
	    fprintf(outfile,
	       "#undef BASE_EXT_DECL\n"
	       "#undef BASE_EXT_DECL0\n"
	       "#undef BASE_PAR_DECL\n"
	       "#undef BASE_PAR_DECL0\n"
	       "#undef BASE_NAME\n\n");

      if (output_mode==NEW || output_mode==OLD || output_mode==STUBS)
      {
	 fprintf(outfile, "#endif /* !%sINLINE_%s_H */\n",
		 (target==M68K_POS ? "__INC_POS_P" : "_"), BaseNamU);
      }
   }

   free(arrdefs);

   if (closeoutfile)
   {
      fclose(outfile);
   }

   return EXIT_SUCCESS;
}
