#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

char *get_line(FILE *);
char *keyword(char *);
int get_words(char *, char ***);

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

