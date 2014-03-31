/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/dos.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <dos/exall.h>
#include <exec/memory.h>
#include <stdio.h>
#include <aros/debug.h>

#define REAL_DELETE 1

int deleteDirContents(char *startpath, int depth, int dnum, int fnum) {
struct ExAllControl *eac;
struct TagItem ti={TAG_DONE};
struct ExAllData *ead;
struct ExAllData *next;
BPTR lock;
struct FileInfoBlock fib;
char dpath[512];
int size;
LONG error;

	if (dnum<1)
		dnum = 1;
	size=(dnum+(fnum*32))*(sizeof(struct ExAllData)+32);
	eac = AllocDosObject(DOS_EXALLCONTROL, &ti);
	if (eac == NULL)
	{
		printf("\nFailed to allocated dos object!\n");
		return 1;
	}
	eac->eac_LastKey = 0;
	ead = AllocVec(size, MEMF_ANY | MEMF_CLEAR);
	if (ead == NULL)
	{
		FreeDosObject(DOS_EXALLCONTROL, eac);
		printf("\nFailed to allocated memory!\n");
		return 1;
	}
	lock = Lock(startpath, SHARED_LOCK);
	if (lock == BNULL)
	{
		error = IoErr();
		FreeVec(ead);
		FreeDosObject(DOS_EXALLCONTROL, eac);
		printf("\nFailed to lock %s!\n", startpath);
		printf("I/O Error is %ld\n", (long)error);
		PrintFault(error, NULL);
		return 1;
	}
	Examine(lock, &fib);
	if (fib.fib_DirEntryType != ST_USERDIR)
	{
		error = IoErr();
		UnLock(lock);
		FreeVec(ead);
		FreeDosObject(DOS_EXALLCONTROL, eac);
		printf("\nEntry %s is not directory!\n", startpath);
		printf("I/O Error is %ld\n", (long)error);
		PrintFault(error, NULL);
		return 1;
	}
	if (ExAll(lock, ead, size, ED_TYPE, eac) != 0)
	{
		error = IoErr();
kprintf("entries = %ld\n", eac->eac_Entries);
		ExAllEnd(lock, ead, size, ED_TYPE, eac);
		UnLock(lock);
		FreeVec(ead);
		FreeDosObject(DOS_EXALLCONTROL, eac);
		printf("\nNot enough memory for %s when doing ExamineAll()!\n", startpath);
		printf("I/O Error is %ld\n", (long)error);
		PrintFault(error, NULL);
		return 1;
	}
	error = IoErr();
	UnLock(lock);
	if (error == ERROR_NO_MORE_ENTRIES)
		error = 0;
	else
	{
		FreeDosObject(DOS_EXALLCONTROL, eac);
		FreeVec(ead);
		printf("\nExAll() returned error on %s!\n", startpath);
		printf("I/O Error is %ld\n", (long)error);
		PrintFault(error, NULL);
		return 1;
	}
	if (eac->eac_Entries == 0)
		next = 0;
	else
		next = ead;
	while (next != NULL)
	{
		AddPart(dpath, startpath, 512);
		AddPart(dpath, next->ed_Name, 512);
		if (next->ed_Type == ST_FILE)
		{
#if REAL_DELETE
			if (!DeleteFile(dpath))
			{
				error = IoErr();
				FreeDosObject(DOS_EXALLCONTROL, eac);
				FreeVec(ead);
				printf("\nFailed to delete file %s\n", dpath);
				printf("I/O Error is %ld\n", (long)error);
				PrintFault(error, NULL);
				return 1;
			}
#endif
		}
		else if (next->ed_Type == ST_USERDIR)
		{
			if (deleteDirContents(dpath, depth-1, dnum-1, fnum == 0 ? fnum : fnum-1) != 0)
			{
				FreeDosObject(DOS_EXALLCONTROL, eac);
				FreeVec(ead);
				return 1;
			}
		}
		else
		{
			FreeDosObject(DOS_EXALLCONTROL, eac);
			FreeVec(ead);
			printf("\nFailed to identify %s - it is no directory or file!\n", dpath);
			return 1;
		}
		next = next->ed_Next;
	}
	FreeDosObject(DOS_EXALLCONTROL, eac);
	FreeVec(ead);
#if REAL_DELETE
	if (!DeleteFile(startpath))
	{
		error = IoErr();
		printf("\nFailed to delete directory %s\n", startpath);
		printf("I/O Error is %ld\n", (long)error);
		PrintFault(error, NULL);
		return 1;
	}
#endif
	return 0;
}

