/* An ADFLib function for Windows which displays a dialogue box containing details about
** the current disk file. 
** Converted from adfVolumeInfo(), part of ADFLib by Laurent Clevy.
**
** Gary Harris
** 30/8/00
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


#include "../ADF_raw.h"
#include "../ADF_Util.h"


void adfVolumeInfoWin(HWND hWnd, struct Volume *vol)
/* Windows version of adfVolumeInfo().
** Input:  Receives a handle to the window on which to display the dialogue and a pointer
**         to a ADFLib Volume structure.
** Output: Nil. Displays a windows dialogue containing the disk file data.
*/
{
	char	szTemp[50], szAdfInfo[500];		/* Info string. */
	
	struct	bRootBlock root;
	char	diskName[35];
	int		days,month,year;
	
	if (adfReadRootBlock(vol, vol->rootBlock, &root)!=RC_OK)
		return;
	
	memset(diskName, 0, 35);
	memcpy(diskName, root.diskName, root.nameLen);
	
	sprintf(szAdfInfo, "Name : %-30s\n", vol->volName);
	strcat(szAdfInfo, "Type : ");
	switch(vol->dev->devType) {
		case DEVTYPE_FLOPDD:
			strcat(szAdfInfo, "Floppy Double Density : 880 KBytes\n");
			break;
		case DEVTYPE_FLOPHD:
			strcat(szAdfInfo, "Floppy High Density : 1760 KBytes\n");
			break;
		case DEVTYPE_HARDDISK:
			sprintf(szTemp, "Hard Disk partition : %3.1f KBytes\n", 
				(vol->lastBlock - vol->firstBlock +1) * 512.0/1024.0);
			strcat(szAdfInfo, szTemp);
			break;
		case DEVTYPE_HARDFILE:
			sprintf(szTemp, "HardFile : %3.1f KBytes\n", 
				(vol->lastBlock - vol->firstBlock +1) * 512.0/1024.0);
			strcat(szAdfInfo, szTemp);
			break;
		default:
			strcat(szAdfInfo, "Unknown devType!\n");
	}
	strcat(szAdfInfo, "Filesystem : ");
	sprintf(szTemp, "%s ",isFFS(vol->dosType) ? "FFS" : "OFS");
	strcat(szAdfInfo, szTemp);
	if (isINTL(vol->dosType))
		strcat(szAdfInfo, "INTL ");
	if (isDIRCACHE(vol->dosType))
		strcat(szAdfInfo, "DIRCACHE ");
	strcat(szAdfInfo, "\n");

    sprintf(szTemp, "Free blocks = %ld\n", adfCountFreeBlocks(vol));
	strcat(szAdfInfo, szTemp);
	if (vol->readOnly)
		strcat(szAdfInfo, "Read only\n");
    else
		strcat(szAdfInfo, "Read/Write\n");
 	
    /* created */
	adfDays2Date(root.coDays, &year, &month, &days);
	sprintf(szTemp, "created %d/%02d/%02d %ld:%02ld:%02ld\n",days,month,year,
	    root.coMins/60,root.coMins%60,root.coTicks/50);
	strcat(szAdfInfo, szTemp);
	adfDays2Date(root.days, &year, &month, &days);
	sprintf(szTemp, "last access %d/%02d/%02d %ld:%02ld:%02ld,   ",days,month,year,
	    root.mins/60,root.mins%60,root.ticks/50);
	strcat(szAdfInfo, szTemp);
	adfDays2Date(root.cDays, &year, &month, &days);
	sprintf(szTemp, "%d/%02d/%02d %ld:%02ld:%02ld\n",days,month,year,
	    root.cMins/60,root.cMins%60,root.cTicks/50);
	strcat(szAdfInfo, szTemp);

	MessageBox(hWnd, szAdfInfo, "Adf Info", MB_OK);
}

