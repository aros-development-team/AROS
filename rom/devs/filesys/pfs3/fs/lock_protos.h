/* Prototypes for functions defined in
lock.c
 */

struct listentry * MakeListEntry(union objectinfo * , listtype , SIPTR *error, globaldata * );

BOOL _AddListEntry(listentry_t *, globaldata * );
#define AddListEntry(a) _AddListEntry(a,g)

void RemoveListEntry(listentry_t * , globaldata * );

void FreeListEntry(listentry_t * , globaldata * );
//void FreeListEntry(listentry_t *);

BOOL _ChangeAccessMode(listentry_t * , LONG , SIPTR *, globaldata * );
#define ChangeAccessMode(a,b,c) _ChangeAccessMode(a,b,c,g)

BOOL AccessConflict(listentry_t * );

BOOL ScanLockList(listentry_t * , ULONG );

#if MULTIUSER

ULONG muFS_CheckDeleteAccess (ULONG protection, ULONG flags, globaldata *g);

ULONG muFS_CheckWriteAccess (ULONG protection, ULONG flags, globaldata *g);

ULONG muFS_CheckReadAccess (ULONG protection, ULONG flags, globaldata *g);

#endif
