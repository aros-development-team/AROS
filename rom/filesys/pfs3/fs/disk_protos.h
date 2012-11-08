/* Prototypes for functions defined in
disk.c
 */

//ULONG ReadFromFile(fileentry_t * , UBYTE * , ULONG , SIPTR * , globaldata * );
//ULONG WriteToFile(fileentry_t * , UBYTE * , ULONG , SIPTR * , globaldata * );
LONG ChangeFileSize(fileentry_t * , LONG , LONG , SIPTR * , globaldata * );
ULONG ReadFromObject(fileentry_t * , UBYTE * , ULONG , SIPTR * , globaldata * );
ULONG WriteToObject(fileentry_t * , UBYTE * , ULONG , SIPTR * , globaldata * );
LONG SeekInObject(fileentry_t * , LONG , LONG , SIPTR * , globaldata * );
LONG ChangeObjectSize(fileentry_t * , LONG , LONG , SIPTR * , globaldata * );
LONG SeekInFile(fileentry_t *file, LONG offset, LONG mode, SIPTR *error, globaldata *g);

ULONG DiskRead(UBYTE * , ULONG , ULONG , globaldata * );

ULONG DiskWrite(UBYTE * , ULONG , ULONG , globaldata * );

ULONG RawRead(UBYTE * , ULONG , ULONG , globaldata * );

ULONG RawWrite(UBYTE * , ULONG , ULONG , globaldata * );

void MotorOff(globaldata * );

void UpdateCache(globaldata *g);
void FlushDataCache(globaldata *g);
void UpdateDataCache(globaldata *g);

BOOL detectaccessmode(UBYTE *buffer, globaldata *g);
