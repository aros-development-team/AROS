/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>

struct libconf;

/* Prototypes of reusable functions */

/* Read in one line from file
   - mallocs needed space					*/
char *get_line(FILE *file);

/* Search for '#Keyword' in string
  - mallocs needed space
  - returns 'Keyword'						*/
char *keyword(char *line);

/* Searches line for whitespace separated words
  - outarray = pointer to array of found words (strings)
  - mallocs needed space
  - frees previously allocated space before !
  - for first use pass
      char **words=NULL; get_words(line,&words)			*/
int get_words(char *line, char ***outarray);

/* Converts string to lower case				*/
void strlower(char *string);

/* Converts string to upper case				*/
void strupper(char *string);

/* Compares two files
   returns 0 for equal files,
   1 for different files,
   -1 if file1 is not present,
   -2 if file2 is not present					*/
int filesdiffer( char *file1, char *file2 );

/* Compares old and new file,
   if old file is not found or different from new
   new file will be renamed to name of old
   old will be named old.bak					*/
void moveifchanged(char *old, char *new);

/* Copies file src to dest, returns 0 on success		*/
int copy(char *src, char *dest);

/* Parses file or lib.conf if file==NULL
   puts values in struct libconf
   returns 0 on success						*/
struct libconf * parse_libconf(char *file);


char *get_line(FILE *fd)
{
int count,len;
char *line;
char buffer;

  len = 0;
  do
  {
    count = fread(&buffer,1,1,fd);
    len += count;
  } while(count!=0 && buffer!='\n');
  if(len==0 && count==0)
    return NULL;
  fseek( fd, -len, SEEK_CUR );
  line = malloc( (len+1) * sizeof(char) );
  fread (line,1,len,fd);
  line[len]=0;
  len--;
  while(isspace(line[len])&& len>=0)
  {
    line[len] = 0;
    len--;
  }

return line;
}

char *keyword(char *line)
{
char *key;
int len;

  if(line[0]=='#')
  {
    len = 1;
    while(line[len] && !isspace(line[len]))
      len++;
    key = malloc( len * sizeof(char) );
    strncpy( key, &line[1], len-1 );
    key[len-1] = 0;

    return key;
  }
  else
    return NULL;
}

int get_words(char *line, char ***outarray)
{
char **array;
char *word;
int num,len;

  if(!outarray||!line)
  {
    fprintf( stderr, "Passed invalid NULL pointer to get_words()!\n" );
    exit(-1);
  }
  array = *outarray;
  if( array )
  {
    while( *array )
    {
      free(*array);
      array++;
    }
    free(*outarray);
  }
  array = NULL;
  num = 0;
  word = line;
  while(*word!=0)
  {
    while( *word && isspace(*word) )
      word++;
    len = 0;
    while( word[len] && !isspace(word[len]) )
      len++;
    if(len)
    {
      num++;
      array = realloc( array, (num+1) * sizeof(char *) );
      array[num-1] = malloc( (len+1) * sizeof(char) );
      strncpy( array[num-1], word, len );
      array[num-1][len] = 0;
      word = &word[len];
    }
  }
  if (array)
    array[num] = NULL;

  *outarray = array;

return num;
}

void strlower(char *string)
{
  while(*string)
  {
    *string = tolower(*string);
    string++;
  }
}

void strupper(char *string)
{
  while(*string)
  {
    *string = toupper(*string);
    string++;
  }
}

int filesdiffer( char *file1, char *file2 )
{
FILE *fd1, *fd2;
int cnt1,cnt2;
char buffer1[1], buffer2[1];
int retval = 0;

  fd1 = fopen(file1,"rb");
  if(!fd1)
    return -1;
  fd2 = fopen(file2,"rb");
  if(!fd2)
    return -2;
  do
  {
    cnt1 = fread(buffer1,1,1,fd1);
    cnt2 = fread(buffer2,1,1,fd2);
  } while( cnt1 && cnt2 && buffer1[0]==buffer2[0] );
  if( buffer1[0]!=buffer2[0] || cnt1 != cnt2 )
    retval = 1;

  fclose(fd1);
  fclose(fd2);

return retval;
}

void moveifchanged(char *old, char *new)
{
struct stat *statold, *statnew;
char *bakname;

  statold = calloc(1, sizeof(struct stat) );
  statnew = calloc(1, sizeof(struct stat) );
  if(stat(old,statold))
  {
    /* Couldn't stat old file -- assume non-existent */
    rename(new,old);
    return;
  }
  if(stat(new,statnew))
  {
    /* Couldn't stat new file -- this shouldn't happen */
    fprintf( stderr, "Couldn't stat() file %s!\n", new );
    exit(-1);
  }
  bakname = malloc( (strlen(old)+5) * sizeof(char) );
  sprintf( bakname, "%s.bak", old );
  if(statold->st_size != statnew->st_size)
  {
    rename(old,bakname);
    rename(new,old);
  }
  else if( filesdiffer(old,new) )
  {
    rename(old,bakname);
    rename(new,old);
  }
  else
    remove(new);
  free(bakname);
}

int copy(char *src, char *dest)
{
FILE *in, *out;
int count;
char buffer[1024];

  in = fopen(src,"rb");
  if(!in)
    return -1;
  out = fopen(dest,"w");
  if(!out)
    return -1;
  do
  {
    count = fread(buffer,1,1024,in);
    fwrite(buffer,1,count,out);
  } while( count==1024 );
  fclose(in);
  fclose(out);

return 0;
}

enum libtype
{
  t_device	= 1,
  t_library	= 2,
  t_resource	= 3,
  t_hidd	= 4,
  t_gadget	= 5,
  t_image	= 6,
  t_class	= 7,
  t_datatype	= 8
};

enum liboption
{
  o_noexpunge = 1,
  o_rom = 2,
  o_unique = 4,
  o_nolibheader = 8,
  o_hasrt = 16
};

struct libconf
{
  char *libname;
  char *basename;
  char *libbase;
  char *libbasetype;
  char *libbasetypeptr;
  int version, revision;
  char *copyright;
  char *define;
  int type;
  int option;
};

typedef struct _Node Node;

struct _Node
{
    Node * next,
         * prev;
    char * name;
};

typedef struct
{
    Node * first,
         * last,
         * prelast;
}
List;

/* Macros */
#   define NewList(l)       (((List *)l)->prelast = (Node *)(l), \
                            ((List *)l)->last = 0, \
                            ((List *)l)->first = (Node *)&(((List *)l)->last))

#   define AddHead(l,n)     ((void)(\
        ((Node *)n)->next        = ((List *)l)->first, \
        ((Node *)n)->prev        = (Node *)&((List *)l)->first, \
        ((List *)l)->first->prev = ((Node *)n), \
        ((List *)l)->first       = ((Node *)n)))

#   define AddTail(l,n)     ((void)(\
        ((Node *)n)->next          = (Node *)&((List *)l)->last, \
        ((Node *)n)->prev          = ((List *)l)->prelast, \
        ((List *)l)->prelast->next = ((Node *)n), \
        ((List *)l)->prelast       = ((Node *)n) ))

#   define Remove(n)        ((void)(\
        ((Node *)n)->prev->next = ((Node *)n)->next,\
        ((Node *)n)->next->prev = ((Node *)n)->prev ))

#   define GetHead(l)       (void *)(((List *)l)->first->next \
                                ? ((List *)l)->first \
                                : (Node *)0)
#   define GetTail(l)       (void *)(((List *)l)->prelast->prev \
                                ? ((List *)l)->prelast \
                                : (Node *)0)
#   define GetNext(n)       (void *)(((Node *)n)->next->next \
                                ? ((Node *)n)->next \
                                : (Node *)0)
#   define GetPrev(n)       (void *)(((Node *)n)->prev->prev \
                                ? ((Node *)n)->prev \
                                : (Node *)0)
#   define ForeachNode(l,n) \
        for (n=(void *)(((List *)(l))->first); \
            ((Node *)(n))->next; \
            n=(void *)(((Node *)(n))->next))
#   define ForeachNodeSafe(l,node,nextnode) \
        for (node=(void *)(((List *)(l))->first); \
            ((nextnode)=(void*)((Node *)(node))->next); \
            (node)=(void *)(nextnode))

Node * FindNode (List * list, const char * name)
{
    Node * node;
    
    ForeachNode (list, node)
    {
	if (!strcmp (node->name, name))
	    return node;
    }

    return NULL;
}

int GetListLen (List * list)
{
    int i = 0;
    Node * node;

    ForeachNode (list, node)
	i ++;

    return i;
}

typedef struct _Headerline
{
    Node node;
    int isLocal;
} HeaderLine;
    
typedef struct _Header
{
    List lines;
} Header;

#define FUNCFLAG_INDEPENDENT	    1
#define FUNCFLAG_RETURNSQUAD	    2

typedef struct _Function
{
    Node node;
    char * macro;
    char * type;
    List parameters;
    int libOffset;
    /* AutoDoc sections */
    List sections;
    char * code;
    int flags;
} Function;

typedef struct _Parameter
{
    Node node;
    char * macro;
    char * type;
    List registers;
    char * description;
} Parameter;

char * readText (FILE * fh, char * delim)
{
    char * text = NULL;
    char * line, * ptr;
    int textLen = 0;
    int delimLen = strlen (delim);
    int n;

    while ( (line = get_line (fh)) )
    {
	if (line[0] == '#' && line[1] == '/'
	    && !strncmp (line+2, delim, delimLen))
	{
	    free (line);
	    break;
	}
	
	ptr = line;
	while (isspace (*ptr))
	    ptr ++;

	for (n=strlen (ptr); n>0; n--)
	    if (!isspace (ptr[n-1]))
		break;

	text = realloc (text, textLen+n+2);
	strncpy (text+textLen, ptr, n);
	textLen += n;
	text[textLen++] = '\n';
	text[textLen] = 0;
	free (line);
    }
    
    return text;
}

char * joinStrings (int n, char ** strings, char * sep)
{
    int i, len, sepLen=0, resultLen=0;
    char * result = NULL;

    if (sep)
	sepLen = strlen (sep);
    
    for (i=0; i<n; i++)
    {
	// printf ("'%s' Adding '%s'\n", result, strings[i]);
	len = strlen (strings[i]);
	if (len)
	{
	    result = realloc (result, resultLen+len+sepLen+1);
	    strcpy (result+resultLen, strings[i]);

	    if (sepLen && i+1!=n)
		strcpy (result+resultLen+len, sep);
	
	    resultLen += len+sepLen;
	}
    }

    // printf ("result '%s'\n", result);
    return result;
}

char * joinVAStrings (char * sep, char * str, ...)
{
    va_list args;
    int len, sepLen=0, resultLen=0;
    char * result = NULL, * next;

    va_start (args, str);
    
    if (sep)
	sepLen = strlen (sep);
    
    while (str)
    {
	next = va_arg (args, char *);

	len = strlen (str);
	if (len)
	{
	    result = realloc (result, resultLen+len+sepLen+1);
	    strcpy (result+resultLen, str);

	    if (sepLen && next)
		strcpy (result+resultLen+len, sep);
	
	    resultLen += len+sepLen;
	}

	str = next;
    }

    return result;
}

Parameter * newParameter (FILE * fh, int num, char ** header)
{
    Parameter * par = malloc (sizeof (Parameter));
    char * ptr;

    par->node.name = strdup (header[num-2]);
    par->type = joinStrings (num-3, &header[1], " ");
    // printf ("Type %d %s\n", num-3, par->type);
    NewList (&par->registers);
    par->description = readText (fh, "Parameter");

    for (ptr=header[num-1]; *ptr; )
    {
	Node * node = malloc (sizeof (Node));
	node->name = malloc (3);
	strncpy (node->name, ptr, 2);
	node->name[2] = 0;
	// printf ("Reg %s\n", node->name);
	AddTail (&par->registers, node);

	ptr += 2;
	if (*ptr == '/')
	    ptr ++;
    }

    if (strlen (header[num-1]) == 5)
    //if (num == 5)
	par->macro = "LHAQUAD";
    else
	par->macro = "LHA";
    
    return par;
}

Function * newFunction (FILE * fh, int num, char ** header)
{
    Function * func = malloc (sizeof (Function));
    char * line, ** words = NULL;
    int n;

    func->node.name = strdup (header[num-1]);
    func->type = joinStrings (num-2, &header[1], " ");
    NewList (&func->parameters);
    NewList (&func->sections);
    func->libOffset = -1;
    func->code = NULL;
    func->flags = 0;

    while ( (line = get_line (fh)) )
    {
	n = get_words (line, &words);
	if (!n)
	{
	    free (line);
	    continue;
	}

	if (!strcmp (words[0], "#Parameter"))
	{
	    Parameter * par;

	    par = newParameter (fh, n, words);
	    if (!par)
	    {
		fprintf (stderr, "Reading of %s failed.", func->node.name);
		free (line);
		return NULL;
	    }
	    AddTail (&func->parameters, par);
	}
	else if (!strcmp (words[0], "#Options"))
	{
	    int i;

	    for (i=1; i<n; i++)
	    {
		if (!strcmp (words[i], "I"))
		    func->flags |= FUNCFLAG_INDEPENDENT;
		else if (!strcmp (words[i], "QUAD"))
		    func->flags |= FUNCFLAG_RETURNSQUAD;
		else
		    fprintf (stderr, "%s: Unknown option %s\n",
			func->node.name, words[i]
		    );
	    }
	}
	else if (!strcmp (words[0], "#LibOffset"))
	{
	    func->libOffset = atoi (words[1]);
	}
	else if (!strcmp (words[0], "#AutoDoc"))
	{
	    /* TODO Read autodocs */
	    char * str = readText (fh, "AutoDoc");
	    free (str);
	}
	else if (!strcmp (words[0], "#Code"))
	{
	    func->code = readText (fh, "Code");
	}
	else if (!strcmp (words[0], "#/Function"))
	    break;
	else
	    fprintf (stderr, "Unexpected input: %s\n", line);

	free (line);
    }
    
    return func;
}

typedef struct _Archive
{
    struct libconf * libconf;
    List functions;
    List header;
} Archive;

void addToArchive (Archive * arch, char * filename)
{
    FILE * fh;
    char * line;
    int inArchive, inHeader, isLocal;
    
    fh = fopen (filename, "rb");
    if (!fh)
    {
	fprintf (stderr, "Cannot open %s for reading\n", filename);
	exit (1);
    }

    inArchive = inHeader = 0;
    while ( (line = get_line (fh)) )
    {
	if (!strncmp (line, "#Archive", 8))
	    inArchive = 1;
	else if (!strncmp (line, "#Header", 7))
	{
	    if (!inArchive)
	    {
		fprintf (stderr, "Found #Header outside of #Archive\n");
		exit (1);
	    }
	    isLocal = 0;
	    
	    free (line);
	    while ( (line = get_line (fh)) )
	    {
		if (!strncmp (line, "#/Header", 8))
		    break;
		else if (!strncmp (line, "#Local", 6))
		    isLocal = 1;
		else if (!strncmp (line, "#/Local", 7))
		    isLocal = 0;
		else
		{
		    HeaderLine * hl = malloc (sizeof (HeaderLine));
		    hl->node.name = line;
		    hl->isLocal = isLocal;

		    AddTail (&arch->header, hl);
		    
		    /* Do not free the line */
		    continue;
		}

		free (line);
	    }
	}
	else if (!strncmp (line, "#Function", 7))
	{
	    Function * func, * old;
	    int n;
	    char ** words = NULL;

	    n = get_words (line, &words);
	    func = newFunction (fh, n, words);
	    if (!func)
	    {
		fprintf (stderr, "Error reading function\n");
		exit (1);
	    }

	    /* Overwrite existing functions */
	    old = (Function *)FindNode (&arch->functions, func->node.name);
	    if (old)
	    {
		Remove (old);
		/* TODO free old */
	    }
	    AddTail (&arch->functions, func);
	}
	    
	free (line);
    }

    fclose (fh);
}

Archive * newArchive (int num, char ** filenames)
{
    Archive * arch = malloc (sizeof (Archive));
    int i;

    arch->libconf = parse_libconf (NULL);
    if (!arch->libconf)
	exit (0);

    NewList (&arch->functions);
    NewList (&arch->header);

    for (i=0; i<num; i++)
	addToArchive (arch, filenames[i]);

    return arch;
}

