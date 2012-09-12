/*
 *  ADF Library. (C) 1997-1998 Laurent Clevy
 *
 *  adf_hd.c
 *
 */



#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include"adf_str.h"
#include"hd_blk.h"
#include"adf_raw.h"
#include"adf_hd.h"
#include"adf_util.h"
#include"adf_disk.h"
#include"adf_nativ.h"
#include"adf_dump.h"
#include"adf_err.h"

#include"defendian.h"

extern struct Env adfEnv;

/*
 * adfDevType
 *
 * returns the type of a device
 * only based of the field 'dev->size'
 */
int adfDevType(struct Device* dev)
{
    if (dev->size==512*11*2*80)
        return(DEVTYPE_FLOPDD);
    else if (dev->size==512*22*2*80)
        return(DEVTYPE_FLOPHD);
    else if (dev->size>512*22*2*80)
        return(DEVTYPE_HARDDISK);
    else {
        (*adfEnv.eFct)("adfDevType : unknown device type");
        return(-1);
    }
}


/*
 * adfDeviceInfo
 *
 * display information about the device and its volumes
 * for demonstration purpose only since the output is stdout !
 *
 * can be used before adfCreateVol() or adfMount()
 */
void adfDeviceInfo(struct Device *dev)
{
    int i;
    
    printf("Cylinders   = %ld\n",(long)dev->cylinders);
    printf("Heads       = %ld\n",(long)dev->heads);
    printf("Sectors/Cyl = %ld\n\n",(long)dev->sectors);
    if (!dev->isNativeDev)
        printf("Dump device\n\n");
    else
        printf("Real device\n\n");
    printf("Volumes     = %d\n\n",dev->nVol);
/*
    switch(dev->devType){
    case DEVTYPE_FLOPDD:
        printf("floppy dd\n"); break;
    case DEVTYPE_FLOPHD:
        printf("floppy hd\n"); break;
    case DEVTYPE_HARDDISK:
        printf("harddisk\n"); break;
    case DEVTYPE_HARDFILE:
        printf("hardfile\n"); break;
    default:
        printf("unknown devType!\n"); break;
    }
*/

    for(i=0; i<dev->nVol; i++) {
        if (dev->volList[i]->volName)
            printf("%2d :  %7ld ->%7ld, \"%s\"", i,
            (long)dev->volList[i]->firstBlock,
            (long)dev->volList[i]->lastBlock,
            dev->volList[i]->volName);
        else
            printf("%2d :  %7ld ->%7ld\n", i,
            (long)dev->volList[i]->firstBlock,
            (long)dev->volList[i]->lastBlock);
        if (dev->volList[i]->mounted)
            printf(", mounted");
        putchar('\n');
    }
}


/*
 * adfFreeTmpVolList
 *
 */
void adfFreeTmpVolList(struct List *root)
{
    struct List *cell;
    struct Volume *vol;

    cell = root;
    while(cell!=NULL) {
        vol = (struct Volume *)cell->content;
        if (vol->volName!=NULL)
            free(vol->volName);  
        cell = cell->next;
    }
    freeList(root);

}


/*
 * adfMountHdFile
 *
 */
RETCODE adfMountHdFile(struct Device *dev)
{
    struct Volume* vol;
    unsigned char buf[512];
    ULONG size;
    BOOL found;

    dev->devType = DEVTYPE_HARDFILE;
    dev->nVol = 0;
    dev->volList = (struct Volume**)malloc(sizeof(struct Volume*));
    if (!dev->volList) { 
        (*adfEnv.eFct)("adfMountHdFile : malloc");
        return RC_ERROR;
    }

    vol=(struct Volume*)malloc(sizeof(struct Volume));
    if (!vol) {
        (*adfEnv.eFct)("adfMountHdFile : malloc");
        return RC_ERROR;
    }
    dev->volList[0] = vol;
    dev->nVol++;      /* fixed by Dan, ... and by Gary */

    vol->volName=NULL;
    
    dev->cylinders = dev->size/512;
    dev->heads = 1;
    dev->sectors = 1;

    vol->firstBlock = 0;

    size = dev->size + 512-(dev->size%512);
//printf("size=%ld\n",size);
    vol->rootBlock = ((size/512)-1+2)/2;
//printf("root=%ld\n",vol->rootBlock);
    do {
        adfReadDumpSector(dev, vol->rootBlock, 512, buf);
        found = swapLong(buf)==T_HEADER && swapLong(buf+508)==ST_ROOT;
        if (!found)
            (vol->rootBlock)--;
    }while (vol->rootBlock>1 && !found);

    if (vol->rootBlock==1) {
        (*adfEnv.eFct)("adfMountHdFile : rootblock not found");
        return RC_ERROR;
    }
    vol->lastBlock = vol->rootBlock*2 - 1 ;

    return RC_OK;
}


