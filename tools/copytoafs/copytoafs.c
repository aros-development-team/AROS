#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "os.h"
#include "filehandles1.h"
#include "filehandles2.h"
#include "misc.h"
#include "volumes.h"

ULONG error;

struct PathElement {
	struct PathElement *next;
	char *path;
};

struct Config {
	char *image;
	struct PathElement *first;
	char *name;
	unsigned int size;
	unsigned int bootblocks;
	unsigned int reserved;
	unsigned int type;
};

char *filepart(char *path) {
char *ptr;

	ptr = path+strlen(path);
	while ((ptr != path) && (*ptr != '/'))
		ptr--;
	if (ptr != path)
		ptr++;
	return ptr;
}

int copyFile(char *srcpath, char *dstpath, struct Volume *volume) {
int fd;
char buffer[2048];
int retval = 1;
char *filename;
struct AfsHandle *ah;
struct AfsHandle *fah;
int len;
int written;
int size;
struct stat st;

	stat(srcpath, &st);
	filename = filepart(srcpath);
	printf("Copying %s to %s ...", filename, dstpath);
	ah = openf(NULL, &volume->ah, dstpath, FMF_READ);
	if (ah != NULL)
	{
		fd = open(srcpath, O_RDONLY);
		if (fd != -1)
		{
			fah = openfile(NULL, ah, filename, FMF_READ | FMF_WRITE | FMF_CREATE | FMF_LOCK | FMF_CLEAR, FIBF_WRITE | FIBF_READ);
			if (fah != NULL)
			{
				written=0;
				while ((len=read(fd, buffer, 2048))>0)
				{
					size = writef(NULL, fah, buffer, len);
					written += size;
					if (size<len)
					{
						retval = 2;
						break;
					}
				}
				if (retval == 2)
				{
					retval = 1;
					if (error == ERROR_NO_FREE_STORE)
						printf("No more space left on device!\nNeed %ld more bytes to write file.\n", st.st_size-written);
					else
						printf("%s: error %ld\n", filename, error);
				}
				else
				{
					printf("done\n");
					retval = 0;
				}
				closef(NULL, fah);
			}
			else
				printf("error %ld\n", error);
			close(fd);
		}
		else
			perror(srcpath);
		closef(NULL, ah);
	}
	else
		printf("%s: error %ld\n", dstpath, error);
	return retval;
}

int makeDir(char *dirname, struct Volume *volume) {
int retval = 1;
struct AfsHandle *ah;
struct AfsHandle *dah;

	printf("Creating directory %s ...", dirname);
	ah = openf(NULL, &volume->ah, "", FMF_READ);
	if (ah != NULL)
	{
		dah = createDir(NULL, ah, dirname, 0);
		if (dah != NULL)
		{
			closef(NULL, dah);
			printf("done\n");
			retval = 0;
		}
		else
		{
			if (error == ERROR_OBJECT_EXISTS)
			{
				printf("was already there\n");
				retval = 0;
			}
			else
				printf("error %ld\n", error);
		}
		closef(NULL, ah);
	}
	else
		printf("error %ld\n", error);
	return retval;
}

int copyDir(char *path, char *dstpath, struct Volume *volume) {
int retval = 1;
int error;
DIR *dir;
struct dirent *de;
char *ndpath;
char *nrpath;

	dir = opendir(path);
	if (dir != NULL)
	{
		while ((de=readdir(dir)) != NULL)
		{
			if ((strcmp(de->d_name, ".")!=0) && (strcmp(de->d_name, "..")!=0))
			{
				if (de->d_type == DT_DIR)
				{
					ndpath = malloc(strlen(path)+1+strlen(de->d_name)+1);
					nrpath = malloc(strlen(path)+1+strlen(de->d_name)+1);
					if ((ndpath != NULL) && (nrpath != NULL))
					{
						sprintf(ndpath, "%s/%s", path, de->d_name);
						if (*dstpath == 0)
							strcpy(nrpath, de->d_name);
						else
							sprintf(nrpath, "%s/%s", dstpath, de->d_name);
						error = makeDir(nrpath, volume);
						if (error == 0)
							error = copyDir(ndpath, nrpath, volume);
						free(nrpath);
						free(ndpath);
						if (error != 0)
						{
							retval = 2;
							break;
						}
					}
					else
					{
						if (nrpath != NULL)
							free(nrpath);
						if (ndpath != NULL)
							free(ndpath);
						printf("No memory!\n");
						retval = 2;
						break;
					}
				}
				else if (de->d_type == DT_REG)
				{
					ndpath = malloc(strlen(path)+1+strlen(de->d_name)+1);
					if (ndpath != NULL)
					{
						sprintf(ndpath, "%s/%s", path, de->d_name);
						error = copyFile(ndpath, dstpath, volume);
						free(ndpath);
						if (error != 0)
						{
							retval = 2;
							break;
						}
					}
					else
					{
						printf("No memory!\n");
						retval = 2;
						break;
					}
				}
				else
				{
					printf("%s/%s: Unkown filetype\n", path, de->d_name);
					retval = 2;
					break;
				}
			}
		}
		if (retval == 2)
			retval = 1;
		else
			retval = 0;
		closedir(dir);
	}
	else
		perror(path);
	return retval;
}