struct libconf * parse_libconf(char *file)
{
FILE *fd;
int num, len, i;
char *line, *word;
char **words = NULL;
struct libconf * lc = calloc (1, sizeof (struct libconf));

  fd = fopen((file?file:"lib.conf"),"rb");
  if(!fd)
  {
    fprintf( stderr, "Couldn't open %s!\n", (file?file:"lib.conf") );
    return NULL;
  }
  while( (line = get_line(fd)) )
  {
    num = get_words(line,&words);
    if( num > 1 )
    {
      if( strcmp(words[0],"name")==0 )
      {
        strlower(words[1]);
        len = strlen(words[1]);
        lc->libname = strdup(words[1]);
      }
      else if( strcmp(words[0],"libname")==0 )
      {
        free(lc->libname);
        lc->libname = strdup(words[1]);
      }
      else if( strcmp(words[0],"basename")==0 )
      {
        len = strlen(words[1]);
        free(lc->basename);
        lc->basename = strdup(words[1]);
      }
      else if( strcmp(words[0],"libbase")==0 )
      {
        len = strlen(words[1]);
        free(lc->libbase);
        lc->libbase = strdup(words[1]);
      }
      else if( strcmp(words[0],"libbasetype")==0 )
      {
        len = 0;
        for( i=1 ; i<num ; i++ )
          len += strlen(words[i]);
        len += num-1;
        free(lc->libbasetype);
        lc->libbasetype = malloc( len * sizeof(char) );
        strcpy(lc->libbasetype, words[1] );
        for( i=2 ; i<num ; i++ )
        {
          strcat( lc->libbasetype, " " );
          strcat( lc->libbasetype, words[i] );
        }
      }
      else if( strcmp(words[0],"libbasetypeptr")==0 )
      {
        len = 0;
        for( i=1 ; i<num ; i++ )
          len += strlen(words[i]);
        len += num-1;
        free(lc->libbasetypeptr);
        lc->libbasetypeptr = malloc( len * sizeof(char) );
        strcpy(lc->libbasetypeptr, words[1]);
        for( i=2 ; i<num ; i++ )
        {
          strcat( lc->libbasetypeptr, " " );
          strcat( lc->libbasetypeptr, words[i] );
        }
      }
      else if( strcmp(words[0],"version")==0 )
      {
        i = 0;
        while( words[1][i] && words[1][i]!='.' )
          i++;
        lc->revision = (words[1][i]==0?0:atoi(&words[1][i+1]));
        if( i==0 )
        {
          lc->version = 0;
        }
        else
        {
          words[1][i] = 0;
          lc->version = atoi(words[1]);
        }
      }
      else if( strcmp(words[0],"copyright")==0 )
      {
        word = &line[9];
        while( *word && isspace(*word) )
          word++;
        free(lc->copyright);
        lc->copyright = strdup(word);
      }
      else if( strcmp(words[0],"define")==0 )
      {
        free(lc->define);
        lc->define = strdup(words[1]);
      }
      else if( strcmp(words[0],"type")==0 )
      {
        if( strcmp(words[1],"device")==0 )
          lc->type = t_device;
        else if( strcmp(words[1],"library")==0 )
          lc->type = t_library;
        else if( strcmp(words[1],"resource")==0 )
          lc->type = t_resource;
        else if( strcmp(words[1],"hidd")==0 )
          lc->type = t_hidd;
        else if( strcmp(words[1],"gadget")==0 )
          lc->type = t_gadget;
        else if( strcmp(words[1],"image")==0 )
          lc->type = t_image;
        else if( strcmp(words[1],"class")==0 )
          lc->type = t_class;
        else if( strcmp(words[1],"datatype")==0 )
          lc->type = t_datatype;
      }
      else if( strcmp(words[0],"options")==0 || strcmp(words[0],"option")==0 )
      {
        for( i=1 ; i<num ; i++ )
        {
          if( strcmp(words[i],"noexpunge")==0 )
            lc->option |= o_noexpunge;
          else if( strcmp(words[i],"rom")==0 )
            lc->option |= o_rom;
          else if( strcmp(words[i],"unique")==0 )
            lc->option |= o_unique;
          else if( strcmp(words[i],"nolibheader")==0 )
            lc->option |= o_nolibheader;
          else if( strcmp(words[i],"hasrt")==0 )
            lc->option |= o_hasrt;
        }
      }
    }
    free(line);
  }

  if( lc->libname == NULL )
  {
    fprintf( stderr, "Missing field \"name\" in lib.conf!\n" );
    return NULL;
  }
  if( lc->basename == NULL )
  {
    lc->basename = strdup(lc->libname);
    lc->basename[0] = toupper(lc->basename[0]);
  }
  if( lc->libbase == NULL )
  {
    lc->libbase = malloc( (strlen(lc->basename)+5) * sizeof(char) );
    sprintf( lc->libbase, "%sBase", lc->basename );
  }
  if( lc->libbasetype == NULL )
  {
    lc->libbasetype = malloc( (strlen(lc->libbase)+8) * sizeof(char) );
    sprintf( lc->libbasetype, "struct %s", lc->libbase );
  }
  if( lc->libbasetypeptr == NULL )
  {
    lc->libbasetypeptr = malloc( (strlen(lc->libbasetype)+3) * sizeof(char) );
    sprintf( lc->libbasetypeptr, "%s *", lc->libbasetype );
  }

  if( lc->define == NULL )
    lc->define = strdup( "_LIBDEFS_H" );
  if( lc->type == 0 )
    lc->type = t_library;

    return lc;
}

/*
#
# Create libdefs.h from a file lib.conf. lib.conf may contain these options:
#
# name <string> - Init the various fields with reasonable defaults. If
#		<string> is XXX, then this is the result:
#
#		    libname	    xxx
#		    basename	    Xxx
#		    libbase	    XxxBase
#		    libbasetype     XxxBase
#		    libbasetypeptr  XxxBase *
#
#		Variables will only be changed if they have not yet been
#		specified.
#
# libname <string> - Set libname to <string>. This is the name of the
#		library (ie. you can open it with <string>.library).
#		It will show up in the version string, too.
# basename <string> - Set basename to <string>. The basename is used in
#		the AROS-LHx macros in the location part (last parameter)
#		and to specify defaults for libbase and libbasetype
#		in case they have no value yet. If <string> is xXx, then
#		libbase will become xXxBase and libbasetype will become
#		xXxBase.
# libbase <string> - Defines the name of the library base (ie. SysBase,
#		DOSBase, IconBase, etc). If libbasetype is not set, then
#		it is set to <string>, too.
# libbasetype <string> - The type of libbase (with struct), ie.
#		struct ExecBase, struct DosLibrary, struct IconBase, etc).
# libbasetypeptr <string> - Type of a pointer to the libbase. (eg.
#		struct ExecBase *).
# version <version>.<revision> - Specifies the version and revision of the
#		library. 41.0103 means version 41 and revision 103.
# copyright <string> - Copyright string.
# define <string> - The define to use to protect the resulting file
#		against double inclusion (ie. #ifndef <string>...)
#		The default is _LIBDEFS_H.
# type <string> - What kind of library is this ? Valid values
#		for <string> are: device, library, resource and hidd.
# option <string>... - Specify an option. Valid values for <string> are:
#
#		    noexpunge - Once the lib/dev is loaded, it can't be
#				removed from memory. Be careful with this
#				option.
#		    rom - For ROM based libraries. Implies noexpunge and
#				unique.
#		    unique - Generate unique names for all external
#				symbols.
#		    nolibheader - We don't want to use the LibHeader prefixed
#				functions in the function table.
#		    hasrt - This library has resource tracking.
#
#		You can specify more than one option in a config file and
#		more than one option per option line. Separate options by
#		space.
#
*/
int genlibdefs(int argc, char **argv)
{
struct libconf *lc;
FILE *fd;
char *date, *filename, *conffile;
struct tm *tm;
time_t t;
int i;
    
  filename = "libdefs.h";
  conffile = "lib.conf";
  for (i=2; i<=argc; i++)
  {
    if (strcmp(argv[i-1],"-o")==0)
    {
      filename = argv[i];
      i++;
    }
    else
      conffile = argv[i-1];
  }
  time(&t);
  tm = localtime(&t);
  date = malloc( 11 * sizeof(char) );
  sprintf( date, "%02d.%02d.%4d", tm->tm_mday, tm->tm_mon+1, tm->tm_year+1900 );
  fd = fopen(filename,"w");
  if(!fd)
  {
    fprintf( stderr, "Couldn't open file %s!\n", "libdefs.h.new" );
    return -1;
  }
  if(!(lc = parse_libconf(conffile)) )
    return(-1);
  if( lc->copyright == NULL )
  {
    lc->copyright = strdup("");
  }
  fprintf( fd, "/* *** Automatic generated file. Do not edit *** */\n\n");
  fprintf( fd, "#ifndef %s\n#define %s\n", lc->define, lc->define );

  if (lc->type == t_library)
  {
    fprintf( fd, "#define NAME_STRING      \"%s.library\"\n", lc->libname );
    fprintf( fd, "#define NT_TYPE          NT_LIBRARY\n" );
  }
  else if (lc->type == t_device)
  {
    fprintf( fd, "#define NAME_STRING      \"%s.device\"\n", lc->libname );
    fprintf( fd, "#define NT_TYPE          NT_DEVICE\n" );
  }
  else if (lc->type == t_resource)
  {
    fprintf( fd, "#define NAME_STRING      \"%s.resource\"\n", lc->libname );
    fprintf( fd, "#define NT_TYPE          NT_RESOURCE\n" );
  }
  else if (lc->type == t_hidd)
  {
    fprintf( fd, "#define NAME_STRING      \"%s.hidd\"\n", lc->libname );
    fprintf( fd, "#define NT_TYPE          NT_HIDD\n" );
  }
  else if (lc->type == t_gadget)
  {
    fprintf( fd, "#define NAME_STRING      \"%s.gadget\"\n", lc->libname );
    fprintf( fd, "#define NT_TYPE          NT_LIBRARY\n" );
  }
  else if (lc->type == t_image)
  {
    fprintf( fd, "#define NAME_STRING      \"%s.image\"\n", lc->libname );
    fprintf( fd, "#define NT_TYPE          NT_LIBRARY\n" );
  }
  else if (lc->type == t_class)
  {
    fprintf( fd, "#define NAME_STRING      \"%s.class\"\n", lc->libname );
    fprintf( fd, "#define NT_TYPE          NT_LIBRARY\n" );
  }
  else if (lc->type == t_datatype)
  {
    fprintf( fd, "#define NAME_STRING      \"%s.datatype\"\n", lc->libname );
    fprintf( fd, "#define NT_TYPE          NT_LIBRARY\n" );
  }

  if (lc->option & o_rom)
      lc->option |= o_noexpunge;

  if (lc->option & o_noexpunge)
    fprintf( fd, "#define NOEXPUNGE\n" );
  if (lc->option & o_rom)
    fprintf( fd, "#define ROMBASED\n" );
  if (lc->option & o_nolibheader)
    fprintf( fd, "#define NOLIBHEADER\n" );

  if (lc->option & o_rom || lc->option & o_unique)
  {
    fprintf( fd, "#define LC_UNIQUE_PREFIX %s\n", lc->basename );
    fprintf( fd, "#define LC_BUILDNAME(n)  %s ## n\n", lc->basename );
  }
  else
  {
    fprintf( fd, "#define LC_BUILDNAME(n)  n\n" );
  }

  fprintf( fd, "#define LIBBASE          %s\n", lc->libbase );
  fprintf( fd, "#define LIBBASETYPE      %s\n", lc->libbasetype );
  fprintf( fd, "#define LIBBASETYPEPTR   %s\n", lc->libbasetypeptr );
  fprintf( fd, "#define VERSION_NUMBER   %d\n", lc->version );
  fprintf( fd, "#define REVISION_NUMBER  %d\n", lc->revision );
  fprintf( fd, "#define BASENAME         %s\n", lc->basename );
  fprintf( fd, "#define BASENAME_STRING  \"%s\"\n", lc->basename );
  fprintf( fd, "#define VERSION_STRING   \"$VER: %s %d.%d (%s)\\r\\n\"\n", lc->libname, lc->version, lc->revision , date );
  fprintf( fd, "#define LIBEND           %s_end\n", lc->basename );
  fprintf( fd, "#define LIBFUNCTABLE     %s_functable\n", lc->basename );
  fprintf( fd, "#define COPYRIGHT_STRING \"%s\"\n", lc->copyright );
  fprintf( fd, "#endif /* %s */\n", lc->define );

  fclose(fd);

return 0;
}

void writeFuncHeader (FILE * fh, Function * func, const char * macro)
{
    int n = GetListLen (&func->parameters);

    fprintf (fh, "%s%s%d%s(%s, %s, \\\n",
	macro,
	(func->flags & FUNCFLAG_RETURNSQUAD) ? "QUAD" : "",
	n,
	(func->flags & FUNCFLAG_INDEPENDENT) ? "I" : "",
	func->type, func->node.name
    );
}

void writeFunctionToFile (FILE * fh, Function * func)
{
    Parameter * par;
    Node * reg;
    char str[128], * ptr;

    writeFuncHeader (fh, func, "AROS_LH");
    ForeachNode (&func->parameters, par)
    {
	ptr = str;
	*ptr = 0;
	ForeachNode (&par->registers, reg)
	{
	    strcat (ptr, reg->name);
	    if (reg->next->next)
		strcat (ptr, ", ");
	}
	//printf ("Reg '%s'\n", str);

	fprintf (fh, "    AROS_%s(%s, %s, %s), \\\n",
	    par->macro, par->type, par->node.name, str
	);
    }

    fprintf (fh, "LIBBASETYPEPTR, LIBBASE, %d, BASENAME)\n", func->libOffset);
    fprintf (fh, "{\n    AROS_LIBFUNC_INIT\n\n");
    fprintf (fh, "%s\n", func->code);
    fprintf (fh, "\n    AROS_LIBFUNC_EXIT\n} /* %s */\n", func->node.name);
}

void writeArchiveHeader (FILE * fh, Archive * arch)
{
    HeaderLine * line;

    fprintf (fh, "#ifndef LC_LIBDEFS_FILE\n#define LC_LIBDEFS_FILE \"libdefs.h\"\n#endif\n#include LC_LIBDEFS_FILE\n\n");
    ForeachNode (&arch->header, line)
    {
	fprintf (fh, "%s\n", line->node.name);
    }
}

int extractfiles(int argc, char **argv)
{
    Archive * arch;
    Function * func;
    char * filename;
    char * newname;
    FILE * fh;
    
    if(argc != 3)
    {
	fprintf( stderr, "Usage: %s <destdir> <archfiles...>\n", argv[0] );
	exit(-1);
    }
    arch = newArchive (argc-2, &argv[2]);

    chdir(argv[1]);

    ForeachNode (&arch->functions, func)
    {
	filename = joinVAStrings (NULL, func->node.name, ".c", NULL);
	newname = joinVAStrings (NULL, func->node.name, ".c.new", NULL);

	fh = fopen (newname, "w");
	if (!fh)
	{
	    fprintf( stderr, "Couldn't open file %s!\n", newname );
	    exit (1);
	}

	writeArchiveHeader (fh, arch);
	writeFunctionToFile (fh, func);
	fclose (fh);
	moveifchanged (filename, newname);
	free (filename);
	free (newname);
    }
    
    return 0;
}