/*
 * adfMountHd
 *
 * normal not used directly : called by adfMount()
 *
 * fills geometry fields and volumes list (dev->nVol and dev->volList[])
 */
RETCODE adfMountHd(struct Device *dev)
{
    struct bRDSKblock rdsk;
    struct bPARTblock part;
    struct bFSHDblock fshd;
    struct bLSEGblock lseg;
    ULONG next;
    struct List *vList, *listRoot;
    int i;
    struct Volume* vol;
    int len;

    if (adfReadRDSKblock( dev, &rdsk )!=RC_OK)
        return RC_ERROR;

    dev->cylinders = rdsk.cylinders;
    dev->heads = rdsk.heads;
    dev->sectors = rdsk.sectors;

    /* PART blocks */
    listRoot = NULL;
    next = rdsk.partitionList;
    dev->nVol=0;
    vList = NULL;
    while( next!=-1 ) {
        if (adfReadPARTblock( dev, next, &part )!=RC_OK) {
            adfFreeTmpVolList(listRoot);
            (*adfEnv.eFct)("adfMountHd : malloc");
            return RC_ERROR;
        }

        vol=(struct Volume*)malloc(sizeof(struct Volume));
        if (!vol) {
            adfFreeTmpVolList(listRoot);
            (*adfEnv.eFct)("adfMountHd : malloc");
            return RC_ERROR;
        }
        vol->volName=NULL;
        dev->nVol++;

        vol->firstBlock = rdsk.cylBlocks * part.lowCyl;
        vol->lastBlock = (part.highCyl+1)*rdsk.cylBlocks -1 ;
        vol->rootBlock = (vol->lastBlock - vol->firstBlock+1)/2;
        vol->blockSize = part.blockSize*4;

        len = min(31, part.nameLen);
        vol->volName = (char*)malloc(len+1);
        if (!vol->volName) { 
            adfFreeTmpVolList(listRoot);
            (*adfEnv.eFct)("adfMount : malloc");
            return RC_ERROR;
        }
        memcpy(vol->volName,part.name,len);
        vol->volName[len] = '\0';

        vol->mounted = FALSE;

        /* stores temporaly the volumes in a linked list */
        if (listRoot==NULL)
            vList = listRoot = newCell(NULL, (void*)vol);
        else
            vList = newCell(vList, (void*)vol);

        if (vList==NULL) {
            adfFreeTmpVolList(listRoot);
            (*adfEnv.eFct)("adfMount : newCell() malloc");
            return RC_ERROR;
        }

        next = part.next;
    }

    /* stores the list in an array */
    dev->volList = (struct Volume**)malloc(sizeof(struct Volume*) * dev->nVol);
    if (!dev->volList) { 
        adfFreeTmpVolList(listRoot);
        (*adfEnv.eFct)("adfMount : unknown device type");
        return RC_ERROR;
    }
    vList = listRoot;
    for(i=0; i<dev->nVol; i++) {
        dev->volList[i]=(struct Volume*)vList->content;
        vList = vList->next;
    }
    freeList(listRoot);

    fshd.segListBlock = -1;
    next = rdsk.fileSysHdrList;
    while( next!=-1 ) {
        if (adfReadFSHDblock( dev, next, &fshd )!=RC_OK) {
            for(i=0;i<dev->nVol;i++) free(dev->volList[i]);
            free(dev->volList);
            (*adfEnv.eFct)("adfMount : adfReadFSHDblock");
            return RC_ERROR;
        }
        next = fshd.next;
    }

    next = fshd.segListBlock;
    while( next!=-1 ) {
        if (adfReadLSEGblock( dev, next, &lseg )!=RC_OK) {
            (*adfEnv.wFct)("adfMount : adfReadLSEGblock");
        }
        next = lseg.next;
    }

    return RC_OK;
}


