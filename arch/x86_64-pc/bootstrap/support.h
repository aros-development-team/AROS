#ifndef SUPPORT_H_
#define SUPPORT_H_

const char *__bs_remove_path(const char *in);
void *__bs_memcpy(void *dest, const void *src, long len);
void kprintf(const char *, ...);

#endif /*SUPPORT_H_*/