void emit(FILE *out, struct libconf *lc, char **names, int number)
{
int i;

  fprintf( out, "/*\n" );
  fprintf( out, "    Copyright (C) 1995-1998 AROS - The Amiga Research OS\n" );
  fprintf( out, "    *** Automatic generated file. Do not edit ***\n" );
  fprintf( out, "    Desc: Function table for %s\n", lc->basename );
  fprintf( out, "    Lang: english\n" );
  fprintf( out, "*/\n" );
  fprintf( out, "#ifndef LIBCORE_COMPILER_H\n" );
  fprintf( out, "#   include <libcore/compiler.h>\n" );
  fprintf( out, "#endif\n" );
  fprintf( out, "#ifndef NULL\n" );
  fprintf( out, "#define NULL ((void *)0)\n" );
  fprintf( out, "#endif\n\n" );
  fprintf( out, "#ifndef LC_LIBDEFS_FILE\n" );
  fprintf( out, "#define LC_LIBDEFS_FILE \"libdefs.h\"\n" );
  fprintf( out, "#endif\n" );
  fprintf( out, "#include LC_LIBDEFS_FILE\n" );
  if(lc->option & o_nolibheader)
  {
    fprintf( out, "extern void AROS_SLIB_ENTRY(open,BASENAME) (void);\n" );
    fprintf( out, "extern void AROS_SLIB_ENTRY(close,BASENAME) (void);\n" );
    fprintf( out, "extern void AROS_SLIB_ENTRY(expunge,BASENAME) (void);\n" );
    fprintf( out, "extern void AROS_SLIB_ENTRY(null,BASENAME) (void);\n" );
  }
  else
  {
    fprintf( out, "extern void AROS_SLIB_ENTRY(LC_BUILDNAME(OpenLib),LibHeader) (void);\n" );
    fprintf( out, "extern void AROS_SLIB_ENTRY(LC_BUILDNAME(CloseLib),LibHeader) (void);\n" );
    fprintf( out, "extern void AROS_SLIB_ENTRY(LC_BUILDNAME(ExpungeLib),LibHeader) (void);\n" );
    fprintf( out, "extern void AROS_SLIB_ENTRY(LC_BUILDNAME(ExtFuncLib),LibHeader) (void);\n" );
  }
  for( i = 0 ; i < number-4 ; i++ )
  {
    if(names[i])
      fprintf( out, "extern void AROS_SLIB_ENTRY(%s,BASENAME) (void);\n", names[i] );
  }
  fprintf( out, "\nvoid *const LIBFUNCTABLE[]=\n{\n" );
  if(lc->option & o_nolibheader)
  {
    fprintf( out, "    AROS_SLIB_ENTRY(open, BASENAME),\n" );
    fprintf( out, "    AROS_SLIB_ENTRY(close, BASENAME),\n" );
    fprintf( out, "    AROS_SLIB_ENTRY(expunge, BASENAME),\n" );
    fprintf( out, "    AROS_SLIB_ENTRY(null, BASENAME),\n" );
  }
  else
  {
    fprintf( out, "    AROS_SLIB_ENTRY(LC_BUILDNAME(OpenLib),LibHeader),\n" );
    fprintf( out, "    AROS_SLIB_ENTRY(LC_BUILDNAME(CloseLib),LibHeader),\n" );
    fprintf( out, "    AROS_SLIB_ENTRY(LC_BUILDNAME(ExpungeLib),LibHeader),\n" );
    fprintf( out, "    AROS_SLIB_ENTRY(LC_BUILDNAME(ExtFuncLib),LibHeader),\n" );
  }
  for( i = 0 ; i < number-4 ; i++ )
  {
    if(names[i])
      fprintf( out, "    AROS_SLIB_ENTRY(%s,BASENAME), /* %d */\n", names[i], i+5 );
    else
      fprintf( out, "    NULL, /* %d */\n", i+5 );
  }
  fprintf( out, "    (void *)-1L\n};\n" );
}

int genfunctable(int argc, char **argv)
{
FILE *fd = NULL, *fdo;
struct libconf *lc;
char *line = 0;
char *word, **words = NULL;
int in_archive, in_header, in_function, in_autodoc, in_code;
int num;
char *funcname = NULL, **funcnames = NULL;

/* Well, there are already 4 functions (open,close,expunge,null) */
int numfuncs = 4;

int has_arch = 1;


  /* First check if we have a HIDD which does not have an archive */
  if(!(lc=parse_libconf(NULL)))
    return(-1);

  if (lc->type == t_hidd || lc->type == t_gadget || lc->type == t_class)
    has_arch = 0;

  if(has_arch)
  {
    if(argc == 2)
    {
      fd = fopen(argv[1],"rb");
      if(!fd)
      {
        fprintf( stderr, "Couldn't open file %s!\n", argv[1] );
        exit(-1);
      }
    }
    else if (argc == 1)
    {
      has_arch = 0;
    }
    else
    {
      fprintf( stderr, "Usage: %s <archfile>\n", argv[0] );
      exit(-1);
    }
  }
  fdo = fopen("functable.c.new","w");
  if(!fdo)
  {
    fprintf( stderr, "Couldn't open file %s!\n", "functable.c.new" );
    exit(-1);
  }

  if(has_arch)
  {
    in_archive = 0;
    in_function = 0;
    in_autodoc = 0;
    in_code = 0;
    in_header = 0;
    while( (line = get_line(fd)) )
    {
      word = keyword(line);
      if( word )
      {
        if( strcmp(word,"Archive")==0 && !in_archive )
          in_archive = 1;
        else if( strcmp(word,"/Archive")==0 && in_archive && ! in_function )
          break;
        else if( strcmp(word,"AutoDoc")==0 && in_function && !in_autodoc && !in_code )
          in_autodoc = 1;
        else if( strcmp(word,"/AutoDoc")==0 && in_autodoc )
          in_autodoc = 0;
        else if( strcmp(word,"Code")==0 && in_function && !in_code && !in_autodoc )
          in_code = 1;
        else if( strcmp(word,"/Code")==0 && in_code )
          in_code = 0;
        else if( strcmp(word,"Header")==0 && in_archive && !in_function )
          in_header = 1;
        else if( strcmp(word,"/Header")==0 && in_header )
          in_header = 0;
        else if( strcmp(word,"Function")==0 && in_archive && !in_function && !in_header )
        {
          num = get_words(line,&words);
          funcname = strdup(words[num-1]);
          in_function = 1;
        }
        else if( strcmp(word,"/Function")==0 && in_function && !in_autodoc && !in_code )
          in_function = 0;
        else if( strcmp(word,"LibOffset")==0 && in_function && !in_autodoc && !in_code )
        {
          get_words(line,&words);
          num = atoi(words[1]);
          if( num>numfuncs )
          {
            funcnames = realloc( funcnames, (num-4) * sizeof(char *));
            /* initialize new memory */
            for( ;numfuncs<num; numfuncs++)
              funcnames[numfuncs-4] = NULL;
          }
          funcnames[num-5] = funcname;
        }

        free(word);
      }
      free(line);
    }
  }
  emit(fdo,lc,funcnames,numfuncs);
  fclose(fdo);
  if(has_arch)
  {
    fclose(fd);
  }
  moveifchanged("functable.c","functable.c.new");
  free(lc);

return 0;
}


int gensource(int argc, char **argv)
{
    Archive * arch;
    Function * func;
    char * filename;
    FILE * fh;

    if(argc != 3)
    {
	fprintf( stderr, "Usage: %s <destfile> <archfiles...>\n", argv[0] );
	exit(-1);
    }
    arch = newArchive (argc-2, &argv[2]);

    filename = joinVAStrings (NULL, argv[1], ".new", NULL);
    fh = fopen (filename, "w");
    if (!fh)
    {
	fprintf( stderr, "Couldn't open file %s!\n", filename);
	exit (1);
    }

    writeArchiveHeader (fh, arch);

    ForeachNode (&arch->functions, func)
    {
	writeFunctionToFile (fh, func);
    }

    fclose (fh);
    moveifchanged (argv[1], filename);
    free (filename);

    return 0;
}


int genautodocs(int argc, char **argv)
{
FILE *fd, *fdo = NULL;
char *line = 0;
char *word, **words = NULL;
int in_archive, in_header, in_function, in_autodoc, in_afunc, in_code ;
int num, i, len;
char **name = NULL, **type = NULL;
int numregs = 1;
int numparams = 0;

  if(argc != 3)
  {
    fprintf( stderr, "Usage: %s <destdir> <archfile>\n", argv[0] );
    exit(-1);
  }
  fd = fopen(argv[2],"rb");
  if(!fd)
  {
    fprintf( stderr, "Couldn't open file %s!\n", argv[2] );
    exit(-1);
  }
  chdir(argv[1]);

  in_archive = 0;
  in_header = 0;
  in_function = 0;
  in_autodoc = 0;
  in_afunc = 0;
  in_code = 0;
  while( (line = get_line(fd)) )
  {
    word = keyword(line);
    if( word )
    {
      if( strcmp(word,"Archive")==0 && !in_archive )
      {
        in_archive = 1;
      }
      else if( strcmp(word,"/Archive")==0 && in_archive && !in_header && !in_function )
        break;
      else if( strcmp(word,"AutoDoc")==0 && in_function && !in_autodoc && !in_code )
      {
      char *filename;

        filename = malloc( (strlen(name[0])+6)*sizeof(char) );
        sprintf( filename, "%s.adoc", name[0] );
        in_autodoc = 1;
        fdo = fopen(filename,"w");
        if(!fdo)
        {
          fprintf( stderr, "Couldn't open file %s!\n", filename );
          exit(-1);
        }
        free(filename);
        fprintf( fdo, "NAME\n    %s %s( ", type[0], name[0] );
        for( i = 1 ; i <= numparams ; i++ )
        {
          if( i != 1 )
            fprintf( fdo, ", " );
          fprintf( fdo, "%s %s", type[i], name[i] );
        }
        fprintf( fdo, " )\n" );
      }
      else if( strcmp(word,"/AutoDoc")==0 && in_autodoc && !in_afunc )
      {
        fclose(fdo);
        in_autodoc = 0;
      }
      else if( strcmp(word,"Function")==0 )
      {
        if( in_archive && !in_function && !in_autodoc && !in_code )
        {
          in_function = 1;
          num = get_words(line,&words);
          numparams = 0;
          name = realloc( name, sizeof(char *) );
          name[0] = strdup(words[num-1]);
          type = realloc( type, sizeof(char *) );
          len = 0;
          for( i=1 ; i < num-1 ; i++ )
            len += (strlen(words[i])+1);
          type[0] = malloc( len * sizeof(char) );
          strcpy( type[0], words[1]);
          for( i=2 ; i < num-1 ; i++ )
          {
            strcat( type[0], " " );
            strcat( type[0], words[i] );
          }
          numregs = 1;
        }
        else if( in_autodoc && !in_afunc )
        {
          fprintf( fdo, "\nFUNCTION\n" );
          num = get_words(line,&words);
          if(num==1)
            in_afunc = 1;
        }
      }
      else if( strcmp(word,"/Function")==0 )
      {
        if( in_function && !in_autodoc )
        {
          for( i=0 ; i < numparams ; i++ )
          {
            free(name[i]);
            free(type[i]);
          }
          free(name);
          free(type);
          name = NULL;
          type = NULL;
          in_function = 0;
        }
        else if( in_afunc )
          in_afunc = 0;
      }
      else if( strcmp(word,"Parameter")==0 && in_function && !in_autodoc && !in_code )
      {
        numparams++;
        num = get_words(line,&words);
        name = realloc( name, (numparams+1)*sizeof(char *) );
        name[numparams] = strdup(words[num-1-numregs]);
        type = realloc( type, (numparams+1)*sizeof(char *) );
        len = 0;
        for ( i = 1 ; i < num-2 ; i++ )
          len += strlen(words[i]);
        type[numparams] = malloc( (len+num-3) * sizeof(char) );
        strcpy( type[numparams], words[1]);
        for ( i = 2 ; i < num-2 ; i++ )
        {
          strcat( type[numparams], " " );
          strcat( type[numparams], words[i] );
        }
        do
        {
          free ( line );
          line = get_line( fd );
        } while ( strncmp(line,"#/Parameter",11)!=0 );
      }
      else if( strcmp(word,"Inputs")==0 && in_autodoc )
        fprintf( fdo, "\nINPUTS\n" );
      else if( strcmp(word,"Result")==0 && in_autodoc )
        fprintf( fdo, "\nRESULT\n" );
      else if( strcmp(word,"Notes")==0 && in_autodoc )
        fprintf( fdo, "\nNOTES\n" );
      else if( strcmp(word,"Example")==0 && in_autodoc )
        fprintf( fdo, "\nEXAMPLE\n" );
      else if( strcmp(word,"Bugs")==0 && in_autodoc )
        fprintf( fdo, "\nBUGS\n" );
      else if( strcmp(word,"SeeAlso")==0 && in_autodoc )
        fprintf( fdo, "\nSEE ALSO\n" );
      else if( strcmp(word,"Internals")==0 && in_autodoc )
        fprintf( fdo, "\nINTERNALS\n" );
      else if( strcmp(word,"History")==0 && in_autodoc )
        fprintf( fdo, "\nHISTORY\n" );
      else if( strcmp(word,"Item")==0 && in_autodoc )
        fprintf( fdo, "<Item>%s\n", &line[6] );

      free(word);
    }
    else if(in_autodoc && line[0] )
      fprintf( fdo, "%s\n", line );
    free(line);
  }
  fclose(fd);

return 0;
}

void replace( FILE *in, FILE *out, int num )
{
int in_archive = 0, in_header = 0, in_function = 0, in_autodoc = 0, in_code = 0;
int i = 0, writefunc = 0;
char *word, *line;

  rewind(in);
  while( (line = get_line(in)) )
  {
    word = keyword(line);
    if( word )
    {
      if( strcmp(word,"Archive")==0 && !in_archive )
        in_archive = 1;
      else if( strcmp(word,"/Archive")==0 && in_archive && !in_header && !in_function )
        break;
      else if( strcmp(word,"Header")==0 && in_archive && !in_header && !in_function )
        in_header = 1;
      else if( strcmp(word,"/Header")==0 && in_header )
        in_header = 0;
      else if( strcmp(word,"AutoDoc")==0 && in_function && !in_autodoc && !in_code )
        in_autodoc = 1;
      else if( strcmp(word,"/AutoDoc")==0 && in_autodoc )
        in_autodoc = 0;
      else if( strcmp(word,"Function")==0 && in_archive && !in_function )
      {
        in_function = 1;
        if( i == num )
        {
          writefunc = 1;
        }
        else if( i > num )
          return;
        i++;
      }
      else if( strcmp(word,"/Function")==0 && in_function && !in_autodoc && !in_code )
      {
        in_function = 0;
        if( writefunc )
          fprintf( out, "#/Function\n\n\n" );
      }

      free(word);
    }
    if( in_function && writefunc )
      fprintf( out, "%s\n", line );
    free(line);
  }
}


