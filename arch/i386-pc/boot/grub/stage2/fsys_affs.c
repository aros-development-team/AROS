#ifdef FSYS_AFFS
#include "shared.h"
#include "filesys.h"

#define T_SHORT		2
#define T_LIST			16

#define ST_FILE		-3
#define ST_ROOT		1
#define ST_USERDIR	2	

struct RootBlock{
	int p_type;					//0
	int n1[2];					//1-2
	int hashtable_size;		//3
	int n2;						//4
	int checksum;				//5
	int hashtable[72];		//6-77
	int bitmap_valid_flag;	//78
	int bitmap_ptrs[25];		//79-103
	int bitmap_extension;	//104
	int root_days;				//105
	int root_mins;				//106
	int root_ticks;			//107;
	char diskname[32];		//108-115
	int n3[2];					//116-117
	int volume_days;			//118
	int volume_mins;			//119
	int volume_ticks;			//120
	int creation_days;		//121
	int creation_mins;		//122
	int creation_ticks;		//123
	int n4[3];					//124-126
	int s_type;					//127
};

struct DirHeader {
	int p_type;					//0
	int own_key;				//1
	int n1[3];					//2-4
	int checksum;				//5
	int hashtable[72];		//6-77
	int n2;						//78
	int owner;					//79
	int protection;			//80
	int n3;						//81
	char comment[92];			//82-104
	int days;					//105
	int mins;					//106
	int ticks;					//107
	char name[32];				//108-115
	int n4[2];					//116-117
	int linkchain;				//118
	int n5[5];					//119-123
	int hashchain;				//124
	int parent;					//125
	int n6;						//126
	int s_type;					//127
};

struct FileHeader {
	int p_type;					//0
	int own_key;				//1
	int n1[3];					//2-4
	int checksum;				//5
	int filekey_table[72];	//6-77
	int n2;						//78
	int owner;					//79
	int protection;			//80
	int bytesize;				//81
	char comment[92];			//82-104
	int days;					//105
	int mins;					//106
	int ticks;					//107
	char name[32];				//108-115
	int n3[2];					//116-117
	int linkchain;				//118
	int n4[5];					//119-123
	int hashchain;				//124
	int parent;					//125
	int extension;				//126
	int s_type;					//127
};

struct FileKeyExtension{
	int p_type;					//0
	int own_key;				//1
	int table_size;			//2
	int n1[2];					//3-4
	int checksum;				//5
	int filekey_table[72];	//6-77
	int info[46];				//78-123
	int n2;						//124
	int parent;					//125
	int extension;				//126
	int s_type;					//127
};

struct Position {
	unsigned int block;
	short filekey;
	unsigned short byte;
	unsigned int offset;
};

struct ReadData {
	unsigned int header_block;
	struct Position current;
	unsigned int filesize;
};

#warning "Big vs. little endian for configure needed"
#define AROS_BE2LONG(l)	\
	(                                  \
	    ((((unsigned long)(l)) >> 24) & 0x000000FFUL) | \
	    ((((unsigned long)(l)) >>  8) & 0x0000FF00UL) | \
	    ((((unsigned long)(l)) <<  8) & 0x00FF0000UL) | \
	    ((((unsigned long)(l)) << 24) & 0xFF000000UL)   \
	)

struct FSysBuffer {
	struct RootBlock rootblock;
	struct ReadData file;
	struct DirHeader buffer1;
	struct FileHeader buffer2;
	struct FileKeyExtension extension;
};

struct FSysBuffer *fsysb;

unsigned int calcChkSum(unsigned short SizeBlock, unsigned int *buffer) {
unsigned int sum=0,count=0;

	for (count=0;count<SizeBlock;count++)
		sum += AROS_BE2LONG(buffer[count]);
	return sum;
}

