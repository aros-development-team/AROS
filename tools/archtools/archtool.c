#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>

char *get_line(FILE *fd)
{
int count,len;
char *line;
char buffer;
char *ln;

  len = 0;
  do
  {
    len += (count = fread(&buffer,1,1,fd));
  } while(count!=0 && buffer!='\n');
  if(len==0 && count==0)
    return NULL;
  fseek( fd, -len, SEEK_CUR );
  line = malloc( (len+1) * sizeof(char) );
  fread (line,1,len,fd);
  line[len]=0;
  ln = &line[len-1];
  while(isspace(*ln)&& ln>=line)
  {
    *ln = 0;
    ln--;
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
      array = realloc( array, num * sizeof(char *) );
      array[num-1] = malloc( (len+1) * sizeof(char) );
      strncpy( array[num-1], word, len );
      array[num-1][len] = 0;
      word = &word[len];
    }
  }
  array = realloc( array, (num+1) * sizeof(char *) );
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

enum libtype
{
  t_device = 1,
  t_library = 2,
  t_resource = 3,
  t_hidd = 4
};

enum liboption
{
  o_noexpunge = 1,
  o_rom = 2,
  o_unique = 4,
  o_nolibheader = 8
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

int parse_libconf(char *file, struct libconf *lc)
{
FILE *fd;
int num, len, i;
char *line, *word;
char **words = NULL;

  if(file)
    fd = fopen(file,"rb");
  else
    fd = fopen("lib.conf","rb");
  if(!fd)
  {
    fprintf( stderr, "Couldn't open %s!\n", (file?file:"lib.conf") );
    return -1;
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
        if( lc->basename == NULL )
        {
          lc->basename = strdup(words[1]);
          lc->basename[0] = toupper(lc->basename[0]);
        }
        if( lc->libbase == NULL )
        {
          lc->libbase = malloc( (len+5) * sizeof(char) );
          sprintf( lc->libbase, "%sBase",words[1]);
          lc->libbase[0] = toupper(lc->libbase[0]);
        }
        if( lc->libbasetype == NULL )
        {
          lc->libbasetype = malloc( (len+12) * sizeof(char) );
          sprintf( lc->libbasetype, "struct %sBase",words[1]);
          lc->libbasetype[7] = toupper(lc->libbasetype[7]);
        }
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
        if( lc->libbase == NULL )
        {
          lc->libbase = malloc( (len+5) * sizeof(char) );
          sprintf( lc->libbase, "%sBase",words[1]);
        }
        if( lc->libbasetype == NULL )
        {
          lc->libbasetype = malloc( (len+12) * sizeof(char) );
          sprintf( lc->libbasetype, "struct %sBase",words[1]);
        }
      }
      else if( strcmp(words[0],"libbase")==0 )
      {
        len = strlen(words[1]);
        free(lc->libbase);
        lc->libbase = strdup(words[1]);
        if( lc->libbasetype == NULL )
        {
          lc->libbasetype = malloc( (len+8) * sizeof(char) );
          sprintf( lc->libbasetype, "struct %s",words[1]);
        }
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
      }
      else if( strcmp(words[0],"options")==0 )
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
        }
      }
    }
    free(line);
  }
  if( lc->define == NULL )
    lc->define = strdup( "_LIBDEFS_H" );
  if( lc->type == 0 )
    lc->type = t_library;

return 0;
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
#		    libbasetype     struct XxxBase
#		    libbasetypeptr  struct XxxBase *
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
char *date;
struct tm *tm;
time_t t;

  time(&t);
  tm = localtime(&t);
  date = malloc( 11 * sizeof(char) );
  sprintf( date, "%02d.%02d.%4d", tm->tm_mday, tm->tm_mon+1, tm->tm_year+1900 );
  fd = fopen("libdefs.h","w");
  if(!fd)
  {
    fprintf( stderr, "Couldn't open libdefs.h!\n" );
    return -1;
  }
  lc = calloc( 1, sizeof(struct libconf) );
  if(parse_libconf((argc==2?NULL:argv[1]),lc))
    return(-1);
  if( lc->copyright == NULL )
  {
    lc->copyright = strdup("");
  }
  if( lc->libbasetypeptr == NULL )
  {
    lc->libbasetypeptr = malloc( (strlen(lc->libbasetype)+3) * sizeof(char) );
    sprintf( lc->libbasetypeptr, "%s *", lc->libbasetype );
  }
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