int mergearch(int argc, char **argv)
{
FILE *fd1, *fd2, *fdo;
char *line = 0;
char *word, **words = NULL;
int in_archive, in_header, in_function, in_autodoc, in_code;
int num,i;
char **name1 = NULL, **name2 = NULL;
int num1, num2, *rep = NULL;
int replace_function;

  if(argc != 4)
  {
    fprintf( stderr, "Usage: %s <arch1> <arch2> <archout>\n", argv[0] );
    exit(-1);
  }
  fd1 = fopen(argv[1],"rb");
  if(!fd1)
  {
    fprintf( stderr, "Couldn't open file %s!\n", argv[1] );
    exit(-1);
  }
  fd2 = fopen(argv[2],"rb");
  if(!fd2)
  {
    fprintf( stderr, "Couldn't open file %s!\n", argv[2] );
    exit(-1);
  }
  fdo = fopen(argv[3],"w");
  if(!fdo)
  {
    fprintf( stderr, "Couldn't open file %s!\n", argv[3] );
    exit(-1);
  }

  fprintf( fdo, "#Archive\n#Header\n" );
  /* Get function names of 1st file and copy write header */
  num1 = 0;
  in_archive = 0;
  in_header = 0;
  in_function = 0;
  in_autodoc = 0;
  in_code = 0;
  while( (line = get_line(fd1)) )
  {
    word = keyword(line);
    if( word && (isupper(word[0]) || isupper(word[1])) )
    {
      if( strcmp(word,"Archive")==0 && !in_archive )
        in_archive = 1;
      else if( strcmp(word,"/Archive")==0 && in_archive && !in_header && !in_function )
        break;
      else if( strcmp(word,"Code")==0 && in_function && !in_code && !in_autodoc )
        in_code = 1;
      else if( strcmp(word,"/Code")==0 && in_code )
        in_code = 0;
      else if( strcmp(word,"Header")==0 && in_archive && !in_header && !in_function )
        in_header = 1;
      else if( strcmp(word,"/Header")==0 && in_header )
        in_header = 0;
      else if( strcmp(word,"AutoDoc")==0 && in_function && !in_autodoc && !in_code )
        in_autodoc = 1;
      else if( strcmp(word,"/AutoDoc")==0 && in_autodoc )
        in_autodoc = 0;
      else if( strcmp(word,"Function")==0 && in_archive && !in_function && !in_header )
      {
        in_function = 1;
        num = get_words(line,&words);
        num1++;
        name1 = realloc( name1, num1 * sizeof(char *) );
        name1[num1-1] = strdup(words[num-1]);
      }
      else if( strcmp(word,"/Function")==0 && in_function && !in_autodoc && !in_code )
        in_function = 0;

      free(word);
    }
    else
    {
      if( in_header )
        fprintf( fdo, "%s\n", line );
    }
    free(line);
  }
  /* Get function names of 2nd file and append header */
  num2 = 0;
  in_archive = 0;
  in_function = 0;
  in_autodoc = 0;
  in_header = 0;
  in_code = 0;
  while( (line = get_line(fd2)) )
  {
    word = keyword(line);
    if( word && (isupper(word[0]) || isupper(word[1])) )
    {
      if( strcmp(word,"Archive")==0 && !in_archive )
        in_archive = 1;
      else if( strcmp(word,"/Archive")==0 && in_archive && !in_header && !in_function )
        break;
      else if( strcmp(word,"Header")==0 && in_archive && !in_header && !in_function )
        in_header = 1;
      else if( strcmp(word,"/Header")==0 && in_header )
        in_header = 0;
      else if( strcmp(word,"AutoDoc")==0 && in_function && !in_autodoc && !in_code )
        in_autodoc = 1;
      else if( strcmp(word,"/AutoDoc")==0 && in_autodoc )
        in_autodoc = 0;
      else if( strcmp(word,"Function")==0 && in_archive && !in_function )
      {
        in_function = 1;
        num = get_words(line,&words);
        num2++;
        name2 = realloc( name2, num2 * sizeof(char *) );
        name2[num2-1] = strdup(words[num-1]);
      }
      else if( strcmp(word,"/Function")==0 && in_function && !in_autodoc && !in_code )
        in_function = 0;

      free(word);
    }
    else
    {
      if( in_header )
        fprintf( fdo, "%s\n", line );
    }
    free(line);
  }
  rewind(fd1);
  rewind(fd2);
  rep = calloc( num2, sizeof(int) );
  fprintf( fdo, "#/Header\n\n" );
  /* Produce merged file */
  num = 0;
  in_archive = 0;
  in_header = 0;
  in_function = 0;
  in_autodoc = 0;
  in_code = 0;
  replace_function = 0;
  while( (line = get_line(fd1)) )
  {
    word = keyword(line);
    if( word && (isupper(word[0]) || isupper(word[1])) )
    {
      if( strcmp(word,"Archive")==0 && !in_archive )
        in_archive = 1;
      else if( strcmp(word,"/Archive")==0 && in_archive && !in_header && !in_function )
        break;
      else if( strcmp(word,"Header")==0 && in_archive && !in_header && !in_function )
        in_header = 1;
      else if( strcmp(word,"/Header")==0 && in_header )
        in_header = 0;
      else if( strcmp(word,"AutoDoc")==0 && in_function && !in_autodoc && !in_code )
        in_autodoc = 1;
      else if( strcmp(word,"/AutoDoc")==0 && in_autodoc )
        in_autodoc = 0;
      else if( strcmp(word,"Function")==0 && in_archive && !in_function )
      {
        in_function = 1;
        for( i = 0 ; i < num2 && strcmp(name1[num],name2[i])!=0 ; i++ );
        if( i == num2 )
          replace_function = 0;
        else
        {
          replace_function = 1;
          replace(fd2,fdo,i);
          rep[i] = 1;
        }
        num++;
      }
      else if( strcmp(word,"/Function")==0 && in_function && !in_autodoc && !in_code )
      {
        in_function = 0;
        if( !replace_function )
          fprintf( fdo, "#/Function\n\n\n" );
      }

      free(word);
    }
    if( in_function && !replace_function )
      fprintf( fdo, "%s\n", line );
    free(line);
  }
  rewind(fd2);
  /* Append non-replaced functions */
  num = 0;
  in_archive = 0;
  in_header = 0;
  in_function = 0;
  in_autodoc = 0;
  in_code = 0;
  replace_function = 0;
  while( (line = get_line(fd2)) )
  {
    word = keyword(line);
    if( word && (isupper(word[0]) || isupper(word[1])) )
    {
      if( strcmp(word,"Archive")==0 && !in_archive )
        in_archive = 1;
      else if( strcmp(word,"/Archive")==0 && in_archive && !in_header && !in_function )
        break;
      else if( strcmp(word,"Header")==0 && in_archive && !in_header && !in_function )
        in_header = 1;
      else if( strcmp(word,"/Header")==0 && in_header )
        in_header = 0;
      else if( strcmp(word,"AutoDoc")==0 && in_function && !in_autodoc && !in_code )
        in_autodoc = 1;
      else if( strcmp(word,"/AutoDoc")==0 && in_autodoc )
        in_autodoc = 0;
      else if( strcmp(word,"Function")==0 && in_archive && !in_function )
      {
        in_function = 1;
        if( rep[num] )
          replace_function = 1;
        else
          replace_function = 0;
        num++;
      }
      else if( strcmp(word,"/Function")==0 && in_function && !in_autodoc && !in_code )
      {
        in_function = 0;
        if( !replace_function )
          fprintf( fdo, "#/Function\n\n\n" );
      }

      free(word);
    }
    if( in_function && !replace_function )
      fprintf( fdo, "%s\n", line );
    free(line);
  }

  fprintf( fdo, "#/Archive\n\n" );
  fclose(fd2);
  fclose(fd1);
  fclose(fdo);
return 0;
}


int gendefines(int argc, char **argv)
{
FILE *fd, *fdo = NULL, *headerstempl;
char *filename, *newname;
struct libconf *lc;
char *upperbasename;
char *line = 0;
char *word = NULL, **words = NULL;
int in_archive, in_header, in_function, in_autodoc, in_code;
int num, i, len;
char **name = NULL, **type = NULL, **reg = NULL;
int numregs = 1;
char *macro[2];
int numparams=0;
int firstlvo;

  if(argc != 3)
  {
    fprintf( stderr, "Usage: %s <incdir> <archfile>\n", argv[0] );
    exit(-1);
  }
  if(!(lc=parse_libconf(NULL)))
    return(-1);
  fd = fopen(argv[2],"rb");
  if(!fd)
  {
    fprintf( stderr, "Couldn't open file %s!\n", argv[2] );
    return(-1);
  }
  filename = malloc( (strlen(argv[1])+strlen(lc->libname)+12) * sizeof(char) );
  sprintf( filename, "%s/defines/%s.h", argv[1], lc->libname );
  newname = malloc( (strlen(argv[1])+strlen(lc->libname)+16) * sizeof(char) );
  sprintf( newname, "%s/defines/%s.h.new", argv[1], lc->libname );
  fdo = fopen(newname,"w");
  if(!fdo)
  {
    fprintf( stderr, "Couldn't open file %s!\n", newname );
    return(-1);
  }

  upperbasename = strdup( lc->basename );
  strupper( upperbasename );
  fprintf( fdo, "#ifndef DEFINES_%s_PROTOS_H\n", upperbasename );
  fprintf( fdo, "#define DEFINES_%s_PROTOS_H\n\n", upperbasename );
  fprintf( fdo, "/*\n" );
  fprintf( fdo, "    Copyright (C) 1995-1998 AROS - The Amiga Research OS\n" );
  fprintf( fdo, "    *** Automatic generated file. Do not edit ***\n" );
  fprintf( fdo, "    Desc: Prototypes for %s.", lc->basename );
  switch( lc->type )
  {
    case t_resource:
      fprintf( fdo, "resource" );
      firstlvo = 0;
      break;
    case t_device:
      fprintf( fdo, "device" );
      firstlvo = 6;
      break;
    case t_hidd:
      fprintf( fdo, "hidd" );
      firstlvo = 4;
      break;
    case t_library:
    default:
      fprintf( fdo, "library" );
      firstlvo = 4;
      break;
  }
  fprintf( fdo, "\n    Lang: english\n" );
  fprintf( fdo, "*/\n\n" );
  fprintf( fdo, "#ifndef AROS_LIBCALL_H\n" );
  fprintf( fdo, "#   include <aros/libcall.h>\n" );
  fprintf( fdo, "#endif\n" );
  fprintf( fdo, "#ifndef EXEC_TYPES_H\n" );
  fprintf( fdo, "#   include <exec/types.h>\n" );
  fprintf( fdo, "#endif\n\n" );
  headerstempl = fopen( "headers.tmpl", "rb" );
  if( headerstempl )
  {
    in_header = 0;
    while( (line = get_line(headerstempl)) )
    {
      num = get_words(line,&words);
      if( num == 2 && strcmp(words[0],"##begin")==0 && strcmp(words[1],"defines")==0 )
      {
        in_header = 1;
      }
      else if( num == 2 && strcmp(words[0],"##end")==0 && strcmp(words[1],"defines")==0 )
      {
        in_header = 0;
      }
      else if( in_header )
      {
        fprintf( fdo, "%s\n", line );
      }
      free(line);
    }
    fclose(headerstempl);
    fprintf( fdo, "\n" );
  }
  fprintf( fdo, "\n/* Defines */\n" );

  macro[0] = strdup("LC");
  macro[1] = strdup("LCA");
  in_archive = 0;
  in_header = 0;
  in_function = 0;
  in_autodoc = 0;
  in_code = 0;
  while( (line = get_line(fd)) )
  {
    free(word);
    word = keyword(line);
    if( word && (isupper(word[0]) || isupper(word[1])) )
    {
      if( strcmp(word,"Archive")==0 && !in_archive )
        in_archive = 1;
      else if( strcmp(word,"/Archive")==0 && in_archive && !in_header && ! in_function )
        break;
      else if( strcmp(word,"AutoDoc")==0 && in_function && !in_code && !in_autodoc )
        in_autodoc = 1;
      else if( strcmp(word,"/AutoDoc")==0 && in_autodoc )
        in_autodoc = 0;
      else if( strcmp(word,"Code")==0 && in_function && !in_code && !in_autodoc )
        in_code = 1;
      else if( strcmp(word,"/Code")==0 && in_code )
        in_code = 0;
      else if( strcmp(word,"Options")==0 && in_function && !in_code && !in_autodoc )
      {
        num = get_words ( line, &words );
        if(strcmp(words[1],"QUAD")==0)
        {
          free ( macro[0] );
          free ( macro[1] );
          macro[0] = strdup("LCQUAD");
          macro[1] = strdup("LCAQUAD");
	  numregs = 2;
        }
      }
      else if( strcmp(word,"Function")==0 && in_archive && !in_function && !in_header )
      {
        num = get_words(line,&words);
        name = realloc( name, sizeof(char *) );
        name[0] = strdup(words[num-1]);
        type = realloc( type, sizeof(char *) );
        len = 0;
        for( i=1 ; i < num-1 ; i++ )
          len += (strlen(words[i])+1);
        type[0] = malloc( len * sizeof(char) );
        strcpy( type[0], words[1]);
        for( i=2 ; i < num-1 ; i++ )
        {
          strcat( type[0], " " );
          strcat( type[0], words[i] );
        }
        numparams = 0;
        in_function = 1;
      }
      else if( strcmp(word,"/Function")==0 && in_function && !in_autodoc && !in_code )
      {
        if( atoi(reg[0]) > firstlvo )
        {
          fprintf( fdo, "\n#define %s(", name[0] );
          for( i = 1 ; i <= numparams ; i++ )
          {
            if( i!=1 )
              fprintf( fdo, ", " );
            fprintf( fdo, "%s", name[i] );
          }
          fprintf( fdo, ") \\\n\tAROS_%s%d(%s, %s, \\\n", macro[0], numparams, type[0], name[0] );
          for( i = 1 ; i <= numparams ; i++ )
            fprintf( fdo, "\tAROS_%s(%s, %s, %s), \\\n", macro[1], type[i], name[i], reg[i] );
          fprintf( fdo, "\t%s, %s, %s, %s)\n", lc->libbasetypeptr, lc->libbase, reg[0], lc->basename );
        }
        for( i=0 ; i < numparams ; i++ )
        {
          free(name[i]);
          free(type[i]);
          free(reg[i]);
        }
        free(name);
        free(type);
        free(reg);
        name = NULL;
        type = NULL;
        reg = NULL;
        in_function = 0;
      }
      else if( strcmp(word,"Header")==0 && in_archive && !in_function && !in_header )
        in_header = 1;
      else if( strcmp(word,"/Header")==0 && in_header )
        in_header = 0;
      else if( strcmp(word,"LibOffset")==0 && in_function && !in_autodoc && !in_code )
      {
        num = get_words(line,&words);
        reg = realloc( reg, (numparams+1)*sizeof(char *) );
        reg[0] = strdup(words[1]);
      }
      else if( strcmp(word,"Parameter")==0 && in_function && !in_autodoc && !in_code )
      {
        numparams++;
        num = get_words( line, &words );
        name = realloc( name, (numparams+1) * sizeof(char *) );
        name[numparams] = strdup( words[num-2] );
        type = realloc( type, (numparams+1) * sizeof(char *) );
        len = 0;
        for( i=1 ; i < num-2 ; i++ )
          len += strlen(words[i]);
        type[numparams] = malloc( (len+num-3) * sizeof(char) );
        strcpy( type[numparams], words[1]);
        for( i=2 ; i < num-2 ; i++ )
        {
          strcat( type[numparams], " " );
          strcat( type[numparams], words[i] );
        }

	reg = realloc( reg, (numparams+1)*sizeof(char *) );
        reg[numparams] = malloc(4*numregs - 1);
	reg[numparams][4*numregs -1] = '\0';
	strncpy(reg[numparams], words[num-1], 2);
	reg[numparams][4*numregs -1] = 	reg[numparams][2] = '\0';
	for (i = 2 ; words[num-1][i]; )
	{
	    if (words[num-1][i++] == '/')
	    {
		strcat(reg[numparams], ", ");
		strncat(reg[numparams], &words[num-1][i], 2);
		i+=2;
		len += 4;
            }
	    else
	    {
	        fprintf(stderr, "Error: register name not known '%s'\n", &words[num-1][i]);
		exit(1);
            }
	}
        do
        {
          free ( line );
          line = get_line( fd );
        } while ( strncmp(line,"#/Parameter",11)!=0 );
      }
    }

    free(line);
  }
  fprintf( fdo, "\n#endif /* DEFINES_%s_PROTOS_H */\n", upperbasename );
  fclose(fdo);
  fclose(fd);
  moveifchanged(filename,newname);
  free(newname);
  free(filename);
  free(lc);
  free(macro[0]);
  free(macro[1]);

return 0;
}

