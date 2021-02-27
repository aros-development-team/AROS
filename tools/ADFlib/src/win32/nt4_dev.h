/* nt4_dev.h - prototypes for NT4 direct drive access functions
 *
 * Copyright 1999 by Dan Sutherland
 *
 *  This file is part of ADFLib.
 *
 *  ADFLib is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  ADFLib is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Foobar; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

HANDLE NT4OpenDrive(char *strDrive);
BOOL NT4CloseDrive(HANDLE hDrv);
BOOL NT4ReadSector(HANDLE hDrv, long iSect, int iSize, void *lpvoidBuf);
BOOL NT4WriteSector(HANDLE hDrv, long iSect, int iSize, void *lpvoidBuf);
ULONG NT4GetDriveSize(HANDLE hDrv);
