#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

extern char *get_line(FILE *);
extern char *keyword(char *);
extern int get_words(char *, char ***);

void replace( FILE *, FILE *, int);

int main(int argc, char **argv)
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
    fprintf( stderr, "Usage %s <arch1> <arch2> <archout>\n", argv[0] );
    exit(-1);
  }
  fd1 = fopen(argv[1],"rb");
  if(!fd1)
  {
    fprintf( stderr, "Couldn't open file %s\n", argv[1] );
    exit(-1);
  }
  fd2 = fopen(argv[2],"rb");
  if(!fd2)
  {
    fprintf( stderr, "Couldn't open file %s\n", argv[2] );
    exit(-1);
  }
  fdo = fopen(argv[3],"w");
  if(!fdo)
  {
    fprintf( stderr, "Couldn't open file %s\n", argv[3] );
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