int genclib(int argc, char **argv)
{
FILE *fd, *fdo = NULL, *headerstempl;
char *filename, *newname;
struct libconf *lc;
char *upperbasename;
char *line = 0;
char *word = NULL, **words = NULL;
int in_archive, in_header, in_function, in_autodoc, in_code;
int num, i, len;
char **name = NULL, **type = NULL, **reg = NULL;
int numregs = 1;
char *macro[2];
int numparams=0;
int firstlvo;

  if(argc != 3)
  {
    fprintf( stderr, "Usage: %s <incdir> <archfile>\n", argv[0] );
    exit(-1);
  }
  if(!(lc=parse_libconf(NULL)) )
    return(-1);
  fd = fopen(argv[2],"rb");
  if(!fd)
  {
    fprintf( stderr, "Couldn't open file %s!\n", argv[2] );
    return(-1);
  }
  filename = malloc( (strlen(argv[1])+strlen(lc->libname)+16) * sizeof(char) );
  sprintf( filename, "%s/clib/%s_protos.h", argv[1], lc->libname );
  newname = malloc( (strlen(argv[1])+strlen(lc->libname)+20) * sizeof(char) );
  sprintf( newname, "%s/clib/%s_protos.h.new", argv[1], lc->libname );
  fdo = fopen(newname,"w");
  if(!fdo)
  {
    fprintf( stderr, "Couldn't open file %s!\n", newname );
    return(-1);
  }

  upperbasename = strdup( lc->basename );
  strupper( upperbasename );
  fprintf( fdo, "#ifndef CLIB_%s_PROTOS_H\n", upperbasename );
  fprintf( fdo, "#define CLIB_%s_PROTOS_H\n\n", upperbasename );
  fprintf( fdo, "/*\n" );
  fprintf( fdo, "    Copyright (C) 1995-1998 AROS - The Amiga Research OS\n" );
  fprintf( fdo, "    *** Automatic generated file. Do not edit ***\n" );
  fprintf( fdo, "    Desc: Prototypes for %s.", lc->basename );
  switch( lc->type )
  {
    case t_resource:
      fprintf( fdo, "resource" );
      firstlvo = 0;
      break;
    case t_device:
      fprintf( fdo, "device" );
      firstlvo = 6;
      break;
    case t_hidd:
      fprintf( fdo, "hidd" );
      firstlvo = 4;
      break;
    case t_library:
    default:
      fprintf( fdo, "library" );
      firstlvo = 4;
      break;
  }
  fprintf( fdo, "\n    Lang: english\n" );
  fprintf( fdo, "*/\n\n" );
  fprintf( fdo, "#ifndef AROS_LIBCALL_H\n" );
  fprintf( fdo, "#   include <aros/libcall.h>\n" );
  fprintf( fdo, "#endif\n\n" );
  headerstempl = fopen( "headers.tmpl", "rb" );
  if( headerstempl )
  {
    in_header = 0;
    while( (line = get_line(headerstempl)) )
    {
      num = get_words(line,&words);
      if( num == 2 && strcmp(words[0],"##begin")==0 && strcmp(words[1],"clib")==0 )
      {
        in_header = 1;
      }
      else if( num == 2 && strcmp(words[0],"##end")==0 && strcmp(words[1],"clib")==0 )
      {
        in_header = 0;
      }
      else if( in_header )
      {
        fprintf( fdo, "%s\n", line );
      }
      free(line);
    }
    fclose(headerstempl);
    fprintf( fdo, "\n" );
  }
  fprintf( fdo, "\n/* Prototypes */\n" );

  macro[0] = strdup("LP");
  macro[1] = strdup("LPA");
  in_archive = 0;
  in_header = 0;
  in_function = 0;
  in_autodoc = 0;
  in_code = 0;
  while( (line = get_line(fd)) )
  {
    free(word);
    word = keyword(line);
    if( word && (isupper(word[0]) || isupper(word[1])) )
    {
      if( strcmp(word,"Archive")==0 && !in_archive )
        in_archive = 1;
      else if( strcmp(word,"/Archive")==0 && in_archive && !in_header && ! in_function )
        break;
      else if( strcmp(word,"AutoDoc")==0 && in_function && !in_code && !in_autodoc )
        in_autodoc = 1;
      else if( strcmp(word,"/AutoDoc")==0 && in_autodoc )
        in_autodoc = 0;
      else if( strcmp(word,"Code")==0 && in_function && !in_code && !in_autodoc )
        in_code = 1;
      else if( strcmp(word,"/Code")==0 && in_code )
        in_code = 0;
      else if( strcmp(word,"Options")==0 && in_function && !in_code && !in_autodoc )
      {
        num = get_words ( line, &words );
        if(strcmp(words[1],"QUAD")==0)
        {
          free ( macro[0] );
          free ( macro[1] );
          macro[0] = strdup("LPQUAD");
          macro[1] = strdup("LPAQUAD");
          numregs = 2;
        }
      }
      else if( strcmp(word,"Function")==0 && in_archive && !in_function && !in_header )
      {
        num = get_words(line,&words);
        name = realloc( name, sizeof(char *) );
        name[0] = strdup(words[num-1]);
        type = realloc( type, sizeof(char *) );
        len = 0;
        for( i=1 ; i < num-1 ; i++ )
          len += (strlen(words[i])+1);
        type[0] = malloc( len * sizeof(char) );
        strcpy( type[0], words[1]);
        for( i=2 ; i < num-1 ; i++ )
        {
          strcat( type[0], " " );
          strcat( type[0], words[i] );
        }
        numparams = 0;
        in_function = 1;
      }
      else if( strcmp(word,"/Function")==0 && in_function && !in_autodoc && !in_code )
      {
        if( atoi(reg[0]) > firstlvo )
        {
          fprintf( fdo, "\nAROS_%s%d(%s, %s,\n", macro[0], numparams, type[0], name[0] );
          for( i = 1 ; i <= numparams ; i++ )
          {
            fprintf( fdo, "\tAROS_%s(%s, %s, %s),\n", macro[1], type[i], name[i], reg[i] );
          }
          fprintf( fdo, "\t%s, %s, %s, %s)\n", lc->libbasetypeptr, lc->libbase, reg[0], lc->basename );
        }
        for( i=0 ; i < numparams ; i++ )
        {
          free(name[i]);
          free(type[i]);
          free(reg[i]);
        }
        free(name);
        free(type);
        free(reg);
        name = NULL;
        type = NULL;
        reg = NULL;
        in_function = 0;
      }
      else if( strcmp(word,"Header")==0 && in_archive && !in_function && !in_header )
        in_header = 1;
      else if( strcmp(word,"/Header")==0 && in_header )
        in_header = 0;
      else if( strcmp(word,"LibOffset")==0 && in_function && !in_autodoc && !in_code )
      {
        num = get_words(line,&words);
        reg = realloc( reg, (numparams+1)*sizeof(char *) );
        reg[0] = strdup(words[1]);
      }
      else if( strcmp(word,"Parameter")==0 && in_function && !in_autodoc && !in_code )
      {
        numparams++;
        num = get_words( line, &words );
        name = realloc( name, (numparams+1) * sizeof(char *) );
        name[numparams] = strdup( words[num-2] );
        type = realloc( type, (numparams+1) * sizeof(char *) );
        len = 0;
        for( i=1 ; i < num-2 ; i++ )
          len += strlen(words[i]);
        type[numparams] = malloc( (len+num-3) * sizeof(char) );
        strcpy( type[numparams], words[1]);
        for( i=2 ; i < num-2 ; i++ )
        {
          strcat( type[numparams], " " );
          strcat( type[numparams], words[i] );
        }

	reg = realloc( reg, (numparams+1)*sizeof(char *) );
        reg[numparams] = malloc(4*numregs - 1);
	reg[numparams][4*numregs -1] = '\0';
	strncpy(reg[numparams], words[num-1], 2);
	reg[numparams][4*numregs -1] = 	reg[numparams][2] = '\0';
	for (i = 2 ; words[num-1][i]; )
	{
	    if (words[num-1][i++] == '/')
	    {
		strcat(reg[numparams], ", ");
		strncat(reg[numparams], &words[num-1][i], 2);
		i+=2;
		len += 4;
            }
	    else
	    {
	        fprintf(stderr, "Error: register name not known '%s'\n", &words[num-1][i]);
		exit(1);
            }
	}
        do
        {
          free ( line );
          line = get_line( fd );
        } while ( strncmp(line,"#/Parameter",11)!=0 );
      }
    }

    free(line);
  }
  fprintf( fdo, "\n#endif /* CLIB_%s_PROTOS_H */\n", upperbasename );
  fclose(fdo);
  fclose(fd);
  moveifchanged(filename,newname);
  free(lc);
  free(newname);
  free(filename);
  free(macro[0]);
  free(macro[1]);

return 0;
}

int genpragmas(int argc, char **argv)
{
FILE *fdo = NULL;
char *filename, *newname;
struct libconf *lc;

  if(argc != 2)
  {
    fprintf( stderr, "Usage: %s <incdir>\n", argv[0] );
    exit(-1);
  }
  if(!(lc = parse_libconf(NULL)) )
    return(-1);
  filename = malloc( (strlen(argv[1])+strlen(lc->libname)+20) * sizeof(char) );
  sprintf( filename, "%s/pragmas/%s_pragmas.h", argv[1], lc->libname );
  newname = malloc( (strlen(argv[1])+strlen(lc->libname)+24) * sizeof(char) );
  sprintf( newname, "%s/pragmas/%s_pragmas.h.new", argv[1], lc->libname );
  fdo = fopen(newname,"w");
  if(!fdo)
  {
    fprintf( stderr, "Couldn't open file %s!\n", newname );
    return(-1);
  }

  fprintf( fdo, "#include <clib/%s_protos.h>\n", lc->libname );
  fclose(fdo);
  moveifchanged(filename,newname);
  free(lc);
  free(newname);
  free(filename);

return 0;
}

const char * m68k_registers[] = {
	"D0",
	"D1",
	"D2",
	"D3",
	"D4",
	"D5",
	"D6",
	"D7",
	"A0",
	"A1",
	"A2",
	"A3",
	"A4",
	"A5",
	"A6",
	"A7",
};

struct pragma_description 
{
	struct pragma_description	*next;
	char				basename[200];
	char				funcname[200];
	unsigned short			offset;
	int				numargs;
	int				args[16];
};

char * skipstr(char * line, char skipper,int direction)
{
	while (line[0] == skipper) {
		line += direction;
	}
	return line;
}

void free_pragma_description(struct pragma_description * pd)
{
	struct pragma_description * _pd;
	while (pd != NULL) {
		_pd = pd -> next;
		free(pd);
		pd = _pd;
	}
	
}

struct pragma_description * find_pragma_description(struct pragma_description * pd,
                                                   char * funcname)
{
	while (NULL != pd) {
		if (0 == strcmp(pd->funcname,
		                funcname))
			return pd;
		pd = pd->next;
	}
	return NULL;
}

struct pragma_description * parse_pragmas(char * filename)
{
	FILE *fdi =NULL;
	char *line;
	struct pragma_description * base_pd = NULL;
	fdi = fopen(filename,"r");
	
        if (!fdi){
		fprintf(stderr, "Couldn't open file %s!\n",filename);
		return NULL;
	}
	
	while( (line = get_line(fdi))) {
		char * substr;
		//printf("%s\n",line);
		/*
		 * look for '#', then 'pragma'.
		 */
		substr = skipstr(line,' ',1);
		
		if (substr[0] == '#'){
			substr++;
			substr = skipstr(substr, ' ',1);
			//printf("1. %s\n",substr);
			if (0 == strncmp(substr, "pragma",6)) {
				substr += 6;
				substr = skipstr(substr,' ',1);
				//printf("2. %s\n",substr);
				if (!strncmp(substr, "amicall",7)) {
					substr += 7;
					substr = skipstr(substr,' ',1);
					
					if (substr[0] == '(') {
						struct pragma_description * pd = calloc(1,sizeof(struct pragma_description));
						substr += 1;
						substr = skipstr(substr,' ',1);
						if (NULL != pd) {
							char * comma;
							char * lastchar;
							pd->next = base_pd;
							base_pd = pd;
							/*
							 * Now read the name of the base
							 * of the library!
							 */
							comma = strchr(substr, ',');
							lastchar = skipstr(comma-1,' ',-1);
							
							strncpy(&pd->basename[0],
							        substr,
							        lastchar-substr+1);
							//printf("basename: %s\n",pd->basename);
							
							/*
							 * after the comma comes the offset in HEX!
							 */
							pd->offset = strtol(comma+1,&lastchar,16)/6;
							//printf("Offset : %x\n",pd->offset);
							
							/*
							 * Now for the next ','
							 */
							comma = strchr(lastchar, ',');
							comma = skipstr(comma+1,' ',1);
							//printf("%s\n",comma);
							
							if ( NULL == comma) {
								fprintf(stderr, "%d: Error parsing pragma file!\n",__LINE__);
								free_pragma_description(base_pd);
								fclose(fdi);
								exit(-1);
							}
							
							/*
							 * Now the name of the function
							 */
							lastchar = strchr(comma+1, '(');
							if ( NULL == lastchar) {
								fprintf(stderr, "%d: Error parsing pragma file!\n",__LINE__);
								free_pragma_description(base_pd);
								fclose(fdi);
								exit(-1);
							}
							
							
							strncpy(&pd->funcname[0],
							        comma,
							        skipstr(lastchar,' ',-1)-comma);
							//printf("funcname: %s\n",pd->funcname);
							        
							substr = lastchar + 1;
							//printf("%s\n",substr);
							lastchar = strchr(substr,')');
							/*
							 * Now read the CPU registers.
							 */
							
							while (substr < lastchar) {
								int r = 0;
								substr = skipstr(substr, ' ',1);
								
								if (substr[0] == 'a' ||
								    substr[0] == 'A') {
									r = 8;
								} else if (substr[0] == 'd' ||
								           substr[0] == 'D') {
									} else {
										fprintf(stderr, "Wrong register (letter) in pragma file!\n");
										free_pragma_description(base_pd);
										fclose(fdi);
										exit(-1);
									}
								
								if (substr[1] >= '0' && substr[1] <= '8') {
									r += substr[1] - '0';
								} else {
									fprintf(stderr, "Wrong register (number) in pragma file!\n");
									free_pragma_description(base_pd);
									fclose(fdi);
									exit(-1);
								}
/*
								printf("r:%d\n",r);
								printf("parameter %d goes into %s\n",
								        pd->numargs+1,
								        m68k_registers[r]);
*/
								pd->args[pd->numargs] = r;
								pd->numargs++;
								
								substr+=2;
								skipstr(substr, ' ', 1);
								substr+=1;
								
							}
							
						}
					}
				} else if (!strncmp(substr, "libcall",7)) {
					struct pragma_description * pd = calloc(1,sizeof(struct pragma_description));
					substr += 7;
					//substr = skipstr(substr,' ',1);
					if (NULL != pd) {
						char offset_s[10];
						char parameters_s[20];
						char hex[3]={0,0,0};
						int i,c;
						/*
						 * Read in the description of the
						 * function
						 */
						sscanf(substr, "%s %s %s %s",
						      &pd->basename[0],
						      &pd->funcname[0],
						      offset_s,
						      parameters_s);
						pd->offset = strtol(offset_s,NULL,16)/6;
#if 1
						printf("|%s| |%s| %d %s[%d]\t",
						       pd->basename,
						       pd->funcname,
						       pd->offset,
						       parameters_s,
						       strlen(parameters_s));
#endif	
						/*
						 * Process the parameters.
						 */
						i = strlen(parameters_s)-1;
						hex[0] = parameters_s[i-1];
						hex[1] = parameters_s[i];
						i -= 2;
						pd->numargs = strtol(hex,NULL,16);
						c = 0;
						hex[1] = 0;
						while (i >= 0) {
							hex[0] = parameters_s[i];
							pd->args[c] = strtol(hex,NULL,16);
							printf("%s ",m68k_registers[pd->args[c]]);
							i--;
							c++;
						}
						printf("\n");

						pd->next = base_pd;
						base_pd = pd;
					}
				} else if(!strncmp(substr, "usrcall",7)) {
					substr += 7;
					substr = skipstr(substr,' ',1);
					
					if (substr[0] == '(') {
						struct pragma_description * pd = calloc(1,sizeof(struct pragma_description));
						pd->offset = 0xffff; // sign for user function
						substr += 1;
						substr = skipstr(substr,' ',1);
						if (NULL != pd) {
							char * lastchar;
							pd->next = base_pd;
							base_pd = pd;
							/*
							 * Now the name of the function
							 */
printf("%s\n",substr);
							lastchar = strchr(substr+1, '(');
							if ( NULL == lastchar) {
								fprintf(stderr, "%d: Error parsing pragma file!\n",__LINE__);
								free_pragma_description(base_pd);
								fclose(fdi);
								exit(-1);
							}
							
							
							strncpy(&pd->funcname[0],
							        substr,
							        skipstr(lastchar,' ',-1)-substr);
							//printf("funcname: %s\n",pd->funcname);
							        
							substr = lastchar + 1;
							//printf("%s\n",substr);
							lastchar = strchr(substr,')');
							/*
							 * Now read the CPU registers.
							 */
							
							while (substr < lastchar) {
								int r = 0;
								substr = skipstr(substr, ' ',1);
								
								if (substr[0] == 'a' ||
								    substr[0] == 'A') {
									r = 8;
								} else if (substr[0] == 'd' ||
								           substr[0] == 'D') {
									} else {
										fprintf(stderr, "Wrong register (letter) in pragma file!\n");
										free_pragma_description(base_pd);
										fclose(fdi);
										exit(-1);
									}
								
								if (substr[1] >= '0' && substr[1] <= '8') {
									r += substr[1] - '0';
								} else {
									fprintf(stderr, "Wrong register (number) in pragma file!\n");
									free_pragma_description(base_pd);
									fclose(fdi);
									exit(-1);
								}
/*
								printf("r:%d\n",r);
								printf("parameter %d goes into %s\n",
								        pd->numargs+1,
								        m68k_registers[r]);
*/
								pd->args[pd->numargs] = r;
								pd->numargs++;
								
								substr+=2;
								skipstr(substr, ' ', 1);
								substr+=1;
								
							}
						}
					}
				}
			}
		}
		free(line);
	}
	