int copyPath(char *path, struct Volume *volume) {
struct stat st;

	if (stat(path, &st) == 0)
	{
		if (S_ISDIR(st.st_mode))
		{
			return copyDir(path, "", volume);
		}
		else if (S_ISREG(st.st_mode))
		{
			/* for now always copy to root */
			return copyFile(path, "", volume);
		}
		else
			printf("Unknown file type\n");
	}
	else
		perror(NULL);
	return 1;
}

int copyData(struct Config *cfg, struct Volume *volume) {
struct PathElement *pe;

	pe = cfg->first;
	while (pe != NULL)
	{
		if (copyPath(pe->path, volume) != 0)
			return 1;
		pe = pe->next;
	}
	return 0;
}

int fillFile(char *image, unsigned int size) {
char buffer[512]={0};
FILE *fh;
int retval=0;
int i;

	fh = fopen(image, "w");
	if (fh != NULL)
	{
		for (i=0;i<size;i++)
		{
			if (fwrite(buffer, 512, 1, fh) != 1)
			{
				retval = 1;
				break;
			}
		}
		fclose(fh);
	}
	else
		retval = 1;
	if (retval == 1)
		perror(image);
	return retval;
}

int createFile(struct Config *cfg) {
struct stat st;
int retval = 1;

	if (stat(cfg->image, &st) == 0)
	{
//		printf("type=%d blocks=%ld blocksize=%ld\n", st.st_rdev, st.st_blocks, st.st_blksize);
		if (S_ISBLK(st.st_mode))
		{
			printf("block device\n");
		}
		else if (S_ISREG(st.st_mode))
		{
			if (st.st_size == 0)
			{
				if (fillFile(cfg->image, cfg->size) == 0)
					retval = 0;
			}
			else if (st.st_size/512 < cfg->size)
			{
				printf("%s: File already exists and is too small\n", cfg->image);
			}
			else
				retval = 0;
		}
		else
		{
			printf("%s: This is not a regular file or blockdevice!\n", cfg->image);
		}
	}
	else
	{
		if (errno == ENOENT)
		{
			if (fillFile(cfg->image, cfg->size) == 0)
				retval = 0;
		}
		else
		{
			perror(cfg->image);
		}
	}
	return retval;
}

int doWork(struct Config *cfg) {
int retval = 1;
struct PathElement *pe;
struct afsbase *afsbase=NULL;
struct DosEnvec de={0};
LONG error;
struct Volume *volume;

	printf("Image: %s\n", cfg->image);
	printf("Size: %d 512 byte sectors\n", cfg->size);
	printf("Name: %s\n", cfg->name);
	printf("Type: ");
	switch (cfg->type)
	{
	case 0:
		printf("Old Filesystem\n");
		cfg->type = ID_DOS_DISK;
		break;
	case 1:
		printf("Fast Filesystem\n");
		cfg->type = ID_FFS_DISK;
		break;
	case 2:
		printf("International Old Filesystem\n");
		cfg->type = ID_INTER_DOS_DISK;
		break;
	case 3:
		printf("International Fast Filesystem\n");
		cfg->type = ID_INTER_FFS_DISK;
		break;
	}
	printf("Path:\n");
	pe = cfg->first;
	while (pe != NULL)
	{
		printf("\t%s\n", pe->path);
		pe = pe->next;
	}
	de.de_SizeBlock = 512>>2;
	de.de_TableSize = 20;
	de.de_BootBlocks = cfg->bootblocks;
	de.de_Reserved = cfg->reserved;
	de.de_NumBuffers = 20;
	de.de_Surfaces=1;
	de.de_BlocksPerTrack=1;
	de.de_LowCyl = 0;
	de.de_HighCyl = cfg->size-1;
	if (createFile(cfg) == 0)
	{
		volume = initVolume(afsbase, NULL, cfg->image, 0, &de, &error);
		if (volume != NULL)
		{
			if ((error == 0) || (error == ERROR_NOT_A_DOS_DISK))
			{
				if (error == ERROR_NOT_A_DOS_DISK)
				{
					printf("Initialising disk ...");
					format(afsbase, volume, cfg->name, cfg->type);
					newMedium(NULL, volume);
					printf("done\n");
				}
				retval = copyData(cfg, volume);
			}
			else
				printf("Error %ld!\n", error);
			uninitVolume(afsbase, volume);
		}
		else
			printf("Error %ld!\n", error);
	}
	return retval;
}

