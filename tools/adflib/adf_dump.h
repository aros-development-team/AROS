#ifndef ADF_DUMP_H
#define ADF_DUMP_H 1

/*
 *  ADF Library. (C) 1997-1998 Laurent Clevy
 *
 *  adf_dump.h
 *
 *  
 */

PREFIX     struct Device*
adfCreateDumpDevice(char* filename, long cyl, long heads, long sec);
PREFIX RETCODE adfCreateHdFile(struct Device* dev, char* volName, int volType);
BOOL adfInitDumpDevice(struct Device* dev, char* name,BOOL);
BOOL adfReadDumpSector(struct Device *dev, SECTNUM n, int size, unsigned char* buf);
BOOL adfWriteDumpSector(struct Device *dev, SECTNUM n, int size, unsigned char* buf);
void adfReleaseDumpDevice(struct Device *dev);


#endif /* ADF_DUMP_H */
/*##########################################################################*/