/*
 * adfMountFlop
 *
 * normaly not used directly, called directly by adfMount()
 *
 * use dev->devType to choose between DD and HD
 * fills geometry and the volume list with one volume
 */
RETCODE adfMountFlop(struct Device* dev)
{
    struct Volume *vol;
    struct bRootBlock root;
    char diskName[35];
    
    dev->cylinders = 80;
    dev->heads = 2;
    if (dev->devType==DEVTYPE_FLOPDD)
        dev->sectors = 11;
    else 
        dev->sectors = 22;

    vol=(struct Volume*)malloc(sizeof(struct Volume));
    if (!vol) { 
        (*adfEnv.eFct)("adfMount : malloc");
        return RC_ERROR;
    }

    vol->mounted = TRUE;
    vol->firstBlock = 0;
    vol->lastBlock =(dev->cylinders * dev->heads * dev->sectors)-1;
    vol->rootBlock = (vol->lastBlock+1 - vol->firstBlock)/2;
    vol->blockSize = 512;
    vol->dev = dev;
 
    if (adfReadRootBlock(vol, vol->rootBlock, &root)!=RC_OK)
        return RC_ERROR;
    memset(diskName, 0, 35);
    memcpy(diskName, root.diskName, root.nameLen);

    vol->volName = strdup(diskName);
    
    dev->volList =(struct Volume**) malloc(sizeof(struct Volume*));
    if (!dev->volList) {
        free(vol);
        (*adfEnv.eFct)("adfMount : malloc");
        return RC_ERROR;
    }
    dev->volList[0] = vol;
    dev->nVol = 1;

/*printf("root=%d\n",vol->rootBlock);       */
    return RC_OK;
}


/*
 * adfMountDev
 *
 * mount a dump file (.adf) or a real device (uses adf_nativ.c and .h)
 *
 * adfInitDevice() must fill dev->size !
 */
struct Device* adfMountDev( char* filename, BOOL ro)
{
    struct Device* dev;
    struct nativeFunctions *nFct;
    RETCODE rc;
    UBYTE buf[512];

    dev = (struct Device*)malloc(sizeof(struct Device));
    if (!dev) {
        (*adfEnv.eFct)("adfMountDev : malloc error");
        return NULL;
    }

    dev->readOnly = ro;

    /* switch between dump files and real devices */
    nFct = adfEnv.nativeFct;
    dev->isNativeDev = (*nFct->adfIsDevNative)(filename);
    if (dev->isNativeDev)
        rc = (*nFct->adfInitDevice)(dev, filename,ro);
    else
        rc = adfInitDumpDevice(dev,filename,ro);
    if (rc!=RC_OK) {
        free(dev); return(NULL);
    }

    dev->devType = adfDevType(dev);

    switch( dev->devType ) {

    case DEVTYPE_FLOPDD:
    case DEVTYPE_FLOPHD:
        if (adfMountFlop(dev)!=RC_OK) {
            free(dev); return NULL;
        }
        break;

    case DEVTYPE_HARDDISK:
        /* to choose between hardfile or harddisk (real or dump) */
        if (adfReadDumpSector(dev, 0, 512, buf)!=RC_OK) {
            (*adfEnv.eFct)("adfMountDev : adfReadDumpSector failed");
            free(dev); return NULL;
        }

        /* a file with the first three bytes equal to 'DOS' */
        if (!dev->isNativeDev && strncmp("DOS",(char *)buf,3)==0) {
            if (adfMountHdFile(dev)!=RC_OK) {
                free(dev); return NULL;
            }
        }
        else if (adfMountHd(dev)!=RC_OK) {
            /* Just return the device */
            return dev;
        }
        break;

    default:
        (*adfEnv.eFct)("adfMountDev : unknown device type");
    }

    return dev;
}


/*
 * adfCreateHdHeader
 *
 * create PARTIALLY the sectors of the header of one harddisk : can not be mounted
 * back on a real Amiga ! It's because some device dependant values can't be guessed...
 *
 * do not use dev->volList[], but partList for partitions information : start and len are cylinders,
 *  not blocks
 * do not fill dev->volList[]
 * called by adfCreateHd()
 */
