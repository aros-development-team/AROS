#ifndef _FUNCS_H
#define _FUNCS_H

/* Prototypes for functions which Purify supports */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>

/* memory.c */
void * Purify_malloc (size_t size);
void * Purify_calloc (size_t nemb, size_t size);
void * Purify_realloc (void * mem, size_t size);
void Purify_free (void * mem);
void * Purify_memmove (void *dest, const void *src, size_t n);
void * Purify_memcpy (void *dest, const void *src, size_t n);
char * Purify_strcpy (char *dest, const char *src);
char * Purify_strcat (char *dest, const char *src);
char * Purify_strncpy (char *dest, const char *src, size_t n);
char * Purify_strdup (char * src);

/* io.c */
size_t Purify_fread (void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t Purify_read (int fd, void *ptr, size_t size);
char * Purify_fgets (char * ptr, int size, FILE * stream);
char * Purify_getcwd (char * buf, size_t size);
int Purify_stat (char * path, struct stat * st);
int Purify_lstat (char * path, struct stat * st);
int Purify_fstat (int fd, struct stat * st);
struct dirent * Purify_readdir (DIR * dir);


#endif /* _FUNCS_H */