int deleteAll(char *startpath, int depth, int dnum, int fnum) {
int i;
char name[32];
char path[512];

	for (i=0; i<dnum; i++)
	{
		sprintf(name, "d-%03d-%03d", depth, i);
		AddPart(path, startpath, 512);
		AddPart(path, name, 512);
		if (deleteDirContents(path, depth, dnum, fnum) != 0)
			return 1;
	}
	return 0;
}

int specificParentCheck(BPTR lock, BPTR dlock, char *path) {
BPTR plock;
LONG error;

	plock = ParentDir(dlock);
	if (plock == BNULL)
	{
		error = IoErr();
		printf("\nFailed to get parent of %s!\n", path);
		printf("I/O Error is %ld\n", (long)error);
		PrintFault(error, NULL);
		return 1;
	}
	if (!SameLock(lock, plock))
	{
		UnLock(plock);
		printf("\nParent of %s is not correct!\n", path);
		return 1;
	}
	UnLock(plock);
	return 0;
}

int checkParent(char *startpath, int depth, int dnum, int fnum, int size) {
BPTR lock;
BPTR dlock;
BPTR flock;
char name[32];
char path[512];
char fpath[512];
int i,j;
LONG error;

	if (dnum<1)
		dnum = 1;
	lock = Lock(startpath, SHARED_LOCK);
	if (lock == BNULL)
	{
		error = IoErr();
		printf("\nFailed to get lock on %s!\n", startpath);
		printf("I/O Error is %ld\n", (long)error);
		PrintFault(error, NULL);
		return 1;
	}
	for (i=0; i<dnum; i++)
	{
		sprintf(name, "d-%03d-%03d", depth, i);
		AddPart(path, startpath, 512);
		AddPart(path, name, 512);
		dlock = Lock(path, SHARED_LOCK);
		if (dlock == BNULL)
		{
			error = IoErr();
			UnLock(lock);
			printf("\nFailed to get lock on %s!\n", path);
			printf("I/O Error is %ld\n", (long)error);
			PrintFault(error, NULL);
			return 1;
		}
		if (specificParentCheck(lock, dlock, path) != 0)
		{
			UnLock(lock);
			UnLock(dlock);
			return 1;
		}
		for (j=0; j<fnum; j++)
		{
			sprintf(name, "f-%03d-%03d-%03d-%08d", depth, i, j, size);
			AddPart(fpath, path, 512);
			AddPart(fpath, name, 512);
			flock = Lock(fpath, SHARED_LOCK);
			if (flock == BNULL)
			{
				error = IoErr();
				UnLock(lock);
				UnLock(dlock);
				printf("\nFailed to get lock on %s!\n", fpath);
				printf("I/O Error is %ld\n", (long)error);
				PrintFault(error, NULL);
				return 1;
			}
			if (specificParentCheck(dlock, flock, fpath) != 0)
			{
				UnLock(lock);
				UnLock(flock);
				UnLock(dlock);
				return 1;
			}
			UnLock(flock);
		}
		if (depth>0)
			if (checkParent(path, depth-1, dnum-1, fnum == 0 ? fnum : fnum-1, size) != 0)
				return 1;
	}
	UnLock(lock);
	return 0;
}

int checkFile(char *path, int depth, int dnum, int fnum, int size) {
BPTR fh;
unsigned int buffer[512];
int i,j;
LONG error;

	fh = Open(path, MODE_OLDFILE);
	if (fh == BNULL)
	{
		error = IoErr();
		printf("\nFailed to open file %s!\n", path);
		printf("I/O Error is %ld\n", (long)error);
		PrintFault(error, NULL);
		return 1;
	}
	for (i=0;i<(size/512);i++)
	{
		if (Read(fh, buffer, 512) != 512)
		{
			error = IoErr();
			printf("\nFailed to read from file %s\n", path);
			printf("I/O Error is %ld\n", (long)error);
			PrintFault(error, NULL);
			return 1;
		}
		for (j=0;j<(512>>4); j+=4)
		{
			if (
					(buffer[j+0] != depth) ||
					(buffer[j+1] != dnum) ||
					(buffer[j+2] != fnum) ||
					(buffer[j+3] != size)
				)
			{
				printf("\nFailed to verify file %s at offset %d\n", path, j*4);
				printf("Expected: %08x %08x %0x %08x\n", depth, dnum, fnum, size);
				printf("Got     : %08x %08x %0x %08x\n", buffer[j+0], buffer[j+1], buffer[j+2], buffer[j+3]);
				return 1;
			}
		}
	}
	Close(fh);
	return 0;
}

