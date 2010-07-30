void *Host_HostLib_Open(const char *filename);
int Host_HostLib_Close(void *handle);
char *Host_HostLib_GetErrorStr(void);
void *Host_HostLib_GetPointer(void *handle, const char *symbol);
unsigned long Host_HostLib_GetInterface(void *handle, char **names, void **funcs);
