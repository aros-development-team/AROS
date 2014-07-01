
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include <dos/filehandler.h>

#include "pfs3.h"
#include "doctor.h"

enum mode mode = repair;	/* check, repair, unformat or search */

struct Window *CheckRepairWnd;
struct stats stats;

struct
{
	char name[200];
} targetdevice;
static BOOL verbose = FALSE;
FILE *logfh = NULL;
static BOOL directscsi = FALSE;
static BOOL inhibited = FALSE;

static const char *accessmodes[] = { "", "Standard", "Direct SCSI", "TD64", "NSD" };

void dummyMsg(char *message)
{
}

void guiMsg(const char *format, ...)
{
	va_list parms;
	va_start (parms, format);
	vprintf (format, parms);
	va_end (parms);
}

void guiUpdateStats(void)
{
}

void guiStatus(int level, char *message, long maxval)
{
	printf("%s...\n", message);
}

void guiProgress(int level, long progress)
{
}

int guiAskUser(char *message, char *okstr, char *cancelstr)
{
	char ch = 0;
	for (;;) {
		printf("%s\n", message);
		printf("1=<%s> 0=<%s>\n", okstr, cancelstr);
		scanf("%c", &ch);
		if (ch == '1')
			return 1;
		if (ch == '0')
			return 0;
	}
}

#define OV_Flags LDF_DEVICES|LDF_VOLUMES
static BOOL OpenVolume(void)
{
	struct DosList *dl;
	ULONG cylsectors, b;
	BOOL t;
	int i;
	UBYTE *detectbuf;

	/* get device doslist */
	dl = LockDosList (OV_Flags|LDF_READ);
	dl = FindDosEntry(dl, targetdevice.name, OV_Flags);

	if (dl && dl->dol_Type == DLT_VOLUME)
	{
		struct DosList *dl2;
		struct DosInfo *di;
		
		di = (struct DosInfo *)BADDR(((struct RootNode *)DOSBase->dl_Root)->rn_Info);
		for (dl2 = (struct DosList *)BADDR(di->di_DevInfo);
			 dl2;
			 dl2 = (struct DosList *)BADDR(dl2->dol_Next))
		{
			if (dl2->dol_Type == DLT_DEVICE && dl->dol_Task == dl2->dol_Task)
			{
				unsigned char *dname;

				dl = dl2;
				dname = (char *)BADDR(dl->dol_Name);
				strncpy(targetdevice.name, dname+1, *dname);
				targetdevice.name[*dname] = 0;
				break;
			}
		}
	}

	if (!dl || dl->dol_Type == DLT_VOLUME)
	{
		UnLockDosList(OV_Flags|LDF_READ);
		guiMsg("DEVICE "); guiMsg(targetdevice.name);
		guiMsg(" not found\nEXITING ...\n\n");
		return FALSE;
	}

	UnLockDosList(OV_Flags|LDF_READ);

	/* inhibit device */
	targetdevice.name[strlen(targetdevice.name)] = ':';
	targetdevice.name[strlen(targetdevice.name)] = 0;
	if (!(inhibited = Inhibit(targetdevice.name, DOSTRUE)))
	{
		guiMsg("Device could not be inhibited.\nEXITING ...\n\n");
		return FALSE;
	}

	/* init volume structure */
	memset(&volume, 0, sizeof(volume));
	volume.fssm = (struct FileSysStartupMsg *)BADDR(dl->dol_misc.dol_handler.dol_Startup);
	volume.dosenvec = (struct DosEnvec *)BADDR(volume.fssm->fssm_Environ);
	strcpy(volume.devicename, targetdevice.name);
	cylsectors = volume.dosenvec->de_Surfaces * volume.dosenvec->de_BlocksPerTrack;
	volume.firstblock = volume.dosenvec->de_LowCyl * cylsectors;
	volume.lastblock = (volume.dosenvec->de_HighCyl + 1) * cylsectors - 1;
	b = volume.blocksize = volume.dosenvec->de_SizeBlock << 2;
	for (i=-1; b; i++)
		b >>= 1;
	volume.blockshift = i;
	volume.rescluster = 0;
	volume.disksize = volume.lastblock - volume.firstblock + 1;
	volume.lastreserved = volume.disksize - 256;	/* temp value, calculated later */

	volume.status = guiStatus;
	volume.showmsg = guiMsg;
	volume.askuser = guiAskUser;
	volume.progress = guiProgress;
	volume.updatestats = guiUpdateStats;
	volume.getblock = vol_GetBlock;
	volume.writeblock = vol_WriteBlock;
	BCPLtoCString(volume.execdevice, (UBYTE *)BADDR(volume.fssm->fssm_Device));
	volume.execunit = volume.fssm->fssm_Unit;

	if (verbose) {
		UBYTE name[FNSIZE];
		BCPLtoCString(name, (UBYTE *)BADDR(volume.fssm->fssm_Device));
		volume.showmsg("Device: %s:%lu\n", name, volume.fssm->fssm_Unit);
		volume.showmsg("Firstblock: %lu\n", volume.firstblock);
		volume.showmsg("Lastblock : %lu\n", volume.lastblock);
		volume.showmsg("Blocksize : %lu\n", volume.blocksize);
	}

	/* open device */
	if (!OpenDiskDevice(volume.fssm, &volume.port, &volume.request, &t))
	{
		guiMsg("Device could not be opened.\nEXITING ...\n\n");
		return FALSE;
	}

	InitCache(64, 32);		/* make this configurable ? */

	detectbuf = AllocVec(volume.blocksize, MEMF_PUBLIC);
	if (!detectbuf) {
		printf("Could not allocated %ld byte buffer.\n", volume.blocksize);
		return FALSE;
	}
	if (!DetectAccessmode(detectbuf, directscsi)) {
		printf("PFSDoctor failed to access this disk\n"
				"above the 4G boundary after attempting\n"
				"TD64, NSD and Direct SCSI\n");
		FreeVec(detectbuf);
		return FALSE;
	}
	FreeVec(detectbuf);
	printf("Autodetected disk access mode: %s\n", accessmodes[volume.accessmode]);

	if (volume.accessmode == ACCESS_DS)
	{
		volume.getrawblocks = dev_GetBlocksDS;
		volume.writerawblocks = dev_WriteBlocksDS;
	}
	else
	{
		if (volume.accessmode == ACCESS_TD64)
			volume.td64mode = TRUE;
		else if (volume.accessmode == ACCESS_NSD)
			volume.nsdmode = TRUE;
		volume.getrawblocks = dev_GetBlocks;
		volume.writerawblocks = dev_WriteBlocks;
	}

	if (mode == check)
		volume.writerawblocks = dev_WriteBlocksDummy;

	return TRUE;
}

