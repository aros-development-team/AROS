#define SD_ACTION_POWEROFF   0
#define SD_ACTION_COLDREBOOT 1

extern char bootstrapdir[];
extern char **Kernel_ArgV;

void Host_Shutdown(unsigned long action);
