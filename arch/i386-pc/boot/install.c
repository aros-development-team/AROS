/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <errno.h>
#include <strings.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/stat.h>

#warning "Only for little endian machines!!!"
#   define AROS_BE2LONG(l)     \
	(                                  \
	    ((((unsigned long)(l)) >> 24) & 0x000000FFUL) | \
	    ((((unsigned long)(l)) >>  8) & 0x0000FF00UL) | \
	    ((((unsigned long)(l)) <<  8) & 0x00FF0000UL) | \
	    ((((unsigned long)(l)) << 24) & 0xFF000000UL)   \
	)

#define CMD_READ	1
#define CMD_WRITE	2

#define ERR_BAD_FILETYPE	1
#define ERR_FILE_NOT_FOUND	2
#define ERR_FSYS_CORRUPT	3

#define T_SHORT	2

#define ST_ROOT		1
#define ST_USERDIR	2

struct Volume {
	int fd;
	unsigned short readcommand;
	unsigned short writecommand;
	unsigned int startblock;
	unsigned int countblock;
	unsigned short SizeBlock;
	unsigned char flags;
	unsigned int *blockbuffer;
};

#define VF_IS_TRACKDISK	(1<<0)
#define VF_MOVE_BB		(1<<1)

struct BlockNode {
	unsigned int sector;
	unsigned short count;
	unsigned short seg_adr;
};

unsigned int stage2_firstblock[128];

int readwriteBlock
	(
		struct Volume *volume,
		unsigned int block,
		void *buffer,
		size_t length,
		unsigned short command
	)
{
off_t offset;
int retval;

	offset = (volume->startblock+block)*(volume->SizeBlock*4);
	if (lseek(volume->fd, offset,SEEK_SET)!=-1)
	{
		if (command == CMD_READ)
		{
			if (read(volume->fd, buffer, length)!=-1)
				return 0;
		}
		else if (command == CMD_WRITE)
		{
			if (write(volume->fd, buffer, length)!=-1)
				return 0;
		}
		else
			errno = 0;
	}
	return errno;
}

unsigned int collectBlockList
	(
		struct Volume *volume,
		unsigned int block,
		struct BlockNode *blocklist
	)
{
unsigned int retval, first_block;
short blk_count,count;
unsigned short i;

#warning "TODO: logical/physical blocks"
	/*
		initialze stage2-blocklist
		(it is NULL-terminated)
	*/
	for (blk_count=-1;blocklist[blk_count].sector!=0;blk_count--)
		blocklist[blk_count].sector = 0;
	/*
		the first block of stage2 will be stored in stage1
		so skip the first filekey in the first loop
	*/
#warning "Block read twice"
	retval=readwriteBlock
		(
			volume, block, volume->blockbuffer, volume->SizeBlock*4,
			volume->readcommand
		);
	i = volume->SizeBlock - 52;
	first_block = AROS_BE2LONG(volume->blockbuffer[volume->SizeBlock-51]);
	blk_count=0;
	do {
		retval=readwriteBlock
			(
				volume, block, volume->blockbuffer, volume->SizeBlock*4,
				volume->readcommand
			);
		if (retval)
		{
			printf("ReadError %ld\n", retval);
			return 0;
		}
		while ((i>=6) && (volume->blockbuffer[i]))
		{
			/*
				if current sector follows right after last sector
				then we don't need a new element
			*/
			if (
					(blocklist[blk_count].sector) &&
					((blocklist[blk_count].sector+blocklist[blk_count].count)==
						AROS_BE2LONG(volume->blockbuffer[i]))
				)
			{
				blocklist[blk_count].count += 1;
			}
			else
			{
				blk_count--; /* decrement first */
				if (blocklist[blk_count-1].sector != 0)
				{
					printf("There is no more space to save blocklist in stage2\n");
					return 0;
				}
				blocklist[blk_count].sector = AROS_BE2LONG(volume->blockbuffer[i]);
				blocklist[blk_count].count = 1;
			}
			i--;
		}
		i = volume->SizeBlock - 51;
		block = AROS_BE2LONG(volume->blockbuffer[volume->SizeBlock - 2]);
	} while (block);
	/*
		blocks in blocklist are relative to the first
		sector of the HD (not partition)
	*/
	i = 0;
	for (count=-1;count>=blk_count;count--)
	{
		blocklist[count].sector += volume->startblock;
		blocklist[count].seg_adr = 0x820 + (i*32);
		i += blocklist[count].count;
	}
	return first_block;
}

/**************************************************************************/

