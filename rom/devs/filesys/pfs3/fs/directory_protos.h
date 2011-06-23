/* Prototypes for functions defined in
directory.c
 */

void *AllocPooledVec(ULONG size, globaldata *g);
void FreePooledVec(void *mem, globaldata *g);
void *AllocPooledBuf(ULONG size, globaldata *g);
void FreePooledBuf(void *mem, globaldata *g);
// void *AllocMemV38(ULONG size, globaldata *g);
// void *AllocBufmemV38(ULONG size, globaldata *g);
// void FreeMemV38(void *mem, globaldata *g);
void *AllocMemPR(ULONG size, globaldata *g);
void *AllocBufmemR(ULONG size, globaldata *g);
void *AllocMemR(ULONG , ULONG , globaldata *g);

BOOL dstricmp(DSTR , STRPTR );
BOOL ddstricmp(DSTR , DSTR );
UBYTE * BCPLtoCString(STRPTR , DSTR );

UBYTE * GetFullPath(union objectinfo * , STRPTR , union objectinfo * , ULONG * , globaldata * );

BOOL GetRoot(union objectinfo * , globaldata * );

BOOL FindObject(union objectinfo * , STRPTR , union objectinfo * , ULONG * , globaldata * );

BOOL GetParent(union objectinfo * , union objectinfo * , ULONG * , globaldata * );

BOOL FetchObject(ULONG diranodenr, ULONG target, union objectinfo *result, globaldata *g);

BOOL ExamineFile(listentry_t * , struct FileInfoBlock * , ULONG * , globaldata * );

BOOL ExamineNextFile(lockentry_t * , struct FileInfoBlock * , ULONG * , globaldata * );

void GetNextEntry(lockentry_t * , globaldata * );

BOOL ExamineAll(lockentry_t * , UBYTE * , ULONG , LONG , struct ExAllControl * , ULONG * , globaldata * );

ULONG NewFile(BOOL, union objectinfo * , STRPTR , union objectinfo * , globaldata * );

lockentry_t * NewDir(union objectinfo * , STRPTR , ULONG * , globaldata * );

struct cdirblock * MakeDirBlock(ULONG , ULONG , ULONG , ULONG , globaldata * );

BOOL DeleteObject(union objectinfo * , ULONG * , globaldata * );
BOOL KillEmpty (union objectinfo *parent, globaldata *g);
LONG forced_RemoveDirEntry (union objectinfo *info, ULONG *error, globaldata *g);

BOOL RenameAndMove(union objectinfo *, union objectinfo *, union objectinfo *, STRPTR , ULONG * , globaldata * );

BOOL AddComment(union objectinfo * , STRPTR , ULONG * , globaldata * );

BOOL ProtectFile(struct fileinfo * , ULONG , ULONG * , globaldata * );

BOOL SetOwnerID(struct fileinfo *file, ULONG owner, ULONG *error, globaldata *g);

LONG ReadSoftLink(union objectinfo *linkfi, char *buffer, ULONG size, ULONG *error, globaldata *g);

BOOL CreateSoftLink(union objectinfo *linkdir, STRPTR linkname, STRPTR softlink,
	union objectinfo *newlink, ULONG *error, globaldata *g);

BOOL CreateLink(union objectinfo *directory, STRPTR linkname, union objectinfo *object,
	union objectinfo *newlink, ULONG *error, globaldata *g);

BOOL SetDate(union objectinfo * , struct DateStamp * , ULONG * , globaldata * );

void Touch(struct fileinfo * , globaldata * );

BOOL CreateRollover(union objectinfo *dir, STRPTR rollname, ULONG size,
	union objectinfo *result, ULONG *error, globaldata *g);
ULONG SetRollover(fileentry_t *rooi, struct rolloverinfo *roinfo, globaldata *g);

void ChangeDirEntry(struct fileinfo from, struct direntry *to, union objectinfo *destdir, struct fileinfo *result, globaldata *g);

struct cdirblock * LoadDirBlock(ULONG , globaldata * );

void GetExtraFields(struct direntry *direntry, struct extrafields *extrafields);
void AddExtraFields (struct direntry *direntry, struct extrafields *extra);
#if MULTIUSER
#if DELDIR
void GetExtraFieldsOI(union objectinfo *info, struct extrafields *extrafields, globaldata *g);
#endif
#endif

#if DELDIR
struct cdeldirblock *NewDeldirBlock(UWORD seqnr, globaldata *g);
struct deldirentry *GetDeldirEntryQuick(ULONG ddnr, globaldata *g);
ULONG SetDeldir(int nbr, globaldata *g);
#endif
void UpdateLinks(struct direntry *object, globaldata *g);
void FreeAnodesInChain(ULONG anodenr, globaldata *g);