int affs_mount(void) {

	if ((current_drive>0x80) && (current_slice != 0x30))
		return 0;
	fsysb = (struct FSysBuffer *)FSYS_BUF;
	devread(0, 0, 512, (char *)&fsysb->buffer1);
	if (!(
			((AROS_BE2LONG(fsysb->buffer1.p_type) & 0xFFFFFF00)==0x444F5300) &&
			((AROS_BE2LONG(fsysb->buffer1.p_type) & 0xFF)>0)
		))
	{
		devread(1, 0, 512, (char *)&fsysb->buffer1);
		if (!(
				((AROS_BE2LONG(fsysb->buffer1.p_type) & 0xFFFFFF00)==0x444F5300) &&
				((AROS_BE2LONG(fsysb->buffer1.p_type) & 0xFF)>0)
			))
		{
			return 0;
		}
	}
	devread(part_length/2, 0, 512, (char *)&fsysb->rootblock);
	if (
			(AROS_BE2LONG(fsysb->rootblock.p_type) != T_SHORT) ||
			(AROS_BE2LONG(fsysb->rootblock.s_type) != ST_ROOT) ||
			calcChkSum(128, (int *)&fsysb->rootblock)
		)
		return 0;
	return 1;
}

int seek(unsigned long offset) {
unsigned long block;
unsigned long togo;

	block = fsysb->file.header_block;

	togo = offset / 512;
	fsysb->file.current.filekey = 71-(togo % 72);
	togo /= 72;
	fsysb->file.current.byte = offset % 512;
	fsysb->file.current.offset = offset;
	while ((togo) && (block))
	{
		disk_read_func = disk_read_hook;
		devread(block, 0, 512, (char *)&fsysb->extension);
		disk_read_func = 0;
		block = AROS_BE2LONG(fsysb->extension.extension);
		togo--;
	}
	if (togo)
		return 1;
	fsysb->file.current.block = block;
	return 0;
}

