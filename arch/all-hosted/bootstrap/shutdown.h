#define SD_ACTION_POWEROFF   0
#define SD_ACTION_COLDREBOOT 1
#define SD_ACTION_WARMREBOOT 2

extern char bootstrapdir[];

void SaveArgs(char **argv);

