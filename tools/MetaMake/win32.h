/* Under MinGW we don't have softlinks */
#ifdef _WIN32
#define lstat stat
#define S_ISLNK(x) 0

int mkstemps (char *pattern, int suffix_len);

#define mkstemp(x) mkstemps(x, 0)
#endif