int affs_read(char *buf, int len) {
unsigned short size;
unsigned int readbytes = 0;

	if (fsysb->file.current.offset != filepos)
	{
		if (seek(filepos))
			return ERR_FILELENGTH;
	}
	if (fsysb->file.current.block == 0)
		return 0;
	if (len>(fsysb->file.filesize-fsysb->file.current.offset))
		len=fsysb->file.filesize-fsysb->file.current.offset;
	disk_read_func = disk_read_hook;
	devread(fsysb->file.current.block, 0, 512, (char *)&fsysb->extension);
	disk_read_func = 0;
	while (len)
	{
		disk_read_func = disk_read_hook;
		if (fsysb->file.current.filekey<0)
		{
			fsysb->file.current.filekey = 71;
			fsysb->file.current.block = AROS_BE2LONG(fsysb->extension.extension);
			if (fsysb->file.current.block)
			{
				devread(fsysb->file.current.block, 0, 512, (char *)&fsysb->extension);
			}
#warning "else shouldn't occour"
		}
		size = 512;
		size -= fsysb->file.current.byte;
		if (size>len)
		{
			size = len;
			devread
				(
					AROS_BE2LONG
						(
							fsysb->extension.filekey_table
								[fsysb->file.current.filekey]
						),
					fsysb->file.current.byte, size, (char *)((int)buf+readbytes)
				);
			fsysb->file.current.byte += size;
		}
		else
		{
			devread
				(
					AROS_BE2LONG
						(
							fsysb->extension.filekey_table
								[fsysb->file.current.filekey]
						),
					fsysb->file.current.byte, size, (char *)((int)buf+readbytes)
				);
			fsysb->file.current.byte = 0;
			fsysb->file.current.filekey--;
		}
		disk_read_func = 0;
		len -= size;
		readbytes += size;
	}
	fsysb->file.current.offset += readbytes;
	filepos = fsysb->file.current.offset;
	return readbytes;
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

grub_error_t getHeaderBlock(char *name, struct DirHeader *dirh) {
int key;

	key = getHashKey(name, 72, 1);
	if (!dirh->hashtable[key])
		return ERR_FILE_NOT_FOUND;
	devread(AROS_BE2LONG(dirh->hashtable[key]), 0, 512, (char *)dirh);
	if (calcChkSum(128, (unsigned int *)dirh))
		return ERR_FSYS_CORRUPT;
	if (AROS_BE2LONG(dirh->p_type) != T_SHORT)
		return ERR_BAD_FILETYPE;
	while (noCaseStrCmp(name,dirh->name,1) != 0)
	{
		if (!dirh->hashchain)
			return ERR_FILE_NOT_FOUND;
		devread(AROS_BE2LONG(dirh->hashchain), 0, 512, (char *)dirh);
		if (calcChkSum(128, (unsigned int *)dirh))
			return ERR_FSYS_CORRUPT;
		if (AROS_BE2LONG(dirh->p_type) != T_SHORT)
			return ERR_BAD_FILETYPE;
	}
	return 0;
}

grub_error_t findBlock(char *name, struct DirHeader *dirh) {
char dname[32];
char *nbuf;

	devread(part_length/2, 0, 512, (char *)dirh);
	name++;
	while (*name)
	{
		if (
				(AROS_BE2LONG(dirh->s_type) != ST_ROOT) &&
				(AROS_BE2LONG(dirh->s_type) != ST_USERDIR)
			)
			return ERR_BAD_FILETYPE;
		nbuf = dname;
		while ((*name != '/') && (*name))
			*nbuf++ = *name++;
		if (*name == '/')
			name++;
		*nbuf = 0;
		errnum = getHeaderBlock(dname, dirh);
		if (errnum)
			return errnum;
	}
	return 0;
}

int affs_dir(char *dirname) {
char *current = dirname;
char filename[128];
char cstr[32];
char *fname = filename;
int i,block;

	if (print_possibilities)
	{
		while (*current)
			current++;
		while (*current != '/')
			current--;
		current++;
		while (*current)
		{
			*fname++ = *current;
			*current++ = 0;
		}
		*fname=0;
		errnum = findBlock(dirname, &fsysb->buffer1);
		if (errnum)
			return 0;
		for (i=0;i<72;i++)
		{
			block = fsysb->buffer1.hashtable[i];
			while (block)
			{
				devread
					(
						AROS_BE2LONG(block),
						0,512,(char *)&fsysb->buffer2
					);
				if (calcChkSum(128, (unsigned int *)&fsysb->buffer2))
				{
					errnum = ERR_FSYS_CORRUPT;
					return 0;
				}
				if (AROS_BE2LONG(fsysb->buffer2.p_type) != T_SHORT)
				{
					errnum = ERR_BAD_FILETYPE;
					return 0;
				}
				if (noCaseStrCmp(filename, fsysb->buffer2.name, 1)<=0)
				{
					if (print_possibilities>0)
						print_possibilities = -print_possibilities;
					memcpy(cstr,fsysb->buffer2.name+1,fsysb->buffer2.name[0]);
					cstr[fsysb->buffer2.name[0]]=0;
					print_a_completion(cstr);
				}
				block = fsysb->buffer2.hashchain;
			}
		}
		while (*current != '/')
			current--;
		current++;
		fname = filename;
		while (*fname)
			*current++ = *fname++;
#warning "TODO: add some more chars until posibilities differ"
		if (print_possibilities>0)
			errnum = ERR_FILE_NOT_FOUND;
		return (print_possibilities<0);
	}
	else
	{
		errnum = findBlock(dirname, (struct DirHeader *)&fsysb->buffer2);
		if (errnum)
			return 0;
		if (AROS_BE2LONG(fsysb->buffer2.s_type)!=ST_FILE)
		{
			errnum = ERR_BAD_FILETYPE;
			return 0;
		}
		fsysb->file.header_block = AROS_BE2LONG(fsysb->buffer2.own_key);
		fsysb->file.current.block = AROS_BE2LONG(fsysb->buffer2.own_key);
		fsysb->file.current.filekey = 71;
		fsysb->file.current.byte = 0;
		fsysb->file.current.offset = 0;
		fsysb->file.filesize = AROS_BE2LONG(fsysb->buffer2.bytesize);
		filepos = 0;
		filemax = fsysb->file.filesize;
		return 1;
	}
}
#endif