RETCODE adfCreateHdHeader(struct Device* dev, int n, struct Partition** partList )
{
    int i;
    struct bRDSKblock rdsk;
    struct bPARTblock part;
    SECTNUM j;
    int len;

    /* RDSK */ 
 
    memset((unsigned char*)&rdsk,0,sizeof(struct bRDSKblock));
    memset((unsigned char*)&part,0,sizeof(struct bPARTblock));

    rdsk.rdbBlockLo = 0;
    rdsk.rdbBlockHi = (dev->sectors*dev->heads*2)-1;
    rdsk.loCylinder = 2;
    rdsk.hiCylinder = dev->cylinders-1;
    rdsk.cylBlocks = dev->sectors*dev->heads;

    rdsk.cylinders = dev->cylinders;
    rdsk.sectors = dev->sectors;
    rdsk.heads = dev->heads;
    
    rdsk.badBlockList = -1;
    rdsk.partitionList = 1;
    rdsk.fileSysHdrList = -1;
    rdsk.driveInit = -1;
    rdsk.bootBlockList = -1;
    for ( i = 0; i < 5; i++)
        rdsk.r1[i] = -1;

    /* Clear all sectors in the header */
    for (i = rdsk.rdbBlockLo; i <= rdsk.rdbBlockHi; i++)
        adfWriteBlockDev(dev, i, sizeof(part), (unsigned char *)&part);
    
    if (adfWriteRDSKblock(dev, &rdsk)!=RC_OK)
        return RC_ERROR;

    /* PART */

    j=1;
    for(i=0; i<dev->nVol; i++) {
        memset(&part, 0, sizeof(struct bPARTblock));

        if (i<dev->nVol-1)
            part.next = j+1;
        else
            part.next = -1;

        len = min(MAXNAMELEN,strlen(partList[i]->volName));
        part.nameLen = len;
        strncpy(part.name, partList[i]->volName, len);

        part.surfaces = dev->heads;
        part.blocksPerTrack = dev->sectors;
        part.lowCyl = partList[i]->startCyl;
        part.highCyl = partList[i]->startCyl + partList[i]->lenCyl -1;
        part.maxTransfer = 0x00ffffff;
        part.mask = 0xfffffffe;
        part.dosReserved = 2;
        part.numBuffer = 100;
        part.vectorSize = 16;
        part.blockSize = 128;
        part.sectorsPerBlock = 1;

        memcpy(part.dosType, partList[i]->volType, 4);
            
        if (adfWritePARTblock(dev, j, &part))
            return RC_ERROR;
        j++;
    }

    return RC_OK;
}


/*
 * adfCreateFlop
 *
 * create a filesystem on a floppy device
 * fills dev->volList[]
 */
RETCODE adfCreateFlop(struct Device* dev, char* volName, int volType )
{
    if (dev==NULL) {
        (*adfEnv.eFct)("adfCreateFlop : dev==NULL");
        return RC_ERROR;
    }
    dev->volList =(struct Volume**) malloc(sizeof(struct Volume*));
    if (!dev->volList) { 
        (*adfEnv.eFct)("adfCreateFlop : unknown device type");
        return RC_ERROR;
    }
    dev->volList[0] = adfCreateVol( dev, 0L, 80L, volName, volType );
    if (dev->volList[0]==NULL) {
        free(dev->volList);
        return RC_ERROR;
    }
    dev->nVol = 1;
    dev->volList[0]->blockSize = 512;
    if (dev->sectors==11)
        dev->devType=DEVTYPE_FLOPDD;
    else
        dev->devType=DEVTYPE_FLOPHD;

    return RC_OK;
}


/*
 * adfCreateHd
 *
 * create a filesystem one an harddisk device (partitions==volumes, and the header)
 *
 * fills dev->volList[]
 *
 */
