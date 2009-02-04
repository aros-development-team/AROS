#ifndef SUPPORT_H_
#define SUPPORT_H_

const char *__bs_remove_path(const char *in);
int __bs_strncmp(const char *s1, const char*s2, long length);
void __bs_memcpy(void *dest, const void *src, long len);
int __bs_strlen(const char *s);
char *__bs_strstr (const char * str, const char * search);
void *__bs_bzero(void *ptr, long len);
void *__bs_memset(void *ptr, int c, long len);


#endif /*SUPPORT_H_*/
