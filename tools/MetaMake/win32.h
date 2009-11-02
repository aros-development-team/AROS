/* Under MinGW we don't have softlinks */
#ifdef _WIN32
#define lstat stat
#define S_ISLNK(x) 0

/* Taken from libiberty */
int mkstemps (char *pattern, int suffix_len);
#endif
