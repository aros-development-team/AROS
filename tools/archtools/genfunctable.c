#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern char *get_line(FILE *);
extern char *keyword(char *);
extern int get_words(char *, char ***);

void emit(FILE *out, char *place, char **names, int number)
{
int i;

  fprintf( out, "/*\n" );
  fprintf( out, "    Copyright (C) 1995-1998 AROS - The Amiga Replacement OS\n" );
  fprintf( out, "    *** Automatic generated file. Do not edit ***\n" );
  fprintf( out, "    Desc: Function table for %s\n", place );
  fprintf( out, "    Lang: english\n" );
  fprintf( out, "*/\n" );
  fprintf( out, "#ifndef LIBCORE_COMPILER_H\n" );
  fprintf( out, "#   include <libcore/compiler.h>\n" );
  fprintf( out, "#endif\n" );
  fprintf( out, "#ifndef NULL\n" );
  fprintf( out, "#define NULL ((void *)0)\n" );
  fprintf( out, "#endif\n\n" );
  fprintf( out, "#include \"libdefs.h\"\n" );
  fprintf( out, "extern void AROS_SLIB_ENTRY(open, BASENAME) (void);\n" );
  fprintf( out, "extern void AROS_SLIB_ENTRY(close, BASENAME) (void);\n" );
  fprintf( out, "extern void AROS_SLIB_ENTRY(expunge, BASENAME) (void);\n" );
  fprintf( out, "extern void AROS_SLIB_ENTRY(null, BASENAME) (void);\n" );
/*
  fprintf( out, "extern void AROS_SLIB_ENTRY(LC_BUILDNAME(OpenLib),LibHeader) (void);\n" );
  fprintf( out, "extern void AROS_SLIB_ENTRY(LC_BUILDNAME(CloseLib),LibHeader) (void);\n" );
  fprintf( out, "extern void AROS_SLIB_ENTRY(LC_BUILDNAME(ExpungeLib),LibHeader) (void);\n" );
  fprintf( out, "extern void AROS_SLIB_ENTRY(LC_BUILDNAME(ExtFuncLib),LibHeader) (void);\n" );
*/
  for( i = 0 ; i < number-4 ; i++ )
  {
    if(names[i])
      fprintf( out, "extern void AROS_SLIB_ENTRY(%s,BASENAME) (void);\n", names[i] );
  }
  fprintf( out, "\nvoid *const LIBFUNCTABLE[]=\n{\n" );
  fprintf( out, "    AROS_SLIB_ENTRY(open, BASENAME),\n" );
  fprintf( out, "    AROS_SLIB_ENTRY(close, BASENAME),\n" );
  fprintf( out, "    AROS_SLIB_ENTRY(expunge, BASENAME),\n" );
  fprintf( out, "    AROS_SLIB_ENTRY(null, BASENAME),\n" );
/*
  fprintf( out, "    AROS_SLIB_ENTRY(LC_BUILDNAME(OpenLib),LibHeader),\n" );
  fprintf( out, "    AROS_SLIB_ENTRY(LC_BUILDNAME(CloseLib),LibHeader),\n" );
  fprintf( out, "    AROS_SLIB_ENTRY(LC_BUILDNAME(ExpungeLib),LibHeader),\n" );
  fprintf( out, "    AROS_SLIB_ENTRY(LC_BUILDNAME(ExtFuncLib),LibHeader),\n" );
*/
  for( i = 0 ; i < number-4 ; i++ )
  {
    if(names[i])
      fprintf( out, "    AROS_SLIB_ENTRY(%s,BASENAME), /* %d */\n", names[i], i+5 );
    else
      fprintf( out, "    NULL, /* %d */\n", i+5 );
  }
  fprintf( out, "    (void *)-1L\n};\n" );
}

int main(int argc, char **argv)
{
FILE *fd, *fdo;
char *line = 0;
char *word, **words;
int in_archive, in_header, in_function, in_autodoc, in_code;
int num;
char *name, *funcname = NULL, **funcnames = NULL;
/* Well, there are already 4 functions (open,close,expunge,null) */
int numfuncs = 4;

  if(argc != 3)
  {
    fprintf( stderr, "Usage %s <name> <archfile>\n", argv[0] );
    exit(-1);
  }
  name = argv[1];
  fd = fopen(argv[2],"rb");
  if(!fd)
  {
    fprintf( stderr, "Couldn't open file %s\n", argv[2] );
    exit(-1);
  }
  fdo = fopen("functable.c","w");
  if(!fdo)
  {
    fprintf( stderr, "Couldn't open file functable.c\n" );
    exit(-1);
  }

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
        funcname = strdup(words[3]);
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
  emit(fdo,name,funcnames,numfuncs);
  fclose(fdo);
  fclose(fd);

return 0;
}

