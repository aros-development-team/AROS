/* Magic return codes to signal a restart */
#define STATUS_WARM_REBOOT 0x8F
#define STATUS_COLD_REBOOT 0x81

/* Prototypes for platform-specific functions */
void Host_FreeMem(void);
void Host_ColdBoot(void);
