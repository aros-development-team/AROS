#ifndef SUPPORT_H_
#define SUPPORT_H_

char *__bs_remove_path(char *in);
void *__bs_malloc(unsigned long size);
void __bs_free(void);
void kprintf(const char *, ...);

#endif /*SUPPORT_H_*/