	fclose(fdi);
	return base_pd;
}


int countchars (char * s, char c)
{
	int ctr = 0;
	while (s[0] != 0) {
		if (s[0] == c)
			ctr++;
		s++;
	}
	return ctr;
}

/*
 * Add a line to the cache. If NULL is added the old cache
 * is flushed to the disk.
 * If a line is added everything that ended with a ';' up
 * to that point will be written into the file and the
 * cache will be cut down to everything that was not written
 * so far.
 * If a 'end-of-comment' is added also everything is flushed to output.
 */
char * addtocache(char * cache, char * newline, int len, FILE * fdo)
{
	int cachelen;
	char * semic;
	char * newcache = NULL;
	char * endcomment;

	if (NULL == newline) {
		if (NULL != cache) {
			fprintf(fdo, "%s\n", cache);
			free(cache);
		}
		return NULL;
	}
	
	
	semic = strchr(newline, ';');
	endcomment = strstr(newline, "*/");
	
	if (NULL != semic || NULL != endcomment) {
		int newlinelen = strlen(newline);
		int i = newlinelen -1;
		char * tmp;
		/*
		 * write the cache and everything up to the
		 * last ';' in the new line to the file.
		 */
		if (NULL != cache) {
			fprintf(fdo,"%s\n",cache);
//printf("1. Flush: |%s|\n",cache);
		}
		
		if (NULL != endcomment) {
			i = endcomment - newline + 1;
		} else {
			while (newline[i] != ';')
				i--;
		}
		
			
		tmp = malloc(i+2);
		memcpy(tmp,newline,i+1);
		tmp[i+1] = 0;
		fprintf(fdo, "%s",tmp);
//printf("2. Flush: |%s|\n",tmp);
		free(tmp);
		
		if (i < newlinelen) {
			newcache = malloc(newlinelen-i+1);
			memcpy(newcache, &newline[i+1], newlinelen-i);
			newcache[newlinelen-i] = 0;
		}
		free(cache);
		
	} else {
		/*
		 * ';' could not be found. Enhance the cache.
		 */
		cachelen = 0;
		if (NULL != cache)
			cachelen = strlen(cache);
			
		cachelen += strlen(newline)+1+1;
		newcache = malloc(cachelen);
		if (NULL != cache)
			sprintf(newcache,"%s\n%s",cache,newline);
		else
			sprintf(newcache,"%s",newline);
//printf("cache: %s\tnewcache: %s\n",cache,newcache);
		free (cache);
	}
	
	return newcache;
}

char * clear_cache(char * cache)
{
	free(cache);
	return NULL;
}

/*
 * cut out all '\n' '\t' and ' ' at the end and
 * beginning of a string
 */
void strtrim(char ** s)
{
	int end = strlen(*s)-1;
	int start = 0;
	char * newstr;
	int end_orig = end;
	
	while ((start < end) && (
	       '\t' == (*s)[start]  ||
	       '\n' == (*s)[start]  ||
	       ' '  == (*s)[start])) {
		start++;
	}

	while (end > 0 && (
	       '\t' == (*s)[end]  ||
	       '\n' == (*s)[end]  ||
	       ' '  == (*s)[end])) {
		end--;
	}

	if ((end > start) && ((start > 0) || (end_orig != end))) {
		newstr = malloc(end-start+2);
		strncpy(newstr, (*s)+start, end-start+1);
		newstr[end-start+1] = 0;
		free(*s);
		*s = newstr;
	}
}

char * getfuncname(char * s)
{
	int i = strlen(s)-1;
	int c = 0;
	char * name;
	int last;
	while (i > 0) {
		if (')' == s[i])
			c++;
		else if ('(' == s[i]) {
			c--;
			if (0 == c) {
				i--;
				while (i >= 0 && ' ' == s[i])
					i--;

				if (i == 0)
					return NULL;
					
				last = i;
				while (i > 0 && ' ' != s[i] && '\n' != s[i])
					i--;
				
				if (last == i)
					return NULL;
				
				if (s[i+1] == '*')
					i++;
				
				name = malloc(last-i+2);
				strncpy(name, &s[i+1], last-i);
				name[last-i]=0;
				strtrim(&name);
				return name;
			}
				
		}
		i--;
	}
	return NULL;
}

char * get_returntype(char * pattern, char * funcname)
{
	int len;
	int c = 0;
	char * ret_type;
	len = (int)strstr(pattern, funcname) - (int)pattern;
	
	ret_type = malloc(len-c+1);
	strncpy(ret_type, pattern+c, len-c);
	ret_type[len-c] = 0;

	strtrim(&ret_type);
	
	return ret_type; 
}

void get_argument(int num, int max, char * pattern, char ** type, char ** val)
{
	int i = 0;
	int _num = num;
	while (1) {
		if ('(' == pattern[i++]) {
			char * start;
			int c;
			int depth = 0;
			int other = 0;

			while (_num > 0) {
				if (',' == pattern[i])
					_num--;
				i++;
			}
			
			/* Start of nth argument. */ 
			start = &pattern[i];

//printf("start: %s\n",start);
			
			i = 0;
			while (1) {
				if (',' == start[i])
					break;
				else if ('(' == start[i])
					depth++;
				else if (')' == start[i])
					depth--;

				if (-1 == depth)
					break;

				i++;
			}
				
			i--;
//printf("end at %d\n",i);	
			/*
			 * Search for the parameter value backwards
			 */
			c = i;
			while (1) {
				if (1 == other && (' ' == start[c] || '\t' == start[c] || '\n' == start[c] || '*' == start[c]))
					break;
				
				if (' ' != start[c])
					other = 1;
				c--;
			}
			//c++;
//printf("variable at %d\n",c);			
			*type = malloc(c+2);
			strncpy(*type, start, c+1);
			(*type)[c+1] = 0;
			*val  = malloc(i-c+2);
			strncpy(*val , start+c+1, i-c);
			(*val)[i-c] = 0; 
//printf("|%s| |%s|\n",*type,*val);

			strtrim(type);
			strtrim(val);
			return;
		}
		
	}
}

/*
 * Rewrite a c function to AROS style c function
 */
int rewrite_function(FILE * fdo, 
                     char * pattern, 
                     struct pragma_description * pd, 
                     struct libconf * lc,
                     FILE * fdefines)
{
	char output[1024];
	int i = 0;
	char * ret_type;
	char * argtype, * argval;
	memset(&output[0],0,1024);
	
	printf("must pick ???? from info in: \n%s\n",pattern);
	ret_type = get_returntype(pattern,pd->funcname);


	if (0xffff != pd->offset) {
		fprintf(fdo,
		       "\nAROS_LH%d(%s, %s,\n",
		       pd->numargs,
		       ret_type,
		       pd->funcname);
	
	       
		while (i < pd->numargs) {
			get_argument(i, pd->numargs,pattern, &argtype, &argval);
			fprintf(fdo,
			        "    AROS_LHA(%s, %s, %s),\n",
			        argtype,
			        argval,
			        m68k_registers[pd->args[i]]);

			free(argtype);
			free(argval);
			i++;
		}  

		if (0 != pd->offset) {	
			fprintf(fdo,
			        "    %s, %s, %d, %s)\n{\n",
			        lc->libbasetypeptr,
			        lc->libbase,
			        pd->offset,
			        lc->basename);
		} else {
			fprintf(fdo,
			        "    struct ExecBase *, SysBase, 0, %s)\n{\n",
			        lc->basename);
		}

		/*
		 * Make the entry in the defines file. Only write
		 * those functions with offset > 4 (not init,open,close,
		 * expunge, null)
		 */
		if (pd->offset > 4) {
			fprintf(fdefines, "#ifndef %s\n#define %s(",
			        pd->funcname,
			        pd->funcname);
			
			i = 0;
		
			while (i < pd->numargs) {
				get_argument(i, pd->numargs,pattern, &argtype, &argval);
				fprintf(fdefines,
				        "%s",
				        argval);
				if (i < pd->numargs-1) {
					fprintf(fdefines,",");
				}
				free(argtype);
				free(argval);
		
				i++;
			}	
	
			fprintf(fdefines,") \\\n\tAROS_LC%d(%s,%s, \\\n",
			        pd->numargs,
			        ret_type,
			        pd->funcname);

			i = 0;
			while (i < pd->numargs) {
				get_argument(i, pd->numargs,pattern, &argtype, &argval);
				fprintf(fdefines,
				        "\tAROS_LCA(%s,%s,%s),\\\n",
				        argtype,
				        argval,
				        m68k_registers[pd->args[i]]);
				free(argtype);
				free(argval);
		
				i++;
			}	

			fprintf(fdefines,
			        "\t%s, %s, %d, %s)\n#endif\n\n",
			        lc->libbasetypeptr,
			        lc->libbase,
			        pd->offset,
			        lc->basename);

			free(ret_type);
		}
	} else {
		fprintf(fdo,
		       "\nAROS_UFH%d(%s, %s",
		       pd->numargs,
		       ret_type,
		       pd->funcname);
	
	       
		while (i < pd->numargs) {
			get_argument(i, pd->numargs,pattern, &argtype, &argval);
			strtrim(&argtype);
			strtrim(&argval);
			
			fprintf(fdo,
			        ",\n    AROS_UFHA(%s, %s, %s)",
			        argtype,
			        argval,
			        m68k_registers[pd->args[i]]);

			free(argtype);
			free(argval);
			i++;
		}  
		fprintf(fdo,")\n{\n");
	}
	return 0;
}

/*
 * Rewrite a whole c source code file according to the info provided
 * from a pragma file.
 */
int rewritecfile(FILE * fdi, 
                 FILE * fdo, 
                 struct pragma_description * pd, 
                 struct libconf * lc,
                 FILE * fdefines) 
{
	char * line;
	int depth = 0; // counting '{' and '}'
	char * cache = NULL;
	
	while (1) {
		line = get_line(fdi);
		if (NULL == line) {
			cache = addtocache(cache, NULL, 0, fdo);
			return 0;
		}
		
		if (0 == depth) {
			depth = countchars(line,'{');
			if (0 != depth) {
				char * funcname;
				/*
				 * A function begins in this line.
				 */
				
				cache = addtocache(cache,line,-1,fdo);
				
				//printf("\ncache where to find function: %s\n",cache);
				funcname = getfuncname(cache);
				if (funcname != NULL) {
					struct pragma_description * _pd;
					printf("funcname: %s",funcname);
					_pd = find_pragma_description(pd, funcname);
					if (_pd) {
						printf("-> Rewriting!\n");
						rewrite_function(fdo,cache,_pd,lc,fdefines);
						/*
						 * Do not throw the cache into the
						 * file but clear it
						 */
					 	cache = clear_cache(cache);
					} else
						printf("-> Not rewriting!\n");
					free(funcname);
				} else {
//printf("ADDING 1\n");
//					cache = addtocache(cache,line,-1,fdo);
				}
				
//printf("ADDING 2\n");
				cache = addtocache(cache,NULL,0,fdo);
			} else {
				char * substr;
				/*
				 * No function begins in this line.
				 * So let's collect it unless it is
				 * a comment ot starts with a '#'.
				 */
				substr = skipstr(line, ' ',1);

				if ('#' == substr[0] ||
				    NULL != strstr(line,"//") ) {
				    	cache = addtocache(cache,line,-1,fdo);
				    	cache = addtocache(cache,NULL,0,fdo);
					//printf("cache: %p\n",cache);
					//fprintf(fdo,"%s\n",line);
				} else {
					cache = addtocache(cache,line,-1,fdo);
				}
			}
			depth -= countchars(line,'}');
		} else {
			depth += countchars(line,'{');
			depth -= countchars(line,'}');
			fprintf(fdo,"%s\n",line);
		}
		free(line);
	}

	return 0;
}

FILE * create_definesfile(char * filename)
{
	FILE * fdo = fopen(filename, "r");
	
	/*
	 * If file existed position at end.
	 */
	if (fdo) {
		fclose(fdo);
		fdo = fopen(filename, "a+");
		return fdo;
	}
	
	/*
	 * File does not exist, so start it, if possible.
	 */
	fdo = fopen(filename, "w");
	if (NULL != fdo) {
		fprintf(fdo, "#include <aros/libcall.h>\n"
		             "#include <exec/types.h>\n\n");
		
	}
	
	return fdo;
}

/*
 * Generate AROS source code for a library from the description 
 * in a pragmas file.
 */
int genarossource(int argc, char **argv)
{
	FILE *fdo =NULL;
	FILE *fdi =NULL;
	FILE *fdefines = NULL;
	char * filename;
	char * sourcefile, * destfile;
	char * definesfile;
	struct libconf *lc;
	struct pragma_description * pd;

	
	if (argc !=5) {
		fprintf(stderr,"Usage: %s <source pragma file> <source c file> <dest c file> <output defines file>\n", argv[0]);
		exit(-1);
	}
	
	if(!(lc = parse_libconf(NULL)) )
	{
	  return(-1);
	}
	filename = malloc( (strlen(argv[1])+strlen(lc->libname)+20) * sizeof(char) );
	sprintf( filename, "%s", argv[1]);

	sourcefile = argv[2];
	destfile   = argv[3];
	definesfile = argv[4];
	
	pd = parse_pragmas(filename);

	if (NULL == pd) {
		fprintf(stderr, "Could not read in the pragmas!\n");
		exit(-1);
	}	

	/*
	 * Now open and parse the C input file and generate an output.
	 */
	fdi = fopen(sourcefile, "r");
	fdo = fopen(destfile, "w");
	fdefines = create_definesfile(definesfile);

	if (NULL != fdi && NULL != fdo && NULL != fdefines)
		rewritecfile(fdi,fdo,pd,lc,fdefines);

	if (NULL != fdi)
		fclose(fdi);
	if (NULL != fdo)
		fclose(fdo);
	if (NULL != fdefines)
		fclose(fdefines);

	free_pragma_description(pd);
	
	free(lc);        
	free(filename);
	return 0;
}


int genproto(int argc, char **argv)
{
FILE *fdo = NULL;
char *filename, *newname;
struct libconf *lc;
char *upperbasename;

  if(argc != 2)
  {
    fprintf( stderr, "Usage: %s <incdir>\n", argv[0] );
    exit(-1);
  }
  if(!(lc = parse_libconf(NULL)) )
    return(-1);
  if(!(lc = parse_libconf(NULL)) )
    return(-1);
  filename = malloc( (strlen(argv[1])+strlen(lc->libname)+10) * sizeof(char) );
  sprintf( filename, "%s/proto/%s.h", argv[1], lc->libname );
  newname = malloc( (strlen(argv[1])+strlen(lc->libname)+14) * sizeof(char) );
  sprintf( newname, "%s/proto/%s.h.new", argv[1], lc->libname );
  fdo = fopen(newname,"w");
  if(!fdo)
  {
    fprintf( stderr, "Couldn't open file %s!\n", newname );
    return(-1);
  }

  upperbasename = strdup( lc->basename );
  strupper( upperbasename );
  fprintf( fdo, "#ifndef PROTO_%s_H\n", upperbasename );
  fprintf( fdo, "#define PROTO_%s_H\n\n", upperbasename );
  fprintf( fdo, "/*\n" );
  fprintf( fdo, "    Copyright (C) 1995-1998 AROS - The Amiga Research OS\n" );
  fprintf( fdo, "    *** Automatic generated file. Do not edit ***\n" );
  fprintf( fdo, "    Lang: english\n" );
  fprintf( fdo, "*/\n\n" );
  fprintf( fdo, "#ifndef AROS_SYSTEM_H\n" );
  fprintf( fdo, "#   include <aros/system.h>\n" );
  fprintf( fdo, "#endif\n\n" );
  fprintf( fdo, "#include <clib/%s_protos.h>\n\n", lc->libname );
  fprintf( fdo, "#include <defines/%s.h>\n\n", lc->libname );
  if(lc->option & o_hasrt)
  {
    fprintf( fdo, "#if defined(ENABLE_RT) && ENABLE_RT && !defined(ENABLE_RT_%s)\n", upperbasename );
    fprintf( fdo, "#   define ENABLE_RT_%s 1\n", upperbasename );
    fprintf( fdo, "#   include <aros/rt.h>\n" );
    fprintf( fdo, "#endif\n\n" );
  }
  fprintf( fdo, "#endif /* PROTO_%s_H */\n", upperbasename );
  fclose(fdo);
  moveifchanged(filename,newname);
  free(lc);
  free(newname);
  free(filename);

return 0;
}

