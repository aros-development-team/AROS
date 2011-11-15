void *Host_HostLib_Open(const char *filename, char **error);
int Host_HostLib_Close(void *handle, char **error);
void *Host_HostLib_GetPointer(void *handle, const char *symbol, char **error);
int KPutC(int chr);
