/* $Id$
 * $Log: doctor.h $
 * Revision 2.8  1999/09/11  16:45:50  Michiel
 * Versie 1.5 with Unformat and Repair nodos
 *
 * Revision 2.7  1999/09/10  22:15:35  Michiel
 * Bugfixes etc (1.4)
 *
 * Revision 2.6  1999/05/28  05:07:33  Michiel
 * Fixed bug occuring on empty directory blocks
 * Added rbl.always fix; improved rbl.disksize fix
 * Reduced cachesize
 *
 * Revision 2.5  1999/05/17  10:32:39  Michiel
 * long filename support, verbose fixes
 *
 * Revision 2.4  1999/05/07  16:49:00  Michiel
 * bugfixes etc
 *
 * Revision 2.3  1999/05/04  17:59:09  Michiel
 * check mode, logfile, search rootblock implemented
 * bugfixes
 *
 * Revision 2.2  1999/05/04  04:27:13  Michiel
 * debugged upto buildrext
 *
 * Revision 2.1  1999/04/30  12:17:58  Michiel
 * Accepts OK disks, bitmapfix and hardlink fix works
 *
 * Revision 1.1  1999/04/19  22:16:53  Michiel
 * Initial revision
 *
 *
 * Doctor headerfile
 * date: 1999/03/01
 * author: Michiel 
 */ 

#define __USE_SYSBASE



#ifndef min
#include <clib/macros.h>
#define min(a,b) MIN(a,b)
#define max(a,b) MAX(a,b)
#endif

typedef unsigned long uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;
typedef long int32;
typedef short int16;
typedef char int8;
typedef enum {false, true} bool;

// last two bytes used for extended file size
#define DELENTRYFNSIZE 16

#define BOOTBLOCK 0
#define ROOTBLOCK 2
#define FNSIZE 108
#define PATHSIZE 256
#define FILENAMESIZE 32
#define DNSIZE 32
#define CMSIZE 80
#define MAX_ENTRYSIZE (sizeof(struct direntry) + FILENAMESIZE + CMSIZE + 32)
#define LONGS_PER_BMB ((rbl->reserved_blksize/4)-3)
#define INDEX_PER_BLOCK ((rbl->reserved_blksize - 3*4) / sizeof(LONG))
#define ANODES_PER_BLOCK ((rbl->reserved_blksize - 4*4) / sizeof(anode_t))
#define FIRSTENTRY(blok) ((struct direntry*)((blok)->data->entries))

/**************************************
 * TD64 support
 **************************************/

#ifndef TD_READ64
#define TD_READ64	24
#define TD_WRITE64	25
#define TD_SEEK64	26
#define TD_FORMAT64	27
#endif

/**************************************
 * NSD support
 **************************************/

#ifndef NSCMD_DEVICEQUERY
#define NSCMD_DEVICEQUERY 0x4000
#define NSCMD_TD_READ64 0xc000
#define NSCMD_TD_WRITE64 0xc001
#define NSDEVTYPE_TRACKDISK 5
struct NSDeviceQueryResult
{
    ULONG   DevQueryFormat;
    ULONG   SizeAvailable;
    UWORD   DeviceType;
    UWORD   DeviceSubType;
    UWORD   *SupportedCommands;
};
#endif

#define ACCESS_UNDETECTED 0
#define ACCESS_STD 1
#define ACCESS_DS 2
#define ACCESS_TD64 3
#define ACCESS_NSD 4

/**************************************
 * Cache
 **************************************/

/* blocknr has to be initialised on a never used blocknr
 * to prevent it to be considered a loaded cacheline.
 */
#define CL_UNUSED 1
struct cacheline
{
	struct cacheline *next;
	struct cacheline *prev;
	uint32 blocknr;			/* 1 == unused */
	bool dirty;
	uint8 *data;
};

struct cache
{
	struct MinList LRUqueue;
	struct MinList LRUpool;
	uint32 linesize;			// linesize in blocks
	uint32 nolines;				
	struct cacheline *cachelines;
};

extern struct cache cache;

/**************************************
 * Blocks
 **************************************/