int geninline(int argc, char **argv)
{
FILE *fd, *fdo = NULL, *headerstempl;
char *filename, *newname;
struct libconf *lc;
char *upperbasename;
char *line = 0;
char *word = NULL, **words = NULL;
int in_archive, in_header, in_function, in_autodoc, in_code;
int num, i, len;
char **name = NULL, **type = NULL, **reg = NULL;
int numregs = 1;
int numparams=0;
int firstlvo;

  if(argc != 3)
  {
    fprintf( stderr, "Usage: %s <incdir> <archfile>\n", argv[0] );
    exit(-1);
  }
  if(!(lc = parse_libconf(NULL)) )
    return(-1);
  fd = fopen(argv[2],"rb");
  if(!fd)
  {
    fprintf( stderr, "Couldn't open file %s!\n", argv[2] );
    return(-1);
  }
  filename = malloc( (strlen(argv[1])+strlen(lc->libname)+11) * sizeof(char) );
  sprintf( filename, "%s/inline/%s.h", argv[1], lc->libname );
  newname = malloc( (strlen(argv[1])+strlen(lc->libname)+15) * sizeof(char) );
  sprintf( newname, "%s/inline/%s.h.new", argv[1], lc->libname );
  fdo = fopen(newname,"w");
  if(!fdo)
  {
    fprintf( stderr, "Couldn't open file %s!\n", newname );
    return(-1);
  }

  upperbasename = strdup( lc->basename );
  strupper( upperbasename );
  fprintf( fdo, "#ifndef _INLINE_%s_H\n", upperbasename );
  fprintf( fdo, "#define _INLINE_%s_H\n\n", upperbasename );
  fprintf( fdo, "/*\n" );
  fprintf( fdo, "    Copyright (C) 1995-1998 AROS - The Amiga Research OS\n" );
  fprintf( fdo, "    *** Automatic generated file. Do not edit ***\n" );
  fprintf( fdo, "    Desc: Inlines for %s.", lc->basename );
  switch( lc->type )
  {
    case t_resource:
      fprintf( fdo, "resource" );
      firstlvo = 0;
      break;
    case t_device:
      fprintf( fdo, "device" );
      firstlvo = 6;
      break;
    case t_hidd:
      fprintf( fdo, "hidd" );
      firstlvo = 4;
      break;
    case t_library:
    default:
      fprintf( fdo, "library" );
      firstlvo = 4;
      break;
  }
  fprintf( fdo, "\n    Lang: english\n" );
  fprintf( fdo, "*/\n\n" );
  fprintf( fdo, "#ifndef __INLINE_MACROS_H\n" );
  fprintf( fdo, "#   include <inline/macros.h>\n" );
  fprintf( fdo, "#endif\n\n" );
  fprintf( fdo, "#ifndef %s_BASE_NAME\n", upperbasename );
  fprintf( fdo, "#define %s_BASE_NAME %s\n", upperbasename, lc->libbase );
  fprintf( fdo, "#endif\n\n" );
  headerstempl = fopen( "headers.tmpl", "rb" );
  if( headerstempl )
  {
    in_header = 0;
    while( (line = get_line(headerstempl)) )
    {
      num = get_words(line,&words);
      if( num == 2 && strcmp(words[0],"##begin")==0 && strcmp(words[1],"clib")==0 )
      {
        in_header = 1;
      }
      else if( num == 2 && strcmp(words[0],"##end")==0 && strcmp(words[1],"clib")==0 )
      {
        in_header = 0;
      }
      else if( in_header )
      {
        fprintf( fdo, "%s\n", line );
      }
      free(line);
    }
    fclose(headerstempl);
    fprintf( fdo, "\n" );
  }
  fprintf( fdo, "\n/* Prototypes */\n" );

  in_archive = 0;
  in_header = 0;
  in_function = 0;
  in_autodoc = 0;
  in_code = 0;
  while( (line = get_line(fd)) )
  {
    free(word);
    word = keyword(line);
    if( word && (isupper(word[0]) || isupper(word[1])) )
    {
      if( strcmp(word,"Archive")==0 && !in_archive )
        in_archive = 1;
      else if( strcmp(word,"/Archive")==0 && in_archive && !in_header && ! in_function )
        break;
      else if( strcmp(word,"AutoDoc")==0 && in_function && !in_code && !in_autodoc )
        in_autodoc = 1;
      else if( strcmp(word,"/AutoDoc")==0 && in_autodoc )
        in_autodoc = 0;
      else if( strcmp(word,"Code")==0 && in_function && !in_code && !in_autodoc )
        in_code = 1;
      else if( strcmp(word,"/Code")==0 && in_code )
        in_code = 0;
      else if( strcmp(word,"Options")==0 && in_function && !in_code && !in_autodoc )
      {
        num = get_words ( line, &words );
        if(strcmp(words[1],"QUAD")==0)
        {
          numregs = 2;
        }
      }
      else if( strcmp(word,"Function")==0 && in_archive && !in_function && !in_header )
      {
        num = get_words(line,&words);
        name = realloc( name, sizeof(char *) );
        name[0] = strdup(words[num-1]);
        type = realloc( type, sizeof(char *) );
        len = 0;
        for( i=1 ; i < num-1 ; i++ )
          len += (strlen(words[i])+1);
        type[0] = malloc( len * sizeof(char) );
        strcpy( type[0], words[1]);
        for( i=2 ; i < num-1 ; i++ )
        {
          strcat( type[0], " " );
          strcat( type[0], words[i] );
        }
        numparams = 0;
        in_function = 1;
      }
      else if( strcmp(word,"/Function")==0 && in_function && !in_autodoc && !in_code )
      {
        if( atoi(reg[0]) > firstlvo )
        {
          fprintf( fdo, "\n#define %s(", name[0] );
          for( i = 1 ; i <= numparams ; i++ )
          {
            if( i!=1 )
              fprintf( fdo, ", " );
            fprintf( fdo, "%s", name[i] );
          }
          if(strcasecmp(type[0],"void")==0)
            fprintf( fdo, ") \\\n\tLP%dNR(0x%x, %s, ", numparams, atoi(reg[0])*6, name[0] );
          else
            fprintf( fdo, ") \\\n\tLP%d(0x%x, %s, %s, ", numparams, atoi(reg[0])*6, type[0], name[0] );
          for( i = 1 ; i <= numparams ; i++ )
            fprintf( fdo, "%s, %s, %s, ", type[i], name[i], reg[i] );
          fprintf( fdo, "\\\n\t, %s_BASE_NAME)\n", upperbasename );
          if( numparams!=0 )
          {
          char *tagsname;
            num = get_words(type[numparams],&words);
            if(num == 3 && strcmp(words[0],"struct")==0 && strcmp(words[1],"TagItem")==0 && strcmp(words[2],"*")==0 )
            {
              len = strlen(name[0]);
              if(name[0][len-1]=='A')
              {
                tagsname = strdup(name[0]);
                tagsname[len-1] = 0;
              }
              else
              {
                tagsname = malloc( (len+5) * sizeof(char) );
                sprintf( tagsname, "%sTags", name[0] );
              }
              fprintf( fdo, "\n#ifndef NO_INLINE_STDARG\n#define %s(", tagsname );
              for( i=1 ; i<numparams ; i++ )
                fprintf( fdo, "%s, ", reg[i] );
              fprintf( fdo, "tags...) \\\n\t({ULONG _tags[] = { tags }; %s(", name[0] );
              for( i=1 ; i<numparams ; i++ )
                fprintf( fdo, "(%s), ", reg[i] );
              fprintf( fdo, "(struct TagItem *)_tags);})\n" );
              fprintf( fdo, "#endif /* !NO_INLINE_STDARG */\n\n" );
              free(tagsname);
            }
          }
        }
        for( i=0 ; i < numparams ; i++ )
        {
          free(name[i]);
          free(type[i]);
          free(reg[i]);
        }
        free(name);
        free(type);
        free(reg);
        name = NULL;
        type = NULL;
        reg = NULL;
        in_function = 0;
      }
      else if( strcmp(word,"Header")==0 && in_archive && !in_function && !in_header )
        in_header = 1;
      else if( strcmp(word,"/Header")==0 && in_header )
        in_header = 0;
      else if( strcmp(word,"LibOffset")==0 && in_function && !in_autodoc && !in_code )
      {
        num = get_words(line,&words);
        reg = realloc( reg, (numparams+1)*sizeof(char *) );
        reg[0] = strdup(words[1]);
      }
      else if( strcmp(word,"Parameter")==0 && in_function && !in_autodoc && !in_code )
      {
        numparams++;
        num = get_words( line, &words );
        name = realloc( name, (numparams+1) * sizeof(char *) );
        name[numparams] = strdup( words[num-2] );
        type = realloc( type, (numparams+1) * sizeof(char *) );
        len = 0;
        for( i=1 ; i < num-2 ; i++ )
          len += strlen(words[i]);
        type[numparams] = malloc( (len+num-3) * sizeof(char) );
        strcpy( type[numparams], words[1]);
        for( i=2 ; i < num-2 ; i++ )
        {
          strcat( type[numparams], " " );
          strcat( type[numparams], words[i] );
        }

	reg = realloc( reg, (numparams+1)*sizeof(char *) );
        reg[numparams] = malloc(4*numregs - 1);
	reg[numparams][4*numregs -1] = '\0';
	strncpy(reg[numparams], words[num-1], 2);
	reg[numparams][4*numregs -1] = 	reg[numparams][2] = '\0';
	for (i = 2 ; words[num-1][i]; )
	{
	    if (words[num-1][i++] == '/')
	    {
		strcat(reg[numparams], ", ");
		strncat(reg[numparams], &words[num-1][i], 2);
		i+=2;
		len += 4;
            }
	    else
	    {
	        fprintf(stderr, "Error: register name not known '%s'\n", &words[num-1][i]);
		exit(1);
            }
	}
        strlower(reg[numparams]);
        do
        {
          free ( line );
          line = get_line( fd );
        } while ( strncmp(line,"#/Parameter",11)!=0 );
      }
    }

    free(line);
  }
  fprintf( fdo, "\n#endif /* _INLINE_%s_H */\n", upperbasename );
  fclose(fdo);
  fclose(fd);
  moveifchanged(filename,newname);
  free(lc);
  free(newname);
  free(filename);

return 0;
}