RETCODE adfCreateHd(struct Device* dev, int n, struct Partition** partList )
{
    int i, j;

//struct Volume *vol;

    if (dev==NULL || partList==NULL || n<=0) {
        (*adfEnv.eFct)("adfCreateHd : illegal parameter(s)");
        return RC_ERROR;
    }

    dev->volList =(struct Volume**) malloc(sizeof(struct Volume*)*n);
    if (!dev->volList) {
        (*adfEnv.eFct)("adfCreateFlop : malloc");
        return RC_ERROR;
    }
    for(i=0; i<n; i++) {
        if (memcmp(partList[i]->volType, "DOS", 3) != 0) {
            (*adfEnv.eFct)("adfCreateHd : Skipping non-DOS volume\n");
            continue;
        }
        dev->volList[i] = adfCreateVol( dev, 
                    partList[i]->startCyl, 
                    partList[i]->lenCyl, 
                    partList[i]->volName, 
                    partList[i]->volType[3] );
        if (dev->volList[i]==NULL) {
           for(j=0; j<i; j++) {
               free( dev->volList[i] );
/* pas fini */
           }
           free(dev->volList);
           (*adfEnv.eFct)("adfCreateHd : adfCreateVol() fails");
        }
        dev->volList[i]->blockSize = 512;
    }
    dev->nVol = n;
/*
vol=dev->volList[0];
printf("0first=%ld last=%ld root=%ld\n",vol->firstBlock,
 vol->lastBlock, vol->rootBlock);
*/

    if (adfCreateHdHeader(dev, n, partList )!=RC_OK)
        return RC_ERROR;
    return RC_OK;
}


/*
 * adfUnMountDev
 *
 */
void adfUnMountDev( struct Device* dev)
{
    int i;
    struct nativeFunctions *nFct;

    if (dev==0)
       return;

    for(i=0; i<dev->nVol; i++) {
        free(dev->volList[i]->volName);
        free(dev->volList[i]);
    }
    if (dev->nVol>0)
        free(dev->volList);
    dev->nVol = 0;

    nFct = adfEnv.nativeFct;
    if (dev->isNativeDev)
        (*nFct->adfReleaseDevice)(dev);
    else
        adfReleaseDumpDevice(dev);

    free(dev);
}

/*
 * adfReadBlockDev
 */
RETCODE adfReadBlockDev( struct Device* dev, ULONG nSect, ULONG size, unsigned char* buf )
{
    struct nativeFunctions *nFct;
    RETCODE rc;
    
    nFct = adfEnv.nativeFct;
    if (dev->isNativeDev)
        rc = (*nFct->adfNativeReadSector)(dev, nSect, size, buf);
    else
        rc = adfReadDumpSector(dev, nSect, size, buf);

    if (rc != RC_OK)
        return rc;

    if (buf[0] == 0 && buf[1] == 0 && buf[2] == 0 && buf[3] == 0)
        rc |= RC_BLOCKTYPE;

    if ( Long(buf + 8) == 0)
        rc |= RC_BLOCKSTYPE;

    if ( Long(buf + 8) != adfNormalSum(buf,8,Long(buf + 4) * 4) )
         rc |= RC_BLOCKSUM;

    return rc;
}

/*
 * adfWriteBlockDev
 */
RETCODE adfWriteBlockDev( struct Device* dev, ULONG nSect, ULONG size, unsigned char* buf )
{
    struct nativeFunctions *nFct;
    ULONG sum;

    memset(buf+8, 0, 4);
    sum = adfNormalSum(buf, 8, Long(buf + 4) * 4);
    swLong(buf+8, sum);
    
    nFct = adfEnv.nativeFct;
    if (dev->isNativeDev)
        return (*nFct->adfNativeWriteSector)(dev, nSect, size, buf);
    else
        return adfWriteDumpSector(dev, nSect, size, buf);
}

/*
 * ReadRDSKblock
 *
 */
    RETCODE
adfReadRDSKblock( struct Device* dev, struct bRDSKblock* blk )
{

    UCHAR buf[256];
    RETCODE rc2;
    RETCODE rc = RC_OK;

    rc2 = adfReadBlockDev(dev, 0, 256, buf);
    if (rc2!=RC_OK)
       return(RC_ERROR);

    memcpy(blk, buf, 256);
#ifdef LITT_ENDIAN
    /* big to little = 68000 to x86 */
    swapEndian((unsigned char*)blk, SWBL_RDSK);
#endif

    if ( strncmp(blk->id,"RDSK",4)!=0 ) {
        (*adfEnv.eFct)("ReadRDSKblock : RDSK id not found");
        return RC_ERROR;
    }

    if ( blk->size != 64 )
        (*adfEnv.wFct)("ReadRDSKBlock : size != 64\n");

    if ( blk->blockSize != 512 )
         (*adfEnv.wFct)("ReadRDSKBlock : blockSize != 512\n");

    if ( blk->cylBlocks !=  blk->sectors*blk->heads )
        (*adfEnv.wFct)( "ReadRDSKBlock : cylBlocks != sectors*heads");

    return rc;
}