int checkFiles(char *startpath, int depth, int dnum, int fnum, int size) {
int i,j;
char name[32];
char path[512];
char fpath[512];

	if (dnum<1)
		dnum = 1;
	for (i=0; i<dnum; i++)
	{
		sprintf(name, "d-%03d-%03d", depth, i);
		AddPart(path, startpath, 512);
		AddPart(path, name, 512);
		for (j=0; j<fnum; j++)
		{
			sprintf(name, "f-%03d-%03d-%03d-%08d", depth, i, j, size);
			AddPart(fpath, path, 512);
			AddPart(fpath, name, 512);
			if (checkFile(fpath, depth, dnum, fnum, size) != 0)
				return 1;
		}
		if (depth>0)
			if (checkFiles(path, depth-1, dnum-1, fnum == 0 ? fnum : fnum-1, size) != 0)
				return 1;
	}
	return 0;
}

int writeFile(char *path, int size, int depth, int dnum, int fnum) {
BPTR fh;
unsigned int buffer[512];
int i;
LONG error;

	fh = Open(path, MODE_NEWFILE);
	if (fh == BNULL)
	{
		error = IoErr();
		printf("\nFailed to create file %s!\n", path);
		printf("I/O Error is %ld\n", (long)error);
		PrintFault(error, NULL);
		return 1;
	}
	for (i=0;i<(512>>4); i+=4)
	{
		buffer[i+0] = depth;
		buffer[i+1] = dnum;
		buffer[i+2] = fnum;
		buffer[i+3] = size;
	}
	for (i=0;i<(size/512);i++)
	{
		if (Write(fh, buffer, 512) != 512)
		{
			error = IoErr();
			Close(fh);
			printf("Failed to write to file %s\n", path);
			printf("I/O Error is %ld\n", (long)error);
			PrintFault(error, NULL);
			return 1;
		}
	}
	Close(fh);
//	printf("Verifying ...");
	if (checkFile(path, size, depth, dnum, fnum) != 0)
		return 1;
//	printf("done\n");
	return 0;
}

int createFiles(char *startpath, int depth, int dnum, int fnum, int size) {
int i,j;
char name[32];
char path[512];
char fpath[512];

	if (dnum<1)
		dnum = 1;
	for (i=0; i<dnum; i++)
	{
		sprintf(name, "d-%03d-%03d", depth, i);
		AddPart(path, startpath, 512);
		AddPart(path, name, 512);
		for (j=0; j<fnum; j++)
		{
			sprintf(name, "f-%03d-%03d-%03d-%08d", depth, i, j, size);
			AddPart(fpath, path, 512);
			AddPart(fpath, name, 512);
			if (writeFile(fpath, size, depth, dnum, fnum) != 0)
				return 1;
		}
		if (depth>0)
			if (createFiles(path, depth-1, dnum-1, fnum == 0 ? fnum : fnum-1, size) != 0)
				return 1;
	}
	return 0;
}

int checkDirs(char *startpath, int depth, int num) {
BPTR dir;
int i;
char name[32];
char path[512];
LONG error;

	if (num<1)
		num = 1;
	for (i=0; i<num; i++)
	{
		sprintf(name, "d-%03d-%03d", depth, i);
		AddPart(path, startpath, 512);
		AddPart(path, name, 512);
		dir = Lock(path, SHARED_LOCK);
		if (dir == BNULL)
		{
			error = IoErr();
			printf("\nFailed locking %s!\n",path);
			printf("I/O Error is %ld\n", (long)error);
			PrintFault(error, NULL);
			return 1;
		}
		UnLock(dir);
		if (depth>0)
			if (checkDirs(path, depth-1, num-1) != 0)
				return 1;
	}
	return 0;
}