enum mode {check=0, build, repair, search, unformat, done};
extern enum mode mode;

/* mode for cached block can be:
 * 'check' -- loaded in cache voor check etc --> indirect (by value) edit
 * 'build' -- loaded in buildblock cache. --> direct edit (by reference)
 */
typedef struct {
	uint32 blocknr;
	enum mode mode;
	reservedblock_t *data;
} cachedblock_t;

typedef struct {
	uint32 blocknr;
	enum mode mode;
	dirblock_t *data;
} c_dirblock_t;

typedef struct {
	uint32 blocknr;
	enum mode mode;
	indexblock_t *data;
} c_indexblock_t;

typedef struct {
	uint32 blocknr;
	enum mode mode;
	extensionblock_t *data;
} c_extensionblock_t;

typedef struct {
	uint32 blocknr;
	enum mode mode;
	deldirblock_t *data;
} c_deldirblock_t;

typedef struct {
	uint32 blocknr;
	enum mode mode;
	anodeblock_t *data;
} c_anodeblock_t;

typedef struct {
	uint32 blocknr;
	enum mode mode;
	bitmapblock_t *data;
} c_bitmapblock_t;

/* note: data in b allocated by build code */
typedef struct buildblock 
{
	struct buildblock *next;
	struct buildblock *prev;
	cachedblock_t b;
} buildblock_t;

typedef struct canode
{
	ULONG clustersize;	// number of blocks in cluster
	ULONG blocknr;		// the block number
	ULONG next;			// next anode (anode number), 0 = eof
	ULONG nr;			// the anode number
} canode_t;

extern rootblock_t *rbl;
extern c_extensionblock_t rext;

/**************************************
 * StandardScan
 **************************************/

/* abort flag, filled by trap */
extern bool aborting;

/* maxima
 */
#define MAX_PASS			7

/* options for StandardScan() (flags)
 */
#define SSF_CHECK	  		1		/* check consistency? */
#define SSF_FIX       		2		/* fix errors? */
#define SSF_ANALYSE   		4		/* analyse volume, count things etc ? */
#define SSF_UNFORMAT		8		/* undo fast format */

/* option flags
 */
#define SSF_GEN_RESBITMAP	16
#define SSF_GEN_ANODEBITMAP	32
#define SSF_GEN_MAINBITMAP	64
#define SSF_GEN_BMMASK	(SSF_GEN_RESBITMAP|SSF_GEN_ANODEBITMAP|SSF_GEN_MAINBITMAP)
#define SSF_VERBOSE			128

/* ready flags
 */

/* errors
 */
typedef enum {
	e_none = 0,		/* no error */
	e_aborted,
	e_dirty,
	e_remove,
	e_repartition,
	e_empty,
	e_not_found,
	e_out_of_memory,
	e_fatal_error,
	e_rbl_not_found,
	e_max_pass_exceeded,
	e_res_bitmap_fail,
	e_main_bitmap_fail,
	e_anode_bitmap_fail,
	e_block_outside_partition,
	e_block_outside_reserved,
	e_options_error,
	e_direntry_error,
	e_invalid_softlink,
	e_anode_error,
	e_reserved_area_error,
	e_outside_bitmap_error,
	e_double_allocation,
	e_number_error,
	e_syntax_error,
	e_read_error,
	e_write_error,
	e_alloc_fail
} error_t;

/**************************************
 * Bitmap
 **************************************/

#define BM_ENABLED	1
#define BM_REBUILD	2
#define BM_COMPARE	4
#define BM_FINISHED	8

#define InvalidBitmap(bitmap)	(bitmap->valid = false)
#define IsBitmapValid(bitmap)	(bitmap && bitmap->valid)

typedef struct 
{
	bool	valid;		/* fix only possible if valid */
	uint32	errorsfound;
	uint32	errorsfixed;
	uint32	start;
	uint32	stop;
	uint32	step;
	uint32	lwsize;		/* size in longwords */

	uint32	*map;
} bitmap_t;

/**************************************
 * Volume
 **************************************/

