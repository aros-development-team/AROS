/* nt4_dev.h - prototypes for NT4 direct drive access functions
 *
 * Copyright 1999 by Dan Sutherland
 */

HANDLE NT4OpenDrive(char *strDrive);
BOOL NT4CloseDrive(HANDLE hDrv);
BOOL NT4ReadSector(HANDLE hDrv, long iSect, int iSize, void *lpvoidBuf);
BOOL NT4WriteSector(HANDLE hDrv, long iSect, int iSize, void *lpvoidBuf);
ULONG NT4GetDriveSize(HANDLE hDrv);
