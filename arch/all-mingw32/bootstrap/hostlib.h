#ifdef __x86_64__
#define __aros __attribute__((sysv_abi))
#else
#define __aros
#endif

void * __aros Host_HostLib_Open(const char *filename, char **error);
int    __aros Host_HostLib_Close(void *handle, char **error);
void   __aros Host_HostLib_FreeErrorStr(char *error);
void * __aros Host_HostLib_GetPointer(void *handle, const char *symbol, char **error);

void __aros Host_Shutdown(unsigned char warm);