/*
 * adfWriteRDSKblock
 *
 */
    RETCODE
adfWriteRDSKblock(struct Device *dev, struct bRDSKblock* rdsk)
{
    unsigned char buf[LOGICAL_BLOCK_SIZE];

    if (dev->readOnly) {
        (*adfEnv.wFct)("adfWriteRDSKblock : can't write block, read only device");
        return RC_ERROR;
    }

    memset(buf,0,LOGICAL_BLOCK_SIZE);

    strncpy(rdsk->id,"RDSK",4);
    rdsk->size = sizeof(struct bRDSKblock)/sizeof(ULONG);
    rdsk->blockSize = LOGICAL_BLOCK_SIZE;
    rdsk->badBlockList = -1;

    strncpy(rdsk->diskVendor,"ADFlib  ",8);
    strncpy(rdsk->diskProduct,"harddisk.adf    ",16);
    strncpy(rdsk->diskRevision,"v1.0",4);

    memcpy(buf, rdsk, sizeof(struct bRDSKblock));
#ifdef LITT_ENDIAN
    swapEndian(buf, SWBL_RDSK);
#endif

    return adfWriteBlockDev(dev, 0, LOGICAL_BLOCK_SIZE, buf);
}


/*
 * ReadPARTblock
 *
 */
    RETCODE
adfReadPARTblock( struct Device* dev, ULONG nSect, struct bPARTblock* blk )
{
    UCHAR buf[ sizeof(struct bPARTblock) ];
    RETCODE rc2, rc = RC_OK;

    rc2 = adfReadBlockDev(dev, nSect, sizeof(struct bPARTblock), buf);
    if (rc2!=RC_OK)
       return RC_ERROR;

    memcpy(blk, buf, sizeof(struct bPARTblock));
#ifdef LITT_ENDIAN
    /* big to little = 68000 to x86 */
    swapEndian((unsigned char*)blk, SWBL_PART);
#endif

    if ( strncmp(blk->id,"PART",4)!=0 ) {
        (*adfEnv.eFct)("ReadPARTblock : PART id not found");
        return RC_ERROR;
    }

    if ( blk->size != 64 )
        (*adfEnv.wFct)("ReadPARTBlock : size != 64");

    if ( blk->blockSize!=128 ) {
        (*adfEnv.eFct)("ReadPARTblock : blockSize!=512, not supported (yet)");
        return RC_ERROR;
    }

    return rc;
}


/*
 * adfWritePARTblock
 *
 */
    RETCODE
adfWritePARTblock(struct Device *dev, ULONG nSect, struct bPARTblock* part)
{
    unsigned char buf[LOGICAL_BLOCK_SIZE];
    
    if (dev->readOnly) {
        (*adfEnv.wFct)("adfWritePARTblock : can't write block, read only device");
        return RC_ERROR;
    }

    memset(buf,0,LOGICAL_BLOCK_SIZE);

    strncpy(part->id,"PART",4);
    part->size = sizeof(struct bPARTblock)/sizeof(ULONG);
    part->blockSize = LOGICAL_BLOCK_SIZE;
    part->vectorSize = 16;
    part->blockSize = 128;
    part->sectorsPerBlock = 1;
    part->dosReserved = 2;

    memcpy(buf, part, sizeof(struct bPARTblock));
#ifdef LITT_ENDIAN
    swapEndian(buf, SWBL_PART);
#endif

    return adfWriteBlockDev(dev, nSect, LOGICAL_BLOCK_SIZE, buf);
}

/*
 * ReadFSHDblock
 *
 */
    RETCODE