int extractfiles(int argc, char **argv)
{
FILE *fd, *fdo = NULL;
char *line = 0;
char *word, **words;
int in_archive, in_header, in_function, in_autodoc, in_code;
int num, len, i;
char **name = NULL, **type = NULL, **reg = NULL, *header = NULL, *code = NULL;
int numparams=0;

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

  words = NULL;
  in_archive = 0;
  in_function = 0;
  in_autodoc = 0;
  in_header = 0;
  in_code = 0;
  while( (line = get_line(fd)) )
  {
    word = keyword(line);
    if( word && (tolower(word[0])!=word[0] || tolower(word[1])!=word[1]) )
    {
      if( strcmp(word,"Archive")==0 && !in_archive )
        in_archive = 1;
      if( strcmp(word,"/Archive")==0 && in_archive && ! in_function && !in_header )
        break;
      if( strcmp(word,"AutoDoc")==0 && in_function && !in_autodoc )
        in_autodoc = 1;
      if( strcmp(word,"/AutoDoc")==0 && in_autodoc )
        in_autodoc = 0;
      if( strcmp(word,"Code")==0 && in_function && !in_code && !in_autodoc )
        in_code = 1;
      if( strcmp(word,"/Code")==0 && in_code )
        in_code = 0;
      if( strcmp(word,"Header")==0 && in_archive && !in_function && !in_header )
        in_header = 1;
      if( strcmp(word,"/Header")==0 && in_header )
        in_header = 0;
      if( strcmp(word,"Function")==0 && in_archive && !in_function && !in_header )
      {
      char *filename;

        num = get_words(line,&words);
        name = realloc( name, sizeof(char *) );
        name[0] = strdup(words[num-1]);
        type = realloc( type, sizeof(char *) );
        len = 0;
        for( i=2 ; i < num-1 ; i++ )
          len += strlen(words[i]);
        type[0] = malloc( (len+num-3) * sizeof(char) );
        strcpy( type[0], words[2]);
        for( i=3 ; i < num-1 ; i++ )
        {
          strcat( type[0], " " );
          strcat( type[0], words[i] );
        }
        numparams = 0;
        if(fdo)
          fclose(fdo);
        filename = malloc( (strlen(name[0])+3)*sizeof(char) );
        sprintf( filename, "%s.c", name[0] );
        for( i = 0 ; filename[i] ; i++ )
          filename[i] = tolower(filename[i]);
        fdo = fopen(filename,"w");
        if(!fdo)
        {
          fprintf( stderr, "Couldn't open file %s!\n", filename );
          exit(-1);
        }
        in_function = 1;
        fprintf( fdo, "#include \"libdefs.h\"\n\n" );
      }
      if( strcmp(word,"/Function")==0 && in_function && !in_autodoc && !in_code )
      {
        fprintf( fdo, "%s\n", header);
        fprintf( fdo, "AROS_LH%d(%s, %s,\n", numparams, type[0], name[0] );
        for( i = 1 ; i <= numparams ; i++ )
          fprintf( fdo, "AROS_LHA(%s, %s, %s),\n", type[i], name[i], reg[i] );
        fprintf( fdo, "struct LIBBASETYPE *, LIBBASE, %s, BASENAME)\n", reg[0] );
        fprintf( fdo, "%s\n", code );
        in_function = 0;
        free(code);
        for( i=0; i<=numparams; i++ )
        {
            free(name[i]);
        }
        free(name);
        name = NULL;
        code = NULL;
      }
      if( strcmp(word,"Parameter")==0 && in_function && !in_autodoc && !in_code )
      {
        numparams++;
        num = get_words( line, &words );
        name = realloc( name, (numparams+1)*sizeof(char *) );
        name[numparams] = strdup( words[num-2] );
        type = realloc( type, (numparams+1)*sizeof(char *) );
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
        reg[numparams] = strdup( words[num-1] );
      }
      if( strcmp(word,"LibOffset")==0 && in_function && !in_autodoc && !in_code )
      {
        num = get_words(line,&words);
        reg = realloc( reg, (numparams+1)*sizeof(char *) );
        reg[0] = strdup(words[1]);
      }

      free(word);
    }
    else
    {
      if(in_header)
      {
        i = (header?strlen(header):0);
        header = realloc( header, (i+strlen(line)+2)*sizeof(char) );
        sprintf( &header[i], "%s\n", line );
      }
      if(in_code)
      {
        i = (code?strlen(code):0);
        code = realloc( code, (i+strlen(line)+2)*sizeof(char) );
        sprintf( &code[i], "%s\n", line );
      }
    }
    free(line);
  }
  fclose(fdo);
  fclose(fd);
  free(header);

return 0;
}