static void CloseVolume(void)
{
	FreeCache();

	if (inhibited)
		Inhibit(targetdevice.name, FALSE);

	if (volume.request)
	{
		if (!(CheckIO((struct IORequest *)volume.request)))
			AbortIO((struct IORequest *)volume.request);
		
		WaitIO((struct IORequest *)volume.request);
		CloseDevice((struct IORequest *)volume.request);
	}

	if (volume.request)
		DeleteIORequest(volume.request);

	if (volume.port)
		DeleteMsgPort(volume.port);

	volume.request = NULL;
	volume.port = NULL;
}

#define TEMPLATE "DEVICE/A,CHECK/S,REPAIR/S,SEARCH/S,UNFORMAT/S,VERBOSE/S,LOGFILE/K,DIRECTSCSI/S"

#define ARGS_DEVICE 0
#define ARGS_CHECK 1
#define ARGS_REPAIR 2
#define ARGS_SEARCH 3
#define ARGS_UNFORMAT 4
#define ARGS_VERBOSE 5
#define ARGS_LOGFILE 6
#define ARGS_DIRECTSCSI 7
#define ARGS_SIZE 8

int main(int argc, char *argv[])
{
	struct RDArgs *rdarg;
	LONG args[ARGS_SIZE] =  { 0 };
	int cnt = 0;
	uint32 opties;

	if (!(rdarg = ReadArgs (TEMPLATE, args, NULL)))
	{
		PrintFault (ERROR_REQUIRED_ARG_MISSING, "pfsdoctor");
		return RETURN_FAIL;
	}

	strcpy(targetdevice.name, (char*)args[ARGS_DEVICE]);

	if (args[ARGS_VERBOSE])
		verbose = TRUE;

	if (args[ARGS_CHECK]) {
		mode = check;
		cnt++;
	}
	if (args[ARGS_REPAIR]) {
		mode = repair;
		cnt++;
	}
	if (args[ARGS_SEARCH]) {
		mode = search;
		cnt++;
	}
	if (args[ARGS_UNFORMAT]) {
		mode = unformat;
		cnt++;
	}
	
	if (args[ARGS_DIRECTSCSI]) {
		directscsi = TRUE;
	}
	
	if (cnt == 0) {
		printf("CHECK, REPAIR, SEARCH or UNFORMAT required.\n");
		return RETURN_FAIL;
	}
	if (cnt > 1) {
		printf("Only one command (CHECK, REPAIR, SEARCH, UNFORMAT) parameter allowed.\n");
		return RETURN_FAIL;
	}
	
	if (args[ARGS_LOGFILE]) {
		logfh = fopen((char*)args[ARGS_LOGFILE], "w");
		if (!logfh) {
			printf("Could not open log file '%s'\n", (char*)args[ARGS_LOGFILE]);
			return RETURN_FAIL;
		}
	}

	if (mode == repair)
		opties = SSF_FIX|SSF_ANALYSE|SSF_GEN_BMMASK;
	else if (mode == unformat)
		opties = SSF_UNFORMAT|SSF_FIX|SSF_ANALYSE|SSF_GEN_BMMASK;
	else
		opties = SSF_CHECK|SSF_ANALYSE|SSF_GEN_BMMASK;
		
	if (verbose)
		opties |= SSF_VERBOSE;

	if (OpenVolume()) {
		StandardScan(opties);
		CloseVolume();
	}

	if (logfh)
		fclose(logfh);

	FreeArgs(rdarg);
	return 0;
}
