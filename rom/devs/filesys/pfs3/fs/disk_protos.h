/* Prototypes for functions defined in
disk.c
 */

//ULONG ReadFromFile(fileentry_t * , UBYTE * , ULONG , ULONG * , globaldata * );
//ULONG WriteToFile(fileentry_t * , UBYTE * , ULONG , ULONG * , globaldata * );
LONG ChangeFileSize(fileentry_t * , LONG , LONG , ULONG * , globaldata * );
ULONG ReadFromObject(fileentry_t * , UBYTE * , ULONG , ULONG * , globaldata * );
ULONG WriteToObject(fileentry_t * , UBYTE * , ULONG , ULONG * , globaldata * );
LONG SeekInObject(fileentry_t * , LONG , LONG , ULONG * , globaldata * );
LONG ChangeObjectSize(fileentry_t * , LONG , LONG , ULONG * , globaldata * );
LONG SeekInFile(fileentry_t *file, LONG offset, LONG mode, ULONG *error, globaldata *g);

ULONG DiskRead(UBYTE * , ULONG , ULONG , globaldata * );

ULONG DiskWrite(UBYTE * , ULONG , ULONG , globaldata * );

ULONG RawRead(UBYTE * , ULONG , ULONG , globaldata * );

ULONG RawWrite(UBYTE * , ULONG , ULONG , globaldata * );

void MotorOff(globaldata * );

void UpdateCache (globaldata *g);
void FlushDataCache (globaldata *g);
void UpdateDataCache (globaldata *g);

#if SCSIDIRECT
int DoSCSICommand(UBYTE *data, ULONG datlen, UBYTE *cmd, UWORD cmdlen, UBYTE dir, globaldata *g);
#endif /* SCSIDIRECT */