void emit(FILE *out, struct libconf *lc, char **names, int number)
{
int i;

  fprintf( out, "/*\n" );
  fprintf( out, "    Copyright (C) 1995-1998 AROS - The Amiga Replacement OS\n" );
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
  fprintf( out, "#include \"libdefs.h\"\n" );
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
FILE *fd, *fdo;
struct libconf *lc;
char *line = 0;
char *word, **words;
int in_archive, in_header, in_function, in_autodoc, in_code;
int num;
char *funcname = NULL, **funcnames = NULL;

/* Well, there are already 4 functions (open,close,expunge,null) */
int numfuncs = 4;

  if(argc != 2)
  {
    fprintf( stderr, "Usage: %s <archfile>\n", argv[0] );
    exit(-1);
  }
  fd = fopen(argv[1],"rb");
  if(!fd)
  {
    fprintf( stderr, "Couldn't open file %s!\n", argv[1] );
    exit(-1);
  }
  fdo = fopen("functable.c","w");
  if(!fdo)
  {
    fprintf( stderr, "Couldn't open file functable.c!\n" );
    exit(-1);
  }

  lc = calloc( 1, sizeof(struct libconf) );
  if(parse_libconf(NULL,lc))
    return(-1);

  words = NULL;
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
      if( strcmp(word,"/Archive")==0 && in_archive && ! in_function )
        break;
      if( strcmp(word,"AutoDoc")==0 && in_function && !in_autodoc && !in_code )
        in_autodoc = 1;
      if( strcmp(word,"/AutoDoc")==0 && in_autodoc )
        in_autodoc = 0;
      if( strcmp(word,"Code")==0 && in_function && !in_code && !in_autodoc )
        in_code = 1;
      if( strcmp(word,"/Code")==0 && in_code )
        in_code = 0;
      if( strcmp(word,"Header")==0 && in_archive && !in_function )
        in_header = 1;
      if( strcmp(word,"/Header")==0 && in_header )
        in_header = 0;
      if( strcmp(word,"Function")==0 && in_archive && !in_function && !in_header )
      {
        num = get_words(line,&words);
        funcname = strdup(words[num-1]);
        in_function = 1;
      }
      if( strcmp(word,"/Function")==0 && in_function && !in_autodoc && !in_code )
        in_function = 0;
      if( strcmp(word,"LibOffset")==0 && in_function && !in_autodoc && !in_code )
      {
        num = get_words(line,&words);
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
  emit(fdo,lc,funcnames,numfuncs);
  fclose(fdo);
  fclose(fd);

return 0;
}


int gensource(int argc, char **argv)
{
FILE *fd, *fdo = NULL;
char *line = 0;
char *word = NULL, **words;
int in_archive, in_header, in_function, in_autodoc, in_code;
int num, i, len;
char **name = NULL, **type = NULL, **reg = NULL;
int numparams=0;

  if(argc != 3)
  {
    fprintf( stderr, "Usage: %s <archfile> <sourcefile>\n", argv[0] );
    exit(-1);
  }
  fd = fopen(argv[1],"rb");
  if(!fd)
  {
    fprintf( stderr, "Couldn't open file %s!\n", argv[1] );
    exit(-1);
  }
  fdo = fopen(argv[2],"w");
  if(!fdo)
  {
    fprintf( stderr, "Couldn't open file %s!\n", argv[2] );
    exit(-1);
  }

  words = NULL;
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
      if( strcmp(word,"/Archive")==0 && in_archive && !in_header && ! in_function )
        break;
      if( strcmp(word,"AutoDoc")==0 && in_function && !in_code && !in_autodoc )
        in_autodoc = 1;
      if( strcmp(word,"/AutoDoc")==0 && in_autodoc )
        in_autodoc = 0;
      if( strcmp(word,"Code")==0 && in_function && !in_code && !in_autodoc )
      {
        fprintf( fdo, "\nAROS_LH%d(%s, %s,\n", numparams, type[0], name[0] );
        for( i = 1 ; i <= numparams ; i++ )
          fprintf( fdo, "AROS_LHA(%s, %s, %s),\n", type[i], name[i], reg[i] );
        fprintf( fdo, "struct LIBBASETYPE *, LIBBASE, %s, BASENAME)\n", reg[0] );
        in_code = 1;
      }
      if( strcmp(word,"/Code")==0 && in_code )
        in_code = 0;
      if( strcmp(word,"Function")==0 && in_archive && !in_function && !in_header )
      {
        num = get_words(line,&words);
        name = realloc( name, sizeof(char *) );
        name[0] = strdup(words[num-1]);
        type = realloc( type, sizeof(char *) );
        len = 0;
        for( i=2 ; i < num-1 ; i++ )
          len += strlen(words[i]);
        type[0] = malloc( (len+num-3) * sizeof(char) );
        strcpy( type[0], words[2]);
        for( i=3 ; i < num-1 ; i++ )
        {
          strcat( type[0], " " );
          strcat( type[0], words[i] );
        }
        numparams = 0;
        in_function = 1;
      }
      if( strcmp(word,"/Function")==0 && in_function && !in_autodoc && !in_code )
        in_function = 0;
      if( strcmp(word,"Header")==0 && in_archive && !in_function && !in_header )
      {
        fprintf( fdo, "#include \"libdefs.h\"\n\n" );
        in_header = 1;
      }
      if( strcmp(word,"/Header")==0 && in_header )
        in_header = 0;
      if( strcmp(word,"LibOffset")==0 && in_function && !in_autodoc && !in_code )
      {
        num = get_words(line,&words);
        reg = realloc( reg, sizeof(char *) );
        reg[0] = strdup(words[1]);
      }
      if( strcmp(word,"Parameter")==0 && in_function && !in_autodoc && !in_code )
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
        reg = realloc( reg, (numparams+1) * sizeof(char *) );
        reg[numparams] = strdup( words[num-1] );
      }
    }
    else if(in_header || in_code )
      fprintf( fdo, "%s\n", line );

    free(line);
  }
  fclose(fdo);
  fclose(fd);