typedef struct {
	/* initialise size */
	uint32 firstblock;		/* abs blocknr, first and last block */
	uint32 lastblock;
	uint32 disksize;			/* disksize in blocks */
	uint32 lastreserved;		/* rel blocknr, last reserved block */
	uint32 blocksize;		/* physical blocksize in bytes */
	int16 blockshift;
	uint32 rescluster;

	/* info
	 */
	char diskname[34];
	int fnsize;

	/* Access */
	error_t (*getblock)(cachedblock_t *blok, uint32 bloknr);
	error_t (*writeblock)(cachedblock_t *blok);

	/* status 
	 * keep message if message==NULL
	 * status 0 = diskname
	 */
	void (*status)(int level, char *message, long maxval);
	void (*progress)(int level, long progress);
	void (*updatestats)(void);
	void (*showmsg)(const char *format, ...);
	int (*askuser)(char *message, char *okstr, char *cancelstr);

	/* flags */
	bool repartitioned;
	int accessmode;
	bool td64mode, nsdmode;
	int standardscan;		/* 0=not done/needed, 1=fixed, -1=not fixable */

	/* bitmaps */
	bitmap_t *mainbitmap;
	bitmap_t *anodebitmap;
	bitmap_t *resbitmap;

	/* full scan */
	struct MinList buildblocks;		/* elements are of type buildblock */

	/* private */
	struct FileSysStartupMsg *fssm;
	struct DosEnvec *dosenvec;
	char devicename[FNSIZE];
	char execdevice[FNSIZE];
	int execunit;

	/* device stuff */
	struct MsgPort *port;
	struct IOExtTD *request;
	error_t (*getrawblocks)(uint8 *data, int32 numblock, uint32 bloknr);
	error_t (*writerawblocks)(uint8 *data, int32 numblock, uint32 bloknr);
} volume_t;

extern volume_t volume;

extern struct stats 
{
	uint32 blocknr;
	uint32 prevblknr;
	int pass;
	int blockschecked;
	int numerrors;
	int errorsfixed;
	int	numfiles;
	int numdirs;
	int	numsoftlink;
	int numhardlink;
	int	numrollover;
	int fragmentedfiles;
	int anodesused;
} stats;

/**************************************
 * Fullscan
 **************************************/

typedef struct scanelement
{
	uint32	blocknr;
	uint32	datestamp;
} scanelement_t;

void InitFullScan(void);
void ExitFullScan(void);
error_t AllocBuildBlocks(void);
uint32 fs_AllocResBlock(void);
error_t Repartition(uint32 bloknr);
error_t BuildBootBlock(void);
error_t BuildRootBlock(rootblock_t *rbl);
error_t BuildRext(c_extensionblock_t *rext);
error_t BuildIndexBlock(c_indexblock_t *blk, uint16 bloktype, uint32 seqnr);
error_t BuildBitmapBlock(c_bitmapblock_t *blk, uint32 seqnr);
void SearchBlocks(scanelement_t el[], uint32 seqlow, uint32 seqhigh, uint32 start, uint32 stop, uint16 bloktype);
uint32 SearchBlock(uint16 bloktype, uint32 seqnr, uint32 last, uint32 datestamp, uint32 anodenr, uint32 parent);
uint32 SearchLastReserved(volume_t *vol);
uint32 SearchFileSystem(int32 startblok, int32 endblok);

/**********************************************************************/
/*                        Lists                                       */
/**********************************************************************/
#define MinAddHead(list, node)  AddHead((struct List *)(list), (struct Node *)(node))
#define MinAddTail(list, node)  AddTail((struct List *)(list), (struct Node *)(node))
#define MinInsert(list, node, listnode) Insert((struct List *)list, (struct Node *)node, (struct Node *)listnode)
#define MinRemove(node) Remove((struct Node *)node)
#define HeadOf(list) ((void *)((list)->mlh_Head))
#define IsHead(node) (!((node)->prev->prev))
#define IsTail(node) (!((node)->next->next))
#ifndef IsMinListEmpty
#define IsMinListEmpty(x) ( ((x)->mlh_TailPred) == (struct MinNode *)(x) )
#endif