int createDirs(char *startpath, int depth, int num) {
BPTR dir;
int i;
char name[32];
char path[512];
LONG error;

	if (num<1)
		num = 1;
	for (i=0; i<num; i++)
	{
		sprintf(name, "d-%03d-%03d", depth, i);
		AddPart(path, startpath, 512);
		AddPart(path, name, 512);
		dir = CreateDir(path);
		if (dir == BNULL)
		{
			error = IoErr();
			printf("\nFailed to create %s!\n", path);
			printf("I/O Error is %ld\n", (long)error);
			PrintFault(error, NULL);
			return 1;
		}
		UnLock(dir);
		if (depth>0)
			if (createDirs(path, depth-1, num-1) != 0)
				return 1;
	}
	return 0;
}

int verifyFiles(char *startpath, int depth, int dnum, int fnum, int size) {
	printf("Verifying %d files per depth with size of %d bytes ...", fnum, size);
	if (checkFiles(startpath, depth, dnum, fnum, size) != 0)
		return 1;
	printf("done\n");
	return 0;
}

int getDiskInfo(char *device, struct InfoData *id) {
BPTR lock;

	lock = Lock(device, SHARED_LOCK);
	if (lock == BNULL)
	{
		printf("Failed to get lock on %s!\n", device);
		return 1;
	}
	Info(lock, id);
	UnLock(lock);
	return 0;
}

int fileTest(char *startpath, int depth, int dnum, int fnum, int isize) {
int size=isize*1024;
int csize;
int cfnum;
int i;

	for (i=0;i<5;i++)
	{
		printf("Creating %d files per depth with size of %d bytes ...", fnum, size);
		if (createFiles(startpath, depth, dnum, fnum, size) != 0)
			return 1;
		printf("done\n");
		csize = size;
		cfnum = fnum;
		while (csize>=1024)
		{
			if (verifyFiles(startpath, depth, dnum, cfnum, csize) != 0)
				return 1;
			csize = csize>>3;
			cfnum = cfnum<<1;
		}
		size = size<<3;
		fnum = fnum>>1;
	}
	return 0;
}

#define TEST 1
#define TEST_PARENT 0

int main(int argc, char *argv[]) {
int isize=1;  /* initial size in 1024 byte */
int depth=10; /* directory depth */
int dnum=6;   /* number of directories per depth*/
int fnum=16;  /* number of files per depth (the bigger the files the lesser are created) */
struct InfoData sid;
struct InfoData mid;
struct InfoData eid;
int nomid=0;

	if (argc<2)
	{
		printf("Usage: %s <device>\n", argv[0]);
		return 1;
	}
#if TEST
	if (getDiskInfo(argv[1], &sid) != 0)
		return 1;
	printf("Directory test\n");
	printf("==============\n");
	printf("Creating directories ...");
	if (createDirs(argv[1], depth, dnum) != 0)
		return 1;
	printf("done\n");
	printf("Checking directories ...");
	if (checkDirs(argv[1], depth, dnum) != 0)
		return 1;
	printf("done\n");
	printf("\n\n");
	printf("File test\n");
	printf("=========\n");
	if (fileTest(argv[1], depth, dnum, fnum, isize) != 0)
		return 1;
#if TEST_PARENT
	printf("Doing a parent check ...");
	if (checkParent(argv[1], depth, dnum, fnum,) != 0)
		return 1;
	printf("done\n");
#endif
	if (getDiskInfo(argv[1], &mid) != 0)
		nomid=1;
#endif
	printf("Deleting files and directories created by this test ...");
	if (deleteAll(argv[1], depth, dnum, fnum) != 0)
		return 1;
	printf("done\n");
#if TEST
	printf("Used blocks before test: %ld\n", (long)sid.id_NumBlocksUsed);
	if (nomid == 0)
		printf("Used blocks using test: %ld\n", (long)mid.id_NumBlocksUsed);
	if (getDiskInfo(argv[1], &eid) == 0)
		printf("Used blocks after test: %ld\n", (long)eid.id_NumBlocksUsed);
#endif
	return 0;
}
