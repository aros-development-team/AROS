/* Prototypes for functions defined in
init.c
 */

BOOL Initialize(DSTR , struct FileSysStartupMsg * , struct DeviceNode * , globaldata * );

void InitModules(struct volumedata * , BOOL, globaldata * );

void lock_device_unit(struct globaldata * );
void unlock_device_unit(struct globaldata * );

void UninstallDiskChangeHandler(struct globaldata *);

void UninstallResetHandler(struct globaldata * );
void HandshakeResetHandler(struct globaldata * );