/* NewList/Remove as macro:
 * NewList(x) \
 *	(x)->mlh_Head = (struct MinNode *)&((x)->mlh_Tail); \
 *	(x)->mlh_Tail = NULL; \
 *	(x)->mlh_TailPred = (struct MinNode *)(x)
 *
 * Remove(x) \
 *	(x)->prev->next = (x)->next;
 *	(x)->next->prev = (x)->prev;
 */


/**************************************
 * Proto
 **************************************/

BOOL OpenDiskDevice(struct FileSysStartupMsg *startup, struct MsgPort **port, struct IOExtTD **request, BOOL *trackdisk);
error_t dev_GetBlocks(uint8 *buffer, int32 blocks, uint32 blocknr);
error_t dev_GetBlocksDS(uint8 *buffer, int32 blocks, uint32 blocknr);
error_t dev_WriteBlocksDS(uint8 *buffer, int32 blocks, uint32 blocknr);
error_t dev_WriteBlocks(uint8 *buffer, int32 blocks, uint32 blocknr);
error_t dev_WriteBlocksDummy(uint8 *buffer, int32 blocks, uint32 blocknr);
void *AllocBufMem(uint32 size);
void FreeBufMem (void *mem);
int DoSCSICommand(UBYTE *data, ULONG datalen, UBYTE *command, UWORD commandlen, UBYTE direction);
UBYTE *BCPLtoCString(STRPTR dest, UBYTE *src);
BOOL DetectAccessmode(UBYTE *buffer, BOOL scsidirectfirst);

/* reflect updated stats in gui */
void guiUpdateStats(void);
void guiStatus(int level, char *message, long maxval);
void guiProgress(int level, long progress);
void guiMsg(const char *format, ...);
void dummyMsg(char *message);
int guiAskUser(char *message, char *okstr, char *cancelstr);

/* functions to call when error found or fixed and new block started */
void clearstats(void);
void adderror(char *message);
void fixederror(char *message);
void enterblock(uint32 blocknr);
void exitblock(void);

/* device.c */
error_t InitCache(uint32 linesize, uint32 nolines);
error_t c_GetBlock(uint8 *data, uint32 bloknr, uint32 bytes);
error_t c_WriteBlock(uint8 *data, uint32 bloknr, uint32 bytes);
void UpdateCache(void);
void FreeCache(void);

/* access.c */
BOOL AccessTest(void);
cachedblock_t *GetBuildBlock(uint16 bloktype, uint32 seqnr);
error_t GetResBlock(cachedblock_t *blok, uint16 bloktype, uint32 seqnr, bool fix);
bool GetAnode(canode_t *anode, uint32 anodenr, bool fix);
bool SaveAnode(canode_t *anode, uint32 nr);

/* standardsscan.c */
error_t StandardScan(uint32 opties);
bool GetPFS2Revision(char *vstring);
error_t vol_GetBlock(cachedblock_t *blok, ULONG bloknr);
error_t vol_WriteBlock(cachedblock_t *blok);
bool IsRootBlock(rootblock_t *r);
error_t ResBlockUsed(uint32 bloknr);
error_t RepairSuperIndex(uint32 *bloknr, uint32 seqnr);
error_t RepairAnodeIndex(uint32 *bloknr, uint32 seqnr);
error_t RepairAnodeBlock(uint32 *bloknr, uint32 seqnr);
error_t RepairBitmapIndex(uint32 *bloknr, uint32 seqnr);
error_t RepairBitmapBlock(uint32 *bloknr, uint32 seqnr);
error_t RepairIndexBlock(uint16 bloktype, error_t (*repairchild)(uint32 *, uint32),
	uint32 *bloknr, uint32 seqnr);
void KillAnodeBitmap(void);
void KillMainBitmap(void);
void KillReservedBitmap(void);

ULONG GetDEFileSize(struct direntry *direntry, struct extrafields *extra, ULONG *high);
ULONG GetDDFileSize(struct deldirentry *dde, ULONG *high);

#if defined(__GNUC__)
static __inline void __chkabort(void) { };
#endif
#if defined(__SASC)
void __regargs __chkabort(void);
#endif

