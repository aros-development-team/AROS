#if !defined(_STRSUP_H) && defined(__OPTIMIZE__)

#define _STRSUP_H

#include <sys/types.h>

extern __inline__ void *memcpy(void *s1,const void *s2,size_t n)
{ register char *a6 __asm("a6") = *(char **)4;
  register const void *a0 __asm("a0") = s2;
  register const void *a1 __asm("a1") = s1;
  register size_t d0 __asm("d0") = n;
  __asm __volatile ("jsr a6@(-0x270)"
  : /* no output */
  : "r" (a6), "r" (a0), "r" (a1), "r" (d0)
  : "a0","a1","d0","d1", "memory");

  return s1;
}

extern __inline__ void *memmove(void *s1,const void *s2,size_t n)
{ 
  bcopy(s2,s1,n); return s1;
}

extern __inline__ void *memset(void *s,int c,size_t n)
{ 
  if (n != 0) {
    unsigned char *p=s;
    do {
      *p++=c;
    } while(--n != 0);
  }
  return s;
}

extern __inline__ int memcmp(const void *s1,const void *s2,size_t n)
{ const unsigned char *p1=s1,*p2=s2;
  unsigned long r,c;

  if((r=n)) {
    c=0;
    do {
      r=*p1++;
      ((unsigned char)c)=*p2++;
    } while(!(r-=c) && --n);
  }
  return r;
}

extern __inline__ void *memchr(const void *s,int c,size_t n)
{
  if (n != 0) {
    unsigned char *p=(unsigned char *)s;
    do {
      if (*p++==(unsigned char)c)
        return --p;
    } while(--n != 0);
  }
  return (void *)n;
}

extern __inline__ size_t strlen_plus_one(const char *string)
{ const char *s=string;

  do{}while(*s++); return (s-string);
}

extern __inline__ size_t strlen(const char *string)
{ const char *s=string;

  do{}while(*s++); return ~(string-s);
}

extern __inline__ char *strcpy(char *s1,const char *s2)
{ char *s=s1;

  do {
    *s++=*s2;
  } while(*s2++!='\0');
  return s1;
}

extern __inline__ char *strncpy(char *s1,const char *s2,size_t n)
{
  if (n != 0) {
    char *s=s1;

    do{}while((*s++=*s2++) && (--n != 0));
    if (n)
      while(--n != 0) *s++=0;
  }
  return s1;
}

extern __inline__ char *strcat(char *s1,const char *s2)
{ char *s=s1;

  do{}while(*s++); --s; do{}while((*s++=*s2++)); return s1;
}

extern __inline__ char *strncat(char *s1,const char *s2,size_t n)
{
  if (n != 0) {
    char *s=s1;

    do{}while(*s++); --s;
    for(;;) {
      if(!(*s++=*s2++))
        return s1;
      if(!--n) {
        *s=0; return s1;
      }
    }
  }
  return s1;
} 

extern __inline__ int strcmp(const char *s1,const char *s2)
{ const unsigned char *p1=s1,*p2=s2;
  unsigned long r,c;

  c=0;  
  do {
    r=*p1++;
    ((unsigned char)c)=*p2++;
  } while(!(r-=c) && (unsigned char)c);
  return r;
}

extern __inline__ int strncmp(const char *s1,const char *s2,size_t n)
{ const unsigned char *p1=s1,*p2=s2;
  unsigned long r,c;

  if((r=n)) {
    c=0;  
    do {
      r=*p1++;
      ((unsigned char)c)=*p2++;
    } while(!(r-=c) && (unsigned char)c && --n);
  }
  return r;
}
#if 0
extern __inline__ char *strchr(const char *s,int c)
{
  while (*s!=(char)c)
    if (!(*s++))
      return NULL;
  return (char *)s;
}
#endif
extern __inline__ char *strupr(char *s)
{ unsigned char *s1=(unsigned char *)s;

  while(*s1) {
    if ((*s1>('a'-1)) && (*s1<('z'+1)))
      *s1-='a'-'A';
    s1++;
  }
  return s;
}

extern __inline__ char *strlwr(char *s)
{ unsigned char *s1=(unsigned char *)s;

  while(*s1) {
    if ((*s1>('A'-1)) && (*s1<('Z'+1)))
      *s1+='a'-'A';
    s1++;
  }
  return s;
}

extern __inline__ char *stpcpy(char *dst,char *src)
{
  do{}while((*dst++=*src++)); return(dst-1);
}

#elif !defined(__OPTIMIZE__)

#define strlen_plus_one(s) strlen(s)+1L

#endif /* _STRSUP_H */
