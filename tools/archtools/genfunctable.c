#include <stdio.h>
#include <stdlib.h>

extern char *get_line(FILE *);
extern char *keyword(char *);
extern int get_words(char *, char ***);

void emit(FILE *out, char *place)
{
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
}

int main(int argc, char **argv)
{
FILE *fd, *fdo;
char *line = 0;
char *word, **words;
int in_archive, in_function, in_autodoc;
int num;
char *name;

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
  while( (line = get_line(fd)) )
  {
    word = keyword(line);
    if( word )
    {
      if( strcmp(word,"Archive")==0 )
      {
        if( !in_archive )
        {
          in_archive = 1;
          emit(fdo,name);
        }
      }
      if( strcmp(word,"/Archive")==0 )
      {
        if( in_archive && !in_autodoc && ! in_function )
          break;
      }
      if( strcmp(word,"AutoDoc")==0 )
        in_autodoc = 1;
      if( strcmp(word,"/AutoDoc")==0 )
        in_autodoc = 0;
      if( strcmp(word,"Function")==0 )
      {
        if( in_archive && !in_function && !in_autodoc )
        {
          in_function = 1;
          num = get_words(line,&words);
          if(num>=4)
            fprintf( fdo, "extern void AROS_SLIB_ENTRY(%s, BASENAME) (void);\n", words[3] );
        }
      }
      if( strcmp(word,"/Function")==0 )
      {
        if( in_archive && in_function && !in_autodoc )
          in_function = 0;
      }

      free(word);
    }
    else
    {
    }
    free(line);
  }
  fclose(fdo);
  fclose(fd);

return 0;
}
