/* Win32/adf_nativ.c - Win32 specific drive-access routines for ADFLib
 *
 * Modified for Win32 by Dan Sutherland <dan@chromerhino.demon.co.uk>
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
 *
 */
 

/* Modified 29/8/00 by Gary Harris.
** - Added a third, Boolean argument to Win32InitDevice() to avoid a compilation warning
**   caused by the mismatch with the number of arguments in ADFLib's adfInitDevice().
*/

#include <windows.h>
#include <stdlib.h>
#include <string.h>

#include "../adf_str.h"
#include "../adf_err.h"

#include "adf_nativ.h"
#include "nt4_dev.h"

extern struct Env adfEnv;

RETCODE Win32InitDevice(struct Device* dev, char* lpstrName, BOOL ro)
{
	struct nativeDevice* nDev;
	char strTempName[3];

	nDev = (struct nativeDevice*)dev->nativeDev;

	nDev = (struct nativeDevice*)malloc(sizeof(struct nativeDevice));
	if (!nDev) {
		(*adfEnv.eFct)("Win32InitDevice : malloc");
		return RC_ERROR;													/* BV */
	}

	/* convert device name to something usable by Win32 functions */
	if (strlen(lpstrName) != 3) {
		(*adfEnv.eFct)("Win32InitDevice : invalid drive specifier");
		return RC_ERROR;													/* BV */
	}

	strTempName[0] = lpstrName[1];
	strTempName[1] = lpstrName[2];
	strTempName[2] = '\0';

	nDev->hDrv = NT4OpenDrive(strTempName);

	if (nDev->hDrv == NULL) {
		(*adfEnv.eFct)("Win32InitDevice : NT4OpenDrive");
		return RC_ERROR;													/* BV */
	}

	dev->size = NT4GetDriveSize(nDev->hDrv);

	dev->nativeDev = nDev;

	return RC_OK;
}


RETCODE Win32ReadSector(struct Device *dev, long n, int size, unsigned char* buf)
{
	struct nativeDevice* tDev;

	tDev = (struct nativeDevice*)dev->nativeDev;

	if (! NT4ReadSector(tDev->hDrv, n, size, buf)) {
		(*adfEnv.eFct)("Win32InitDevice : NT4ReadSector");
		return RC_ERROR;													/* BV */
	}

	return RC_OK;
}


RETCODE Win32WriteSector(struct Device *dev, long n, int size, unsigned char* buf)
{
	struct nativeDevice* tDev;

	tDev = (struct nativeDevice*)dev->nativeDev;

	if (! NT4WriteSector(tDev->hDrv, n, size, buf)) {
		(*adfEnv.eFct)("Win32InitDevice : NT4WriteSector");
		return RC_ERROR;													/* BV */
	}

	return RC_OK;
}


RETCODE Win32ReleaseDevice(struct Device *dev)
{
	struct nativeDevice* nDev;

	nDev = (struct nativeDevice*)dev->nativeDev;

	if (! NT4CloseDrive(nDev->hDrv))
		return RC_ERROR;													/* BV */

	free(nDev);

	return RC_OK;
}


void adfInitNativeFct()
{
	struct nativeFunctions *nFct;

	nFct = (struct nativeFunctions*)adfEnv.nativeFct;

	nFct->adfInitDevice = Win32InitDevice;
	nFct->adfNativeReadSector = Win32ReadSector;
	nFct->adfNativeWriteSector = Win32WriteSector;
	nFct->adfReleaseDevice = Win32ReleaseDevice;
	nFct->adfIsDevNative = Win32IsDevNative;
}


BOOL Win32IsDevNative(char *devName)
{
	return devName[0] == '|';
}
