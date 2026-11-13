#ifdef _UNICODE

LPTSTR StrConvert(const char *src);
#define StrFree(s) free(s);

#else

#define StrConvert(s) s
#define StrFree(s)

#endif