adfReadFSHDblock( struct Device* dev, ULONG nSect, struct bFSHDblock* blk)
{
    UCHAR buf[sizeof(struct bFSHDblock)];
    RETCODE rc;
   
    rc = adfReadBlockDev(dev, nSect, sizeof(struct bFSHDblock), buf);
    if (rc!=RC_OK)
        return RC_ERROR;
        
    memcpy(blk, buf, sizeof(struct bFSHDblock));
#ifdef LITT_ENDIAN
    /* big to little = 68000 to x86 */
    swapEndian((unsigned char*)blk, SWBL_FSHD);
#endif

    if ( strncmp(blk->id,"FSHD",4)!=0 ) {
        (*adfEnv.eFct)("ReadFSHDblock : FSHD id not found");
        return RC_ERROR;
    }

    if ( blk->size != 64 )
         (*adfEnv.wFct)("ReadFSHDblock : size != 64");

    return RC_OK;
}


/*
 *  adfWriteFSHDblock
 *
 */
    RETCODE
adfWriteFSHDblock(struct Device *dev, ULONG nSect, struct bFSHDblock* fshd)
{
    unsigned char buf[LOGICAL_BLOCK_SIZE];

    if (dev->readOnly) {
        (*adfEnv.wFct)("adfWriteFSHDblock : can't write block, read only device");
        return RC_ERROR;
    }

    memset(buf,0,LOGICAL_BLOCK_SIZE);

    strncpy(fshd->id,"FSHD",4);
    fshd->size = sizeof(struct bFSHDblock)/sizeof(ULONG);

    memcpy(buf, fshd, sizeof(struct bFSHDblock));
#ifdef LITT_ENDIAN
    swapEndian(buf, SWBL_FSHD);
#endif

    return adfWriteBlockDev(dev, nSect, LOGICAL_BLOCK_SIZE, buf);
}


/*
 * ReadLSEGblock
 *
 */
   RETCODE
adfReadLSEGblock(struct Device* dev, ULONG nSect, struct bLSEGblock* blk)
{
    UCHAR buf[sizeof(struct bLSEGblock)];
    RETCODE rc;
   
    rc = adfReadBlockDev(dev, nSect, sizeof(struct bLSEGblock), buf);
    if (rc!=RC_OK)
        return RC_ERROR;
        
    memcpy(blk, buf, sizeof(struct bLSEGblock));
#ifdef LITT_ENDIAN
    /* big to little = 68000 to x86 */
    swapEndian((unsigned char*)blk, SWBL_LSEG);
#endif

    if ( strncmp(blk->id,"LSEG",4)!=0 ) {
        (*adfEnv.eFct)("ReadLSEGblock : LSEG id not found");
        return RC_ERROR;
    }

    if ( blk->next!=-1 && blk->size != 128 )
        (*adfEnv.wFct)("ReadLSEGBlock : size != 128");

    return RC_OK;
}


/*
 * adfWriteLSEGblock
 *
 */
    RETCODE
adfWriteLSEGblock(struct Device *dev, ULONG nSect, struct bLSEGblock* lseg)
{
    unsigned char buf[LOGICAL_BLOCK_SIZE];

    if (dev->readOnly) {
        (*adfEnv.wFct)("adfWriteLSEGblock : can't write block, read only device");
        return RC_ERROR;
    }

    memset(buf,0,LOGICAL_BLOCK_SIZE);

    strncpy(lseg->id,"LSEG",4);
    lseg->size = sizeof(struct bLSEGblock)/sizeof(ULONG);

    memcpy(buf, lseg, sizeof(struct bLSEGblock));
#ifdef LITT_ENDIAN
    swapEndian(buf, SWBL_LSEG);
#endif

    return adfWriteBlockDev(dev, nSect, LOGICAL_BLOCK_SIZE, buf);
}

/*
 * ReadBOOTblock
 *
 */
   RETCODE
adfReadBOOTblock(struct Device* dev, ULONG nSect, struct bBOOTblock* blk)
{
    UCHAR buf[sizeof(struct bBOOTblock)];
    RETCODE rc;
   
    rc = adfReadBlockDev(dev, nSect, sizeof(struct bBOOTblock), buf);
    if (rc!=RC_OK)
        return RC_ERROR;
        
    memcpy(blk, buf, sizeof(struct bBOOTblock));
#ifdef LITT_ENDIAN
    /* big to little = 68000 to x86 */
    swapEndian((unsigned char*)blk, SWBL_BOOT);
#endif

    if ( strncmp(blk->id,"BOOT",4)!=0 ) {
        (*adfEnv.eFct)("ReadBOOTblock : BOOT id not found");
        return RC_ERROR;
    }

    if ( blk->next!=-1 && blk->size != 128 )
        (*adfEnv.wFct)("ReadBOOTBlock : size != 128");

    return RC_OK;
}


