#ifndef SUPPORT_H_
#define SUPPORT_H_

const char *remove_path(const char *in);
int strncmp(const char *s1, const char*s2, long length);
void memcpy(void *dest, const void *src, long len);
int strlen(const char *s);
const char *remove_path(const char *in);
char *strstr (const char * str, const char * search);
void *bzero(void *ptr, long len);
void *memset(void *ptr, int c, long len);

#endif /*SUPPORT_H_*/