unsigned int calcChkSum(unsigned short SizeBlock, unsigned int *buffer) {
unsigned int sum=0,count=0;

	for (count=0;count<SizeBlock;count++)
		sum += AROS_BE2LONG(buffer[count]);
	return sum;
}

unsigned char capitalch(unsigned char ch, unsigned char flags) {

	if ((flags==0) || (flags==1))
		return (unsigned char)((ch>='a') && (ch<='z') ? ch-('a'-'A') : ch);
	else		// DOS\(>=2)
		return (unsigned char)(((ch>=224) && (ch<=254) && (ch!=247)) ||
				 ((ch>='a') && (ch<='z')) ? ch-('a'-'A') : ch);
}

// str2 is a BCPL string
int noCaseStrCmp(char *str1, char *str2, unsigned char flags) {
unsigned char length;

	length=str2++[0];
	do {
		if ((*str1==0) && (length==0))
			return 0;
		length--;
//		if ((*str1==0) && (*str2==0)) return 1;
	} while (capitalch(*str1++,flags)==capitalch(*str2++,flags));
	str1--;
	return (*str1) ? 1 : -1;
}

unsigned int getHashKey(char *name,unsigned int tablesize, unsigned char flags) {
unsigned int result;

	result=strlen(name);
	while (*name!=0)
		result=(result * 13 +capitalch(*name++,flags)) & 0x7FF;
	return result%tablesize;
}

int getHeaderBlock(struct Volume *volume, char *name, unsigned int *dirh) {
int key;

	key = getHashKey(name, (volume->SizeBlock-51)-6+1, 1);
	if (!dirh[6+key])
		return ERR_FILE_NOT_FOUND;
	readwriteBlock
		(
			volume,
			AROS_BE2LONG(dirh[6+key]),
			dirh,
			volume->SizeBlock*4,
			volume->readcommand
		);
	if (calcChkSum(volume->SizeBlock, dirh))
		return ERR_FSYS_CORRUPT;
	if (AROS_BE2LONG(dirh[0]) != T_SHORT)
		return ERR_BAD_FILETYPE;
	while (noCaseStrCmp(name,(char *)((unsigned int)dirh+((volume->SizeBlock-20)*4)),1) != 0)
	{
		if (!dirh[volume->SizeBlock-4])
			return ERR_FILE_NOT_FOUND;
		readwriteBlock
			(
				volume,
				AROS_BE2LONG(dirh[volume->SizeBlock-4]),
				dirh,
				volume->SizeBlock*4,
				volume->readcommand
			);
		if (calcChkSum(volume->SizeBlock, (unsigned int *)dirh))
			return ERR_FSYS_CORRUPT;
		if (AROS_BE2LONG(dirh[0]) != T_SHORT)
			return ERR_BAD_FILETYPE;
	}
	return 0;
}

int findFile(struct Volume *volume, char *name, unsigned int *buffer) {
char dname[32];
char *nbuf;
int errnum;

	readwriteBlock
		(
			volume,
			volume->countblock/2,
			buffer,
			volume->SizeBlock*4,
			volume->readcommand
		);
	name++;
	while (*name)
	{
		if (
				(AROS_BE2LONG(buffer[volume->SizeBlock-1]) != ST_ROOT) &&
				(AROS_BE2LONG(buffer[volume->SizeBlock-1]) != ST_USERDIR)
			)
			return ERR_BAD_FILETYPE;
		nbuf = dname;
		while ((*name != '/') && (*name))
			*nbuf++ = *name++;
		if (*name == '/')
			name++;
		*nbuf = 0;
		errnum = getHeaderBlock(volume, dname, buffer);
		if (errnum)
			return errnum;
	}
	return 0;
}


