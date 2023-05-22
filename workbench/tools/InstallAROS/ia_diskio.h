#ifndef IA_DISKIO_H
#define IA_DISKIO_H

extern void AddSkipListEntry(struct List *SkipList, char *SkipEntry);
extern void ClearSkipList(struct List *SkipList);
extern BPTR RecursiveCreateDir(CONST_STRPTR dirpath);
extern LONG CopyDirArray(Class * CLASS, Object * self, CONST_STRPTR sourcePath,
    CONST_STRPTR destinationPath, CONST_STRPTR directories[], struct List *SkipList);

#endif /* IA_DISKIO_H */
