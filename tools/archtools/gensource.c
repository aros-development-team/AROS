#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>

extern char *get_line(FILE *);
extern char *keyword(char *);
extern int get_words(char *, char ***);


int main(int argc, char **argv)
{
FILE *fd, *fdo = NULL;
char *line = 0;
char *word, **words;
int in_archive, in_function, in_autodoc, in_header, in_code;
int num;
char **name = NULL, **type = NULL, **reg = NULL;
int numparams=0;

  if(argc != 3)
  {
    fprintf( stderr, "Usage %s <archfile> <sourcefile>\n", argv[0] );
    exit(-1);
  }
  fd = fopen(argv[1],"rb");
  if(!fd)
  {
    fprintf( stderr, "Couldn't open file %s\n", argv[1] );
    exit(-1);
  }
  fdo = fopen(argv[2],"w");
  if(!fdo)
  {
    fprintf( stderr, "Couldn't open file %s\n", argv[2] );
    exit(-1);
  }

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
      if( strcmp(word,"/Archive")==0 && in_archive && !in_autodoc && ! in_function )
        break;
      if( strcmp(word,"AutoDoc")==0 )
        in_autodoc = 1;
      if( strcmp(word,"/AutoDoc")==0 )
        in_autodoc = 0;
      if( strcmp(word,"Parameter")==0 && in_archive && in_function && !in_header && !in_autodoc )
      {
        numparams++;
        num = get_words( line, &words );
        name = realloc( name, (numparams+1)*sizeof(char *) );
        name[numparams] = strdup( words[num-2] );
        type = realloc( type, (numparams+1)*sizeof(char *) );
        type[numparams] = strdup( words[1] );
        reg = realloc( reg, (numparams+1)*sizeof(char *) );
        reg[numparams] = strdup( words[3] );
      }
      if( strcmp(word,"Header")==0 && in_archive && !in_function && !in_header )
        in_header = 1;
      if( strcmp(word,"/Header")==0 && in_archive && !in_function && in_header )
        in_header = 0;
      if( strcmp(word,"Code")==0 && in_archive && in_function && !in_code && !in_autodoc )
      {
      int i;
        fprintf( fdo, "\nAROS_LH%d(%s, %s,\n", numparams, type[0], name[0] );
        for( i = 1 ; i <= numparams ; i++ )
          fprintf( fdo, "AROS_LHA(%s, %s, %s),\n", type[i], name[i], reg[i] );
#warning TODO: get LibraryBase, etc. from lib.conf
        fprintf( fdo, "%s, %s, %s, %s)\n", "struct Library *", "LibraryBase", reg[0], "Library" );
        in_code = 1;
      }
      if( strcmp(word,"/Code")==0 && in_archive && in_function && in_code && !in_autodoc )
        in_code = 0;
      if( strcmp(word,"Function")==0 && in_archive && !in_function && !in_autodoc )
      {
        num = get_words(line,&words);
        name = realloc( name, sizeof(char *) );
        name[0] = strdup(words[num-1]);
        type = realloc( type, sizeof(char *) );
        type[0] = strdup(words[2]);
        numparams = 0;
        in_function = 1;
      }
      if( strcmp(word,"/Function")==0 && in_archive && in_function && !in_autodoc )
        in_function = 0;
      if( strcmp(word,"LibOffset")==0 && in_archive && in_function && !in_autodoc && !in_code )
      {
        num = get_words(line,&words);
        reg = realloc( reg, sizeof(char *) );
        reg[0] = strdup(words[1]);
      }

      free(word);
    }
    else
    {
      if(in_header || in_code )
        fprintf( fdo, "%s\n", line );
    }
    free(line);
  }
  fclose(fdo);
  fclose(fd);

return 0;
}
