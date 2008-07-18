void *Kern_HostLib_Open(const char *filename, char **error);
BOOL Kern_HostLib_Close(void *handle, char **error);
void Kern_HostLib_FreeErrorStr(char *error);
void *Kern_HostLib_GetPointer(void *handle, const char *symbol, char **error);
unsigned long Kern_HostLib_GetInterface(void *handle, char **names, void **funcs);
