void *Host_HostLib_Open(const char *filename, char **error);
BOOL Host_HostLib_Close(void *handle, char **error);
void Host_HostLib_FreeErrorStr(char *error);
void *Host_HostLib_GetPointer(void *handle, const char *symbol, char **error);
unsigned long Host_HostLib_GetInterface(void *handle, char **names, void **funcs);
