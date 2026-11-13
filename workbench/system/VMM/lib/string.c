#include <string.h>

char *strcpy(char *dest, const char *src)

{
char *tmp1, *tmp2;

tmp1 = dest;
tmp2 = (char*)src;

while ((*tmp1++ = *tmp2++) != 0);

return (dest);
}

/***********************************************************************/

char *strncpy(char *dest, const char *src, size_t len)

{
char *tmp1, *tmp2;
int i;

tmp1 = dest;
tmp2 = (char*)src;
i = (int)len;

while ((i-- > 0) && ((*tmp1++ = *tmp2++) != 0));

return (dest);
}

/***********************************************************************/

char *strcat(char *dest, const char *src)

{
char *tmp1, *tmp2;

tmp1 = dest;
tmp2 = (char*)src;
while (*tmp1 != 0)
  tmp1++;

while ((*tmp1++ = *tmp2++) != 0);

return (dest);
}

/***********************************************************************/

int strcmp(const char *str1, const char *str2)

{
char *tmp1, *tmp2;

tmp1 = (char*)str1;
tmp2 = (char*)str2;

while ((*tmp1 == *tmp2) && (*tmp1 != 0))
  {
  tmp1++;
  tmp2++;
  }

return ((int)(*tmp1 - *tmp2));
}

/***********************************************************************/

int strncmp(const char *str1, const char *str2, size_t len)

{
char *tmp1, *tmp2;
int i;

tmp1 = (char*)str1;
tmp2 = (char*)str2;
i = (int)len - 1;

while ((*tmp1 == *tmp2) && (*tmp1 != 0) && (i > 0))
  {
  tmp1++;
  tmp2++;
  i--;
  }

return ((int)(*tmp1 - *tmp2));
}

/***********************************************************************/

char *strchr(const char *string, int c)

{
char *tmp;

tmp = (char*)string;

while ((*tmp != c) && (*tmp != 0))
  tmp++;

if (*tmp == 0)
  return (NULL);

return (tmp);
}

/***********************************************************************/

size_t strlen(const char *string)

{
int len = 0;
char *tmp;

tmp = (char*)string;
while (*tmp != 0)
  {
  tmp++;
  len++;
  }

return ((size_t)len);
}

/***********************************************************************/

void CopyMem (void*, void*, unsigned long);

#ifdef __GNUC__
void bcopy (const void *from, void *to, size_t size)
#else
void *bcopy (const void *from, void *to, size_t size)
#endif

{
CopyMem ((void*)from, to, size);

#ifndef __GNUC__
return (to);
#endif
}
