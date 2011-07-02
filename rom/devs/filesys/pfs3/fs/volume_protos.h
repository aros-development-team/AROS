/* Prototypes for functions defined in
volume.c
 */

void NewVolume(BOOL , globaldata * );

void DiskRemoveSequence(globaldata * );

void DiskInsertSequence(struct rootblock * , globaldata * );

struct volumedata * MakeVolumeData(struct rootblock * , globaldata * );

void FreeVolumeResources(struct volumedata * , globaldata * );

void FreeUnusedResources(struct volumedata * , globaldata * );

BOOL UpdateChangeCount(globaldata * );

void UpdateCurrentDisk(globaldata * );

BOOL CheckVolume(struct volumedata * , BOOL , SIPTR * , globaldata * );

LONG ErrorMsg(CONST_STRPTR, APTR, globaldata *);
LONG _NormalErrorMsg(CONST_STRPTR, APTR, ULONG, globaldata * );
#define NormalErrorMsg(a,b,c) _NormalErrorMsg(a,b,c,g)

BOOL GetCurrentRoot(struct rootblock ** , globaldata * );

void GetDriveGeometry(globaldata * );

void RequestCurrentVolumeBack(globaldata *g);
BOOL CheckCurrentVolumeBack(globaldata *g);
