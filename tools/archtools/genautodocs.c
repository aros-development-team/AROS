#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern char *get_line(FILE *);
extern char *keyword(char *);
extern int get_words(char *, char ***);

int main(int argc, char **argv)
{
FILE *fd, *fdo = NULL;
char *line = 0;
char *word, **words;
int in_archive, in_header, in_function, in_autodoc, in_afunc, in_code ;
int num, i;
char **name = NULL, **type = NULL;
int numparams = 0;

  if(argc != 3)
  {
    fprintf( stderr, "Usage %s <destdir> <archfile>\n", argv[0] );
    exit(-1);
  }
  fd = fopen(argv[2],"rb");
  if(!fd)
  {
    fprintf( stderr, "Couldn't open file %s\n", argv[2] );
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
          fprintf( stderr, "Couldn't open file %s\n", filename );
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
          type[0] = strdup(words[2]);
        }
        else if( in_autodoc && !in_afunc )
        {
          fprintf( fdo, "\nFUNCTION\n" );
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
        type[numparams] = strdup(words[1]);
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
    else
    {
      if(in_autodoc && line[0] )
        fprintf( fdo, "%s\n", line );
    }
    free(line);
  }
  if(fdo)
    fclose(fdo);
  fclose(fd);

return 0;
}