/*
 * adfWriteBOOTblock
 *
 */
    RETCODE
adfWriteBOOTblock(struct Device *dev, ULONG nSect, struct bBOOTblock* lseg)
{
    unsigned char buf[LOGICAL_BLOCK_SIZE];

    if (dev->readOnly) {
        (*adfEnv.wFct)("adfWriteBOOTblock : can't write block, read only device");
        return RC_ERROR;
    }

    memset(buf,0,LOGICAL_BLOCK_SIZE);

    strncpy(lseg->id,"BOOT",4);
    lseg->size = sizeof(struct bBOOTblock)/sizeof(ULONG);

    memcpy(buf, lseg, sizeof(struct bBOOTblock));
#ifdef LITT_ENDIAN
    swapEndian(buf, SWBL_BOOT);
#endif

    return adfWriteBlockDev(dev, nSect, LOGICAL_BLOCK_SIZE, buf);
}

/* Write BOOT code
 */
RETCODE adfWriteBOOT(struct Device *dev, const UBYTE *code, size_t size)
{
    struct bRDSKblock rdsk;
    struct bBOOTblock boot;
    UBYTE buf[LOGICAL_BLOCK_SIZE];
    ULONG *sector, *next_block;
    int i, sectors, n;
    RETCODE rc;

    if (size & 3) {
        printf("adfWriteBOOT: Size must be a multiple of 4\n");
        return RC_ERROR;
    }

    rc = adfReadRDSKblock(dev, &rdsk);
    if (rc != RC_OK)
        return rc;

    /* Sector map of sectors we can use for boot code */
    sectors = (size + sizeof(boot.loadData) - 1) / sizeof(boot.loadData);
    sector = alloca((rdsk.rdbBlockHi - rdsk.rdbBlockLo + 1) * sizeof(sector[0]));

    /* Do we have space for the new boot code? */
    next_block = &rdsk.bootBlockList;
    for (n = 0, i = rdsk.rdbBlockLo; i <= rdsk.rdbBlockHi; i++) {
        sector[i - rdsk.rdbBlockLo] = (ULONG)-1;
        rc = adfReadBlockDev(dev, i, sizeof(buf), buf);
        if (rc == RC_OK) {
            if (memcmp(buf, "BOOT", 4) != 0)
                continue;
        } else if (rc & RC_BLOCKREAD) {
            /* Skip unreadable blocks */
            continue;
        }
        if (n < sectors) {
            *next_block = i;
            next_block = &sector[i - rdsk.rdbBlockLo];
            n++;
        } else
            sector[i - rdsk.rdbBlockLo] = (ULONG)-2;       /* Erase sector */
    }
    *next_block = (ULONG)-1;

    if (n < sectors) {
        printf("Needed %d sectors for the BOOT code, only found %d\n", sectors, n);
        return RC_ERROR;
    }

    /* Clear out unused sectors */
    memset(buf, 0, 512);
    for (i = rdsk.rdbBlockLo; i <= rdsk.rdbBlockHi; i++) {
        if (sector[i - rdsk.rdbBlockLo] == (ULONG)-2)
            adfWriteBlockDev(dev, i, sizeof(buf), buf);
    }

    /* Write the new boot code */
    n = rdsk.bootBlockList;
    while (size > 0) {
        int longs = (size > sizeof(boot.loadData)) ? sizeof(boot.loadData) : size;
        longs = (longs + 3) / 4;
        boot.size = longs + 5;
        boot.hostID = 7;        /* Default */
        boot.next = sector[n - rdsk.rdbBlockLo];
        memset(boot.loadData, 0xff, sizeof(boot.loadData));
        for (i = 0; i < longs; i++) {
            boot.loadData[i] = *(ULONG *)code;
            code += 4;
            size -= 4;
        }

        adfWriteBOOTblock(dev, n, &boot);

        n = sector[n - rdsk.rdbBlockLo];
    }

    return adfWriteRDSKblock(dev, &rdsk);
}

/*##########################################################################*/
