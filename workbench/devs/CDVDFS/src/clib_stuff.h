#ifndef CLIB_STUFF_H
#define CLIB_STUFF_H

#define StrCpy(s1,s2)    strcpy(s1,s2)
#define StrCat(s1,s2)    strcat(s1,s2)
#define StrLen(s)        strlen(s)
#define StrChr(s,c)      strchr(s,c)
#define StrStr(s1,s2)    strstr(s1,s2)
#define StrCmp(s1,s2)    strcmp(s1,s2)
#define StrNCmp(s1,s2,n) strncmp(s1,s2,n)
#define MemSet(p,c,n)    memset(p,c,n)
#define MemCmp(p1,p2,n)  memcmp(p1,p2,n)

#endif