void installStageFiles(struct Volume *volume) {
unsigned int block,retval;

	retval=findFile(volume, "/boot/grub/stage2", stage2_firstblock);
	if ( retval == 0 )
	{
		block = AROS_BE2LONG(stage2_firstblock[1]);
		/* read first data block */
		readwriteBlock
			(
				volume,
				AROS_BE2LONG(stage2_firstblock[volume->SizeBlock-51]),
				(void *)stage2_firstblock,
				volume->SizeBlock*4,
				volume->readcommand
			);
		/* save first BB (with flags) */
		if (volume->flags & VF_MOVE_BB)
		{
			readwriteBlock
				(
					volume, 1, volume->blockbuffer, 512,
					volume->writecommand
				);
		}
		if (
				(
					block=collectBlockList
						(
							volume, block,
							(struct BlockNode *)&stage2_firstblock[128]
						)
				)
			)
		{
			if (findFile(volume, "/boot/grub/stage1", volume->blockbuffer) == 0)
			{
				/* read first data block of stage1 */
				retval = readwriteBlock
					(
						volume,
						AROS_BE2LONG(volume->blockbuffer[volume->SizeBlock-51]),
						volume->blockbuffer,
						512,
						volume->readcommand
					);
				if (retval == 0)
				{
					/* write stage1 as BB */
					volume->blockbuffer[17]=block;
					retval = readwriteBlock
						(
							volume, 0,
							volume->blockbuffer, 512, volume->writecommand
						);
					if (retval)
						printf("WriteError %ld\n", retval);
					else
					{
						/* write first data block of stage2 */
						readwriteBlock
							(
								volume,
								block,
								stage2_firstblock,
								512,
								volume->writecommand
							);
					}
				}
				else
					printf("ReadError %d\n", retval);
			}
			else
				printf("stage1 file not found\n");
		}
	}
	else
		printf("stage2: %d\n",retval);
}

struct Volume *initVolume(char *filename) {
struct Volume *volume=0;
struct stat stat;
char *error;
unsigned int retval;

	if (lstat(filename, &stat)==0)
	{
		volume = (struct Volume *)calloc(1, sizeof(struct Volume));
		if (volume)
		{
			volume->SizeBlock = 128;
			volume->blockbuffer = (unsigned int *)calloc(1, volume->SizeBlock*4);
			if (volume->blockbuffer)
			{
				volume->fd = open(filename, O_RDWR);
				if (volume->fd)
				{
#warning "No support for partitions"
					volume->startblock = 0;
					volume->countblock = stat.st_size/(volume->SizeBlock*4);
					volume->readcommand = CMD_READ;
					volume->writecommand = CMD_WRITE;
					retval = readwriteBlock
						(
							volume, 0,
							volume->blockbuffer, 512, volume->readcommand
						);
					if (retval == 0)
					{
						if ((AROS_BE2LONG(volume->blockbuffer[0]) & 0xFFFFFF00)!=0x444F5300)
						{
							retval = readwriteBlock
								(
									volume, 1,
									volume->blockbuffer, 512, volume->readcommand
								);
						}
						else
							volume->flags |= VF_MOVE_BB;
						if (
								((AROS_BE2LONG(volume->blockbuffer[0]) & 0xFFFFFF00)==0x444F5300) &&
								((AROS_BE2LONG(volume->blockbuffer[0]) & 0xFF)>0)
							)
						{
							return volume;
						}
						else
							error = "No Amiga FFS disk";
					}
					else
						error = "ReadError";
				}
				else
					error = "Couldn't open file";
				free(volume->blockbuffer);
			}
			else
				error = "Not enough memory";
			free(volume);
			volume = 0;
		}
		else
			error = "Not enough memory";
	}
	else
		error = "lstat() error";
	printf("%s\n",error);
	return 0;
}

void uninitVolume(struct Volume *volume) {

	close(volume->fd);
	free(volume->blockbuffer);
	free(volume);
}

void checkBootCode(struct Volume *volume) {
	printf("CHECK not implemented yet\n");
}

void removeBootCode(struct Volume *volume) {
unsigned int retval;

	retval = readwriteBlock
		(
			volume, 1,
			volume->blockbuffer, 512, volume->readcommand
		);
	if (retval)
		printf("ReadError %ld\n", retval);
	else
	{
		if ((AROS_BE2LONG(volume->blockbuffer[0]) & 0xFFFFFF00)==0x444F5300)
		{
			retval = readwriteBlock
				(
					volume, 0,
					volume->blockbuffer, 512, volume->writecommand
				);
			if (retval)
				printf("WriteError %ld\n", retval);
		}
	}
}

int main(int argc, char **argv) {
struct Volume *volume;

	if (
			(argc == 1) ||
			((argc == 2) && (strcmp(argv[1],"--help")==0)) ||
			(argc > 3)
		)
		printf("Usage: %s filename --noboot --check\n",argv[0]);
	else
	{
		volume = initVolume(argv[1]);
		if (volume)
		{
			if (argc == 3)
			{
				if (strcmp(argv[2],"--noboot")==0)
					removeBootCode(volume);
				else if (strcmp(argv[2],"--check")==0)
					checkBootCode(volume);
			}
			else
				installStageFiles(volume);
			uninitVolume(volume);
		}
	}
	return 0;
}