void printUsage(char *prg) {
	printf("Usage: %s [options] <imagefile> <path1> [path2 ...] \n", prg);
	printf("\t--size\timage size\n"
			"\t\tThis is either of type int (a multiple of 512) or the special\n"
			"\t\tvalue 'floppy1440'.\n");
	printf("\t--reserved\tnumber of reserved blocks (default: 2)\n");
	printf("\t--bootblock\tnumber of bootblocks (default: 2)\n");
	printf("\t--name\tlabel of the FS image\n");
	printf("\t--type\tFS type (OFS, IOFS, FFS, IFFS(default))\n");
	printf("\t--help\tthis help message\n");
}

void addPathElement(struct Config *cfg, struct PathElement *pe) {
struct PathElement *next;

	next = (struct PathElement *)&cfg->first;
	while (next->next != NULL)
		next = next->next;
	next->next = pe;
	pe->next = NULL;
}

int parseCommandLine(int argc, char *argv[], struct Config *cfg) {
int i;
int have_image=0;
struct PathElement *pe;

	cfg->first = NULL;
	cfg->name = NULL;
	cfg->type = 3;
	cfg->reserved = 2;
	cfg->bootblocks = 2;
	for (i=1;i<argc;i++)
	{
		if ((argv[i][0] == '-') && (argv[i][0] == argv[i][1]))
		{
			if (strcasecmp(argv[i]+2, "help") == 0)
			{
				printUsage(argv[0]);
				return 1;
			}
			else if (strcasecmp(argv[i]+2, "size") == 0)
			{
				i++;
				if (i<argc)
				{
					if (strcasecmp(argv[i], "floppy1440") == 0)
						cfg->size = 2880;
					else
					{
						char *end;
						cfg->size = strtoul(argv[i], &end, 10);
						if (end[0] != 0)
						{
							printf("%s: Integer error\n", argv[i-1]);
							return 2;
						}
						if (cfg->size < 8)
						{
							printf("%s: Value must be at least 8\n", argv[i-1]);
							return 2;
						}
					}
				}
				else
				{
					printf("%s: Missing argument to option\n", argv[i-1]);
					return 2;
				}
			}
			else if (strcasecmp(argv[i]+2, "reserved") == 0)
			{
				i++;
				if (i<argc)
				{
					char *end;
					cfg->reserved = strtoul(argv[i], &end, 10);
					if (end[0] != 0)
					{
						printf("%s: Integer error\n", argv[i-1]);
						return 2;
					}
				}
				else
				{
					printf("%s: Missing argument to option\n", argv[i-1]);
					return 2;
				}
			}
			else if (strcasecmp(argv[i]+2, "bootblocks") == 0)
			{
				i++;
				if (i<argc)
				{
					char *end;
					cfg->bootblocks = strtoul(argv[i], &end, 10);
					if (end[0] != 0)
					{
						printf("%s: Integer error\n", argv[i-1]);
						return 2;
					}
				}
				else
				{
					printf("%s: Missing argument to option\n", argv[i-1]);
					return 2;
				}
			}
			else if (strcasecmp(argv[i]+2, "name") == 0)
			{
				i++;
				if (i<argc)
					cfg->name = argv[i];
				else
				{
					printf("%s: Missing argument to option\n", argv[i-1]);
					return 2;
				}
			}
			else if (strcasecmp(argv[i]+2, "type") == 0)
			{
				i++;
				if (i<argc)
				{
					if (strcasecmp(argv[i], "OFS") == 0)
						cfg->type = 0;
					else if (strcasecmp(argv[i], "IOFS") == 0)
						cfg->type = 2;
					else if (strcasecmp(argv[i], "FFS") == 0)
						cfg->type = 1;
					else if (strcasecmp(argv[i], "IFFS") == 0)
						cfg->type = 3;
					else
					{
						printf("%s: Unknown fs type\n", argv[i-1]);
						return 5;
					}
				}
				else
				{
					printf("%s: Missing argument to option\n", argv[i-1]);
					return 2;
				}
			}
			else
			{
				printf("%s: Unknown option\n", argv[i]);
				return 4;
			}
		}
		else
		{
			if (have_image==0)
			{
				cfg->image = argv[i];
				have_image = 1;
			}
			else
			{
				pe = malloc(sizeof(struct PathElement));
				if (pe == NULL)
				{
					printf("Not enough memory\n");
					return 3;
				}
				pe->path = argv[i];
				addPathElement(cfg, pe);
			}
		}
	}
	if (cfg->name == NULL)
		cfg->name = "SomeDisk";
	return 0;
}
int main(int argc, char *argv[]) {
int error;
struct Config cfg;

	if (argc<3)
	{
		printUsage(argv[0]);
		return 1;
	}
	else
	{
		error = parseCommandLine(argc, argv, &cfg);
		if (error == 0)
		{
			error = doWork(&cfg);
		}
		else if (error > 1)
		{
			error = 1;
		}
	}
	return error;
}