/* Generate clib/ defines/ proto/ inline/ pragmas/ */
#define NUM_INCLUDES 5
int genincludes(int argc, char **argv)
{
FILE *fd, *fdo[NUM_INCLUDES], *headerstempl;
char *filename[NUM_INCLUDES], *newname[NUM_INCLUDES];
struct libconf *lc;
char *upperbasename;
char *line = 0;
char *word = NULL, **words = NULL;
int in_archive, in_header, in_function, in_autodoc, in_code;
int num, i, len;
char **name = NULL, **type = NULL, **reg = NULL;
int numregs = 1;
char *macro[4];
int numparams=0;
int firstlvo;

  if(argc != 3)
  {
    fprintf( stderr, "Usage: %s <incdir> <archfile>\n", argv[0] );
    exit(-1);
  }
  if(!(lc = parse_libconf(NULL)) )
    return(-1);
  fd = fopen(argv[2],"rb");
  if(!fd)
  {
    fprintf( stderr, "Couldn't open file %s!\n", argv[2] );
    return(-1);
  }
  filename[0] = malloc( (strlen(argv[1])+strlen(lc->libname)+16) * sizeof(char) );
  sprintf( filename[0], "%s/clib/%s_protos.h", argv[1], lc->libname );
  newname[0] = malloc( (strlen(argv[1])+strlen(lc->libname)+20) * sizeof(char) );
  sprintf( newname[0], "%s/clib/%s_protos.h.new", argv[1], lc->libname );

  filename[1] = malloc( (strlen(argv[1])+strlen(lc->libname)+12) * sizeof(char) );
  sprintf( filename[1], "%s/defines/%s.h", argv[1], lc->libname );
  newname[1] = malloc( (strlen(argv[1])+strlen(lc->libname)+16) * sizeof(char) );
  sprintf( newname[1], "%s/defines/%s.h.new", argv[1], lc->libname );

  filename[2] = malloc( (strlen(argv[1])+strlen(lc->libname)+10) * sizeof(char) );
  sprintf( filename[2], "%s/proto/%s.h", argv[1], lc->libname );
  newname[2] = malloc( (strlen(argv[1])+strlen(lc->libname)+14) * sizeof(char) );
  sprintf( newname[2], "%s/proto/%s.h.new", argv[1], lc->libname );

  filename[3] = malloc( (strlen(argv[1])+strlen(lc->libname)+11) * sizeof(char) );
  sprintf( filename[3], "%s/inline/%s.h", argv[1], lc->libname );
  newname[3] = malloc( (strlen(argv[1])+strlen(lc->libname)+15) * sizeof(char) );
  sprintf( newname[3], "%s/inline/%s.h.new", argv[1], lc->libname );

  filename[4] = malloc( (strlen(argv[1])+strlen(lc->libname)+20) * sizeof(char) );
  sprintf( filename[4], "%s/pragmas/%s_pragmas.h", argv[1], lc->libname );
  newname[4] = malloc( (strlen(argv[1])+strlen(lc->libname)+24) * sizeof(char) );
  sprintf( newname[4], "%s/pragmas/%s_pragmas.h.new", argv[1], lc->libname );

  for( i=0 ; i<NUM_INCLUDES ; i++ )
  {
    fdo[i] = fopen(newname[i],"w");
    if(!fdo[i])
    {
      fprintf( stderr, "Couldn't open file %s!\n", newname[i] );
      for( i-- ; i>0 ; i-- )
        fclose(fdo[i]);
      return(-1);
    }
  }

  upperbasename = strdup( lc->basename );
  strupper( upperbasename );
  fprintf( fdo[0], "#ifndef CLIB_%s_PROTOS_H\n", upperbasename );
  fprintf( fdo[0], "#define CLIB_%s_PROTOS_H\n\n", upperbasename );
  fprintf( fdo[0], "/*\n" );
  fprintf( fdo[0], "    Copyright (C) 1995-1998 AROS - The Amiga Research OS\n" );
  fprintf( fdo[0], "    *** Automatic generated file. Do not edit ***\n" );
  fprintf( fdo[0], "    Desc: Prototypes for %s.", lc->basename );
  fprintf( fdo[1], "#ifndef DEFINES_%s_PROTOS_H\n", upperbasename );
  fprintf( fdo[1], "#define DEFINES_%s_PROTOS_H\n\n", upperbasename );
  fprintf( fdo[1], "/*\n" );
  fprintf( fdo[1], "    Copyright (C) 1995-1998 AROS - The Amiga Research OS\n" );
  fprintf( fdo[1], "    *** Automatic generated file. Do not edit ***\n" );
  fprintf( fdo[1], "    Desc: Prototypes for %s.", lc->basename );
  fprintf( fdo[2], "#ifndef PROTO_%s_H\n", upperbasename );
  fprintf( fdo[2], "#define PROTO_%s_H\n\n", upperbasename );
  fprintf( fdo[2], "/*\n" );
  fprintf( fdo[2], "    Copyright (C) 1995-1998 AROS - The Amiga Research OS\n" );
  fprintf( fdo[2], "    *** Automatic generated file. Do not edit ***\n" );
  fprintf( fdo[2], "    Lang: english\n" );
  fprintf( fdo[2], "*/\n\n" );
  fprintf( fdo[2], "#ifndef AROS_SYSTEM_H\n" );
  fprintf( fdo[2], "#   include <aros/system.h>\n" );
  fprintf( fdo[2], "#endif\n\n" );
  fprintf( fdo[2], "#include <clib/%s_protos.h>\n\n", lc->libname );
  fprintf( fdo[2], "#include <defines/%s.h>\n\n", lc->libname );
  if(lc->option & o_hasrt)
  {
    fprintf( fdo[2], "#if defined(ENABLE_RT) && ENABLE_RT && !defined(ENABLE_RT_%s)\n", upperbasename );
    fprintf( fdo[2], "#   define ENABLE_RT_%s 1\n", upperbasename );
    fprintf( fdo[2], "#   include <aros/rt.h>\n" );
    fprintf( fdo[2], "#endif\n\n" );
  }
  fprintf( fdo[3], "#ifndef _INLINE_%s_H\n", upperbasename );
  fprintf( fdo[3], "#define _INLINE_%s_H\n\n", upperbasename );
  fprintf( fdo[3], "/*\n" );
  fprintf( fdo[3], "    Copyright (C) 1995-1998 AROS - The Amiga Research OS\n" );
  fprintf( fdo[3], "    *** Automatic generated file. Do not edit ***\n" );
  fprintf( fdo[3], "    Desc: Inlines for %s.", lc->basename );
  fprintf( fdo[4], "#include <clib/%s_protos.h>\n", lc->libname );
  switch( lc->type )
  {
    case t_resource:
      fprintf( fdo[0], "resource" );
      fprintf( fdo[1], "resource" );
      fprintf( fdo[3], "resource" );
      firstlvo = 0;
      break;
    case t_device:
      fprintf( fdo[0], "device" );
      fprintf( fdo[1], "device" );
      fprintf( fdo[3], "device" );
      firstlvo = 6;
      break;
    case t_hidd:
      fprintf( fdo[0], "hidd" );
      fprintf( fdo[1], "hidd" );
      fprintf( fdo[3], "hidd" );
      firstlvo = 4;
      break;
    case t_library:
    default:
      fprintf( fdo[0], "library" );
      fprintf( fdo[1], "library" );
      fprintf( fdo[3], "library" );
      firstlvo = 4;
      break;
  }
  fprintf( fdo[0], "\n    Lang: english\n" );
  fprintf( fdo[0], "*/\n\n" );
  fprintf( fdo[0], "#ifndef AROS_LIBCALL_H\n" );
  fprintf( fdo[0], "#   include <aros/libcall.h>\n" );
  fprintf( fdo[0], "#endif\n\n" );
  fprintf( fdo[1], "\n    Lang: english\n" );
  fprintf( fdo[1], "*/\n\n" );
  fprintf( fdo[1], "#ifndef AROS_LIBCALL_H\n" );
  fprintf( fdo[1], "#   include <aros/libcall.h>\n" );
  fprintf( fdo[1], "#endif\n" );
  fprintf( fdo[1], "#ifndef EXEC_TYPES_H\n" );
  fprintf( fdo[1], "#   include <exec/types.h>\n" );
  fprintf( fdo[1], "#endif\n\n" );
  fprintf( fdo[3], "\n    Lang: english\n" );
  fprintf( fdo[3], "*/\n\n" );
  fprintf( fdo[3], "#ifndef __INLINE_MACROS_H\n" );
  fprintf( fdo[3], "#   include <inline/macros.h>\n" );
  fprintf( fdo[3], "#endif\n\n" );
  fprintf( fdo[3], "#ifndef %s_BASE_NAME\n", upperbasename );
  fprintf( fdo[3], "#define %s_BASE_NAME %s\n", upperbasename, lc->libbase );
  fprintf( fdo[3], "#endif\n\n" );
  headerstempl = fopen( "headers.tmpl", "rb" );
  if( headerstempl )
  {
    in_header = 0;
    while( (line = get_line(headerstempl)) )
    {
      num = get_words(line,&words);
      if( num == 2 && strcmp(words[0],"##begin")==0 )
      {
        if( strcmp(words[1],"clib")==0 )
          in_header = 1;
        else if( strcmp(words[1],"defines")==0 )
          in_header = 2;
      }
      else if( num == 2 && strcmp(words[0],"##end")==0 )
      {
        in_header = 0;
      }
      else if( in_header )
      {
        if( in_header == 1 )
        {
          fprintf( fdo[0], "%s\n", line );
          fprintf( fdo[3], "%s\n", line );
        }
        else if( in_header == 2 )
          fprintf( fdo[1], "%s\n", line );
      }
      free(line);
    }
    fclose(headerstempl);
    fprintf( fdo[0], "\n" );
    fprintf( fdo[1], "\n" );
    fprintf( fdo[3], "\n" );
  }
  fprintf( fdo[0], "\n/* Prototypes */\n" );
  fprintf( fdo[1], "\n/* Defines */\n" );
  fprintf( fdo[3], "\n/* Prototypes */\n" );
  
  macro[0] = strdup("LP");
  macro[1] = strdup("LPA");
  macro[2] = strdup("LC");
  macro[3] = strdup("LCA");
  in_archive = 0;
  in_header = 0;
  in_function = 0;
  in_autodoc = 0;
  in_code = 0;
  while( (line = get_line(fd)) )
  {
    free(word);
    word = keyword(line);
    if( word && (isupper(word[0]) || isupper(word[1])) )
    {
      if( strcmp(word,"Archive")==0 && !in_archive )
        in_archive = 1;
      else if( strcmp(word,"/Archive")==0 && in_archive && !in_header && ! in_function )
        break;
      else if( strcmp(word,"AutoDoc")==0 && in_function && !in_code && !in_autodoc )
        in_autodoc = 1;
      else if( strcmp(word,"/AutoDoc")==0 && in_autodoc )
        in_autodoc = 0;
      else if( strcmp(word,"Code")==0 && in_function && !in_code && !in_autodoc )
        in_code = 1;
      else if( strcmp(word,"/Code")==0 && in_code )
        in_code = 0;
      else if( strcmp(word,"Options")==0 && in_function && !in_code && !in_autodoc )
      {
        num = get_words ( line, &words );
        if(strcmp(words[1],"QUAD")==0)
        {
          free ( macro[0] );
          free ( macro[1] );
          free ( macro[2] );
          free ( macro[3] );
          macro[0] = strdup("LPQUAD");
          macro[1] = strdup("LPAQUAD");
          macro[2] = strdup("LCQUAD");
          macro[3] = strdup("LCAQUAD");
          numregs = 2;
        }
      }
      else if( strcmp(word,"Function")==0 && in_archive && !in_function && !in_header )
      {
        num = get_words ( line, &words );
        name = realloc( name, sizeof(char *) );
        name[0] = strdup(words[num-1]);
        type = realloc( type, sizeof(char *) );
        len = 0;
        for( i=1 ; i < num-1 ; i++ )
          len += (strlen(words[i])+1);
        type[0] = malloc( len * sizeof(char) );
        strcpy( type[0], words[1]);
        for( i=2 ; i < num-1 ; i++ )
        {
          strcat( type[0], " " );
          strcat( type[0], words[i] );
        }
        numparams = 0;
        in_function = 1;
      }
      else if( strcmp(word,"/Function")==0 && in_function && !in_autodoc && !in_code )
      {
        if( atoi(reg[0]) > firstlvo )
        {
          fprintf( fdo[0], "\nAROS_%s%d(%s, %s,\n", macro[0], numparams, type[0], name[0] );
          fprintf( fdo[1], "\n#define %s(", name[0] );
          fprintf( fdo[3], "\n#define %s(", name[0] );
          for( i = 1 ; i <= numparams ; i++ )
          {
            if( i!=1 )
            {
              fprintf( fdo[1], ", " );
              fprintf( fdo[3], ", " );
            }
            fprintf( fdo[1], "%s", name[i] );
            fprintf( fdo[3], "%s", name[i] );
          }
          fprintf( fdo[1], ") \\\n\tAROS_%s%d(%s, %s, \\\n", macro[2], numparams, type[0], name[0] );
          if(strcasecmp(type[0],"void")==0)
            fprintf( fdo[3], ") \\\n\tLP%dNR(0x%x, %s, ", numparams, atoi(reg[0])*6, name[0] );
          else
            fprintf( fdo[3], ") \\\n\tLP%d(0x%x, %s, %s, ", numparams, atoi(reg[0])*6, type[0], name[0] );
          for( i = 1 ; i <= numparams ; i++ )
          {
            fprintf( fdo[0], "\tAROS_%s(%s, %s, %s),\n", macro[1], type[i], name[i], reg[i] );
            fprintf( fdo[1], "\tAROS_%s(%s, %s, %s), \\\n", macro[3], type[i], name[i], reg[i] );
            strlower(reg[i]);
            fprintf( fdo[3], "%s, %s, %s, ", type[i], name[i], reg[i] );
          }
          fprintf( fdo[0], "\t%s, %s, %s, %s)\n", lc->libbasetypeptr, lc->libbase, reg[0], lc->basename );
          fprintf( fdo[1], "\t%s, %s, %s, %s)\n", lc->libbasetypeptr, lc->libbase, reg[0], lc->basename );
          fprintf( fdo[3], "\\\n\t, %s_BASE_NAME)\n", upperbasename );

          if( numparams!=0 )
          {
          char *tagsname;
            num = get_words(type[numparams],&words);
            if(num == 3 && strcmp(words[0],"struct")==0 && strcmp(words[1],"TagItem")==0 && strcmp(words[2],"*")==0 )
            {
              len = strlen(name[0]);
              if(name[0][len-1]=='A')
              {
                tagsname = strdup(name[0]);
                tagsname[len-1] = 0;
              }
              else
              {
                tagsname = malloc( (len+5) * sizeof(char) );
                sprintf( tagsname, "%sTags", name[0] );
              }
              fprintf( fdo[3], "\n#ifndef NO_INLINE_STDARG\n#define %s(", tagsname );
              for( i=1 ; i<numparams ; i++ )
                fprintf( fdo[3], "%s, ", reg[i] );
              fprintf( fdo[3], "tags...) \\\n\t({ULONG _tags[] = { tags }; %s(", name[0] );
              for( i=1 ; i<numparams ; i++ )
                fprintf( fdo[3], "(%s), ", reg[i] );
              fprintf( fdo[3], "(struct TagItem *)_tags);})\n" );
              fprintf( fdo[3], "#endif /* !NO_INLINE_STDARG */\n\n" );
              free(tagsname);
            }
          }
        }
        in_function = 0;
        for( i=0 ; i < numparams ; i++ )
        {
          free(name[i]);
          free(type[i]);
          free(reg[i]);
        }
        free(name);
        free(type);
        free(reg);
        name = NULL;
        type = NULL;
        reg = NULL;
      }
      else if( strcmp(word,"Header")==0 && in_archive && !in_function && !in_header )
        in_header = 1;
      else if( strcmp(word,"/Header")==0 && in_header )
        in_header = 0;
      else if( strcmp(word,"LibOffset")==0 && in_function && !in_autodoc && !in_code )
      {
        num = get_words(line,&words);
        reg = realloc( reg, (numparams+1)*sizeof(char *) );
        reg[0] = strdup(words[1]);
      }
      else if( strcmp(word,"Parameter")==0 && in_function && !in_autodoc && !in_code )
      {
        numparams++;
        num = get_words( line, &words );
        name = realloc( name, (numparams+1) * sizeof(char *) );
        name[numparams] = strdup( words[num-2] );
        type = realloc( type, (numparams+1) * sizeof(char *) );
        len = 0;
        for( i=1 ; i < num-2 ; i++ )
          len += strlen(words[i]);
        type[numparams] = malloc( (len+num-3) * sizeof(char) );
        strcpy( type[numparams], words[1]);
        for( i=2 ; i < num-2 ; i++ )
        {
          strcat( type[numparams], " " );
          strcat( type[numparams], words[i] );
        }

	reg = realloc( reg, (numparams+1)*sizeof(char *) );
        reg[numparams] = malloc(4*numregs - 1);
	reg[numparams][4*numregs -1] = '\0';
	strncpy(reg[numparams], words[num-1], 2);
	reg[numparams][4*numregs -1] = 	reg[numparams][2] = '\0';
	for (i = 2 ; words[num-1][i]; )
	{
	    if (words[num-1][i++] == '/')
	    {
		strcat(reg[numparams], ", ");
		strncat(reg[numparams], &words[num-1][i], 2);
		i+=2;
		len += 4;
            }
	    else
	    {
	        fprintf(stderr, "Error: register name not known '%s'\n", &words[num-1][i]);
		exit(1);
            }
	}

	do
        {
          free ( line );
          line = get_line( fd );
        } while ( strncmp(line,"#/Parameter",11)!=0 );
      }
    }

    free(line);
  }
  fprintf( fdo[0], "\n#endif /* CLIB_%s_PROTOS_H */\n", upperbasename );
  fprintf( fdo[1], "\n#endif /* DEFINES_%s_PROTOS_H */\n", upperbasename );
  fprintf( fdo[2], "#endif /* PROTO_%s_H */\n", upperbasename );
  fprintf( fdo[3], "\n#endif /* _INLINE_%s_H */\n", upperbasename );
  fclose(fd);
  for(i=0;i<NUM_INCLUDES;i++)
  {
    fclose(fdo[i]);
    moveifchanged(filename[i],newname[i]);
    free(newname[i]);
    free(filename[i]);
  }
  free(lc);
  for(i=0;i<4;i++)
    free(macro[i]);

return 0;
}


int main(int argc, char **argv)
{
int retval = 0;
char option;

  if( argc < 2 )
  {
    fprintf( stderr, "Usage: %s [-h|-e|-a|-t|-s|-m|-M|-c|-d|-C|-p|-i|-r|-I] <parameter>\n", argv[0] );
    fprintf( stderr, "  -h help\n  -e extractfiles\n  -a genautodocs\n  -t genfunctable\n  -s gensource\n  -m mergearch\n  -M mergearchs\n  -c genlibdefs\n  -d gendefines\n  -C genclib\n  -p genproto\n  -i geninline\n  -r genpragmas\n  -I genincludes\n  -R genarossource\n" );
    exit(-1);
  }

  if( argv[1][0] == '-' )
  {
    argc--;
    option = argv[1][1];
    argv[1] = malloc( (strlen(argv[0])+4) * sizeof(char) );
    sprintf( argv[1], "%s -%c", argv[0], option );
    switch( option )
    {
      case 'e':
        retval = extractfiles( argc, &argv[1] );
        break;
      case 'a':
        retval = genautodocs( argc, &argv[1] );
        break;
      case 't':
        retval = genfunctable( argc, &argv[1] );
        break;
      case 's':
        retval = gensource( argc, &argv[1] );
        break;
      case 'm':
        retval = mergearch( argc, &argv[1] );
        break;
      case 'M':
      {
      char *name[4];
      int i;

        if( argc < 3 )
        {
          fprintf( stderr, "Usage: %s <outarch> <inarch>...\n",argv[1] );
          exit(-1);
        }
        name[0] = argv[1];
        name[1] = argv[2];
        name[3] = strdup( "__tmp.arch" );
        if(copy(argv[3],name[1]))
        {
          fprintf( stderr, "Couldn't copy %s to %s!\n", argv[3], name[3]);
          exit(-1);
        }
        i = 4;
        while( i <= argc )
        {
          name[2] = argv[i];
          retval = mergearch( 4, name );
          if(rename(name[3],name[1]))
          {
            fprintf( stderr, "Couldn't move %s to %s!\n", name[3], name[1]);
            exit(-1);
          }
          i++;
        }
        break;
      }
      case 'c':
        retval = genlibdefs( argc, &argv[1] );
        break;
      case 'd':
        retval = gendefines( argc, &argv[1] );
        break;
      case 'C':
        retval = genclib( argc, &argv[1] );
        break;
      case 'p':
        retval = genproto( argc, &argv[1] );
        break;
      case 'i':
        retval = geninline( argc, &argv[1] );
        break;
      case 'I':
        retval = genincludes( argc, &argv[1] );
        break;
      case 'r':
        retval = genpragmas( argc, &argv[1] );
        break;
      case 'R':
        retval = genarossource(argc, &argv[1]);
        break;
      case 'h':
      default:
        fprintf( stdout, "Usage: %s [-h|-e|-a|-t|-s|-m|-M|-c|-d|-C|-p|-i|-r|-I] <parameter>\n", argv[0] );
        fprintf( stdout, "  -h help\n  -e extractfiles\n  -a genautodocs\n  -t genfunctable\n  -s gensource\n  -m mergearch\n  -M mergearchs\n  -c genlibdefs\n  -d gendefines\n  -C genclib\n  -p genproto\n  -i geninline\n  -r genpragmas\n  -I genincludes\n  -R genarossource\n" );
        break;
    }
  }

return retval;
}