return 0;
}

int genautodocs(int argc, char **argv)
{
FILE *fd, *fdo = NULL;
char *line = 0;
char *word, **words;
int in_archive, in_header, in_function, in_autodoc, in_afunc, in_code ;
int num, i, len;
char **name = NULL, **type = NULL;
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

  words = NULL;
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
      if( strcmp(word,"/Archive")==0 && in_archive && !in_header && !in_function )
        break;
      if( strcmp(word,"AutoDoc")==0 && in_function && !in_autodoc && !in_code )
      {
      char *filename;

        if(fdo)
          fclose(fdo);
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
      if( strcmp(word,"/AutoDoc")==0 && in_autodoc && !in_afunc )
        in_autodoc = 0;
      if( strcmp(word,"Function")==0 )
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
          for( i=2 ; i < num-1 ; i++ )
            len += strlen(words[i]);
          type[0] = malloc( (len+num-3) * sizeof(char) );
          strcpy( type[0], words[2]);
          for( i=3 ; i < num-1 ; i++ )
          {
            strcat( type[0], " " );
            strcat( type[0], words[i] );
          }
        }
        else if( in_autodoc && !in_afunc )
        {
          fprintf( fdo, "\nFUNCTION\n" );
          num = get_words(line,&words);
          if(num==1)
            in_afunc = 1;
        }
      }
      if( strcmp(word,"/Function")==0 )
      {
        if( in_function && !in_autodoc )
          in_function = 0;
        else if( in_afunc )
          in_afunc = 0;
      }
      if( strcmp(word,"Parameter")==0 && in_function && !in_autodoc && !in_code )
      {
        numparams++;
        num = get_words(line,&words);
        name = realloc( name, (numparams+1)*sizeof(char *) );
        name[numparams] = strdup(words[num-2]);
        type = realloc( type, (numparams+1)*sizeof(char *) );
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
      }
      if( strcmp(word,"Inputs")==0 && in_autodoc )
        fprintf( fdo, "\nINPUTS\n" );
      if( strcmp(word,"Result")==0 && in_autodoc )
        fprintf( fdo, "\nRESULT\n" );
      if( strcmp(word,"Notes")==0 && in_autodoc )
        fprintf( fdo, "\nNOTES\n" );
      if( strcmp(word,"Example")==0 && in_autodoc )
        fprintf( fdo, "\nEXAMPLE\n" );
      if( strcmp(word,"Bugs")==0 && in_autodoc )
        fprintf( fdo, "\nBUGS\n" );
      if( strcmp(word,"SeeAlso")==0 && in_autodoc )
        fprintf( fdo, "\nSEE ALSO\n" );
      if( strcmp(word,"Internals")==0 && in_autodoc )
        fprintf( fdo, "\nINTERNALS\n" );
      if( strcmp(word,"History")==0 && in_autodoc )
        fprintf( fdo, "\nHISTORY\n" );
      if( strcmp(word,"Item")==0 && in_autodoc )
        fprintf( fdo, "<Item>%s\n", &line[6] );

      free(word);
    }
    else if(in_autodoc && line[0] )
      fprintf( fdo, "%s\n", line );
    free(line);
  }
  if(fdo)
    fclose(fdo);
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
      if( strcmp(word,"/Archive")==0 && in_archive && !in_header && !in_function )
        break;
      if( strcmp(word,"Header")==0 && in_archive && !in_header && !in_function )
        in_header = 1;
      if( strcmp(word,"/Header")==0 && in_header )
        in_header = 0;
      if( strcmp(word,"AutoDoc")==0 && in_function && !in_autodoc && !in_code )
        in_autodoc = 1;
      if( strcmp(word,"/AutoDoc")==0 && in_autodoc )
        in_autodoc = 0;
      if( strcmp(word,"Function")==0 && in_archive && !in_function )
      {
        in_function = 1;
        if( i == num )
        {
          writefunc = 1;
        }
        if( i > num )
          return;
        i++;
      }
      if( strcmp(word,"/Function")==0 && in_function && !in_autodoc && !in_code )
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
char *word, **words;
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
  words = NULL;
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
      if( strcmp(word,"/Archive")==0 && in_archive && !in_header && !in_function )
        break;
      if( strcmp(word,"Code")==0 && in_function && !in_code && !in_autodoc )
        in_code = 1;
      if( strcmp(word,"/Code")==0 && in_code )
        in_code = 0;
      if( strcmp(word,"Header")==0 && in_archive && !in_header && !in_function )
        in_header = 1;
      if( strcmp(word,"/Header")==0 && in_header )
        in_header = 0;
      if( strcmp(word,"AutoDoc")==0 && in_function && !in_autodoc && !in_code )
        in_autodoc = 1;
      if( strcmp(word,"/AutoDoc")==0 && in_autodoc )
        in_autodoc = 0;
      if( strcmp(word,"Function")==0 && in_archive && !in_function && !in_header )
      {
        in_function = 1;
        num = get_words(line,&words);
        num1++;
        name1 = realloc( name1, num1 * sizeof(char *) );
        name1[num1-1] = strdup(words[num-1]);
      }
      if( strcmp(word,"/Function")==0 && in_function && !in_autodoc && !in_code )
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
  words = NULL;
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
      if( strcmp(word,"/Archive")==0 && in_archive && !in_header && !in_function )
        break;
      if( strcmp(word,"Header")==0 && in_archive && !in_header && !in_function )
        in_header = 1;
      if( strcmp(word,"/Header")==0 && in_header )
        in_header = 0;
      if( strcmp(word,"AutoDoc")==0 && in_function && !in_autodoc && !in_code )
        in_autodoc = 1;
      if( strcmp(word,"/AutoDoc")==0 && in_autodoc )
        in_autodoc = 0;
      if( strcmp(word,"Function")==0 && in_archive && !in_function )
      {
        in_function = 1;
        num = get_words(line,&words);
        num2++;
        name2 = realloc( name2, num2 * sizeof(char *) );
        name2[num2-1] = strdup(words[num-1]);
      }
      if( strcmp(word,"/Function")==0 && in_function && !in_autodoc && !in_code )
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
  words = NULL;
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
      if( strcmp(word,"/Archive")==0 && in_archive && !in_header && !in_function )
        break;
      if( strcmp(word,"Header")==0 && in_archive && !in_header && !in_function )
        in_header = 1;
      if( strcmp(word,"/Header")==0 && in_header )
        in_header = 0;
      if( strcmp(word,"AutoDoc")==0 && in_function && !in_autodoc && !in_code )
        in_autodoc = 1;
      if( strcmp(word,"/AutoDoc")==0 && in_autodoc )
        in_autodoc = 0;
      if( strcmp(word,"Function")==0 && in_archive && !in_function )
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
      if( strcmp(word,"/Function")==0 && in_function && !in_autodoc && !in_code )
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
  /* Append not replaces functions */
  num = 0;
  words = NULL;
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
      if( strcmp(word,"/Archive")==0 && in_archive && !in_header && !in_function )
        break;
      if( strcmp(word,"Header")==0 && in_archive && !in_header && !in_function )
        in_header = 1;
      if( strcmp(word,"/Header")==0 && in_header )
        in_header = 0;
      if( strcmp(word,"AutoDoc")==0 && in_function && !in_autodoc && !in_code )
        in_autodoc = 1;
      if( strcmp(word,"/AutoDoc")==0 && in_autodoc )
        in_autodoc = 0;
      if( strcmp(word,"Function")==0 && in_archive && !in_function )
      {
        in_function = 1;
        if( rep[num] )
          replace_function = 1;
        else
          replace_function = 0;
        num++;
      }
      if( strcmp(word,"/Function")==0 && in_function && !in_autodoc && !in_code )
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

int main(int argc, char **argv)
{
int retval = 0;
char option;

  if( argc < 2 )
  {
    fprintf( stderr, "Usage: %s [-h|-e|-a|-t|-s|-m|-M|-c] <parameter>\n", argv[0] );
    fprintf( stderr, "  -h help\n  -e extractfiles\n  -a genautodocs\n  -t genfunctable\n  -s gensource\n  -m mergearch\n  -M mergearchs\n  -c genlibdefs\n" );
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
      case 'h':
        fprintf( stdout, "Usage: %s [-h|-e|-a|-t|-s|-m|-M|-c] <parameter>\n", argv[0] );
        fprintf( stdout, "  -h help\n  -e extractfiles\n  -a genautodocs\n  -t genfunctable\n  -s gensource\n  -m mergearch\n  -M mergearchs\n  -c genlibdefs\n" );
        break;
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
      default:
        break;
    }
  }


return retval;
}

