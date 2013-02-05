#include <clib/macros.h>
#include <devices/scsidisk.h>
#include <devices/trackdisk.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/filehandler.h>
#include <exec/errors.h>
#include <exec/io.h>
#include <exec/memory.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <string.h>
#include "stdio.h"

#include "../FS/globals.h"
#include "../FS/deviceio.h"
#include "../FS/deviceio_protos.h"
#include "../FS/cachedio_protos.h"
#include "../FS/asmsupport.h"
#include "../FS/adminspaces.h"
#include "../FS/bitmap.h"
#include "../FS/objects.h"
#include "../FS/transactions.h"
#include "../FS/fs.h"
#include "../FS/btreenodes.h"


#define BLCKFACCURACY   (5)                          /* 2^5 = 32 */


const char version[]={"\0$VER: SFSCheck 1.3 (" ADATE ")r\n"};


LONG read2(ULONG block);
void dumpblock(ULONG block, void *data, ULONG bytes);
void sfscheck(void);
BOOL iszero(void *a,LONG size);
BOOL isblockvalid(void *b, ULONG block, ULONG id);
LONG findbnode(BLCK rootblock,ULONG key,struct BNode **returned_bnode);
UBYTE *mark(LONG offset, LONG blocks);
BOOL error(UBYTE *fmt, ... );
void freestring(UBYTE *str);
UBYTE *tostring(UBYTE *fmt, ... );

ULONG errors=0;
ULONG maxerrors=10;
UBYTE emptystring[]="";

UBYTE string[200];

struct DosEnvec *dosenvec;

/* blocks_  = the number of blocks of something (blocks can contain 1 or more sectors)
   block_   = a block number of something (relative to start of partition)
   sectors_ = the number of sectors of something (1 or more sectors form a logical block)
   sector_  = a sector number (relative to the start of the disk)
   bytes_   = the number of bytes of something
   byte_    = a byte number of something
   shifts_  = the number of bytes written as 2^x of something
   mask_    = a mask for something */

ULONG blocks_reserved_start;      /* number of blocks reserved at start (=reserved) */
ULONG blocks_reserved_end;        /* number of blocks reserved at end (=prealloc) */
ULONG blocks_bitmap;              /* number of BTMP blocks for this partition */
ULONG blocks_inbitmap;            /* number of blocks a single bitmap block can contain info on */
ULONG blocks_admin;               /* the size of all AdminSpaces */

ULONG block_root;                 /* the block offset of the root block */
ULONG block_bitmapbase;           /* the block offset of the first bitmap block */
ULONG block_extentbnoderoot;      /* the block offset of the root of the extent bnode tree */
ULONG block_adminspace;           /* the block offset of the first adminspacecontainer block */
ULONG block_rovingblockptr;       /* the roving block pointer! */
ULONG block_objectnodesbase;

ULONG node_containers;            /* number of containers per ExtentIndexContainer */

ULONG mask_block32;               /* masks the least significant bits of a BLCKf pointer */

UWORD shifts_block32;             /* shift count needed to convert a blockoffset<->32byteoffset (only used by nodes.c!) */

void *pool;

UBYTE *buffer;
ULONG *bitmap;

ULONG returncode=0;

LONG main() {
  struct RDArgs *readarg;
  UBYTE template[]="DEVICE=DRIVE/A,LOCK/S,LINES/N,READAHEADSIZE/N\n";

  struct {char *device;
          ULONG lock;
          ULONG *lines;
          ULONG *readaheadsize;} arglist={NULL};

  globals = AllocMem(sizeof(struct SFSBase), MEMF_PUBLIC | MEMF_CLEAR);
  if(globals==NULL)
    return RETURN_FAIL;
  initGlobals();

  if((DOSBase=(struct DosLibrary *)OpenLibrary("dos.library",39))!=0) {
    if((IntuitionBase=(APTR)OpenLibrary("intuition.library",39))!=0) {
      if((pool=CreatePool(0,16384,8192))!=0) {
        if((readarg=ReadArgs(template,(IPTR *)&arglist,0))!=0) {
          struct DosList *dl;
          UBYTE *devname=arglist.device;

          while(*devname!=0) {
            if(*devname==':') {
              *devname=0;
              break;
            }
            devname++;
          }

          dl=LockDosList(LDF_DEVICES|LDF_READ);
          if((dl=FindDosEntry(dl,arglist.device,LDF_DEVICES))!=0) {
            struct FileSysStartupMsg *fssm;
            LONG errorcode;

            fssm=(struct FileSysStartupMsg *)BADDR(dl->dol_misc.dol_handler.dol_Startup);
            dosenvec=(struct DosEnvec *)BADDR(fssm->fssm_Environ);

            UnLockDosList(LDF_DEVICES|LDF_READ);

            if(arglist.lock==0 || Inhibit(arglist.device, DOSTRUE)!=DOSFALSE) {

              if((initcachedio(AROS_BSTR_ADDR(fssm->fssm_Device), fssm->fssm_Unit, fssm->fssm_Flags, dosenvec))==0) {

                setiocache(arglist.lines!=0 ? *arglist.lines : 128, arglist.readaheadsize!=0 ? *arglist.readaheadsize : 8192, FALSE);     /* 1 MB for read-ahead cache, no copyback mode. */

                shifts_block32=globals->shifts_block-BLCKFACCURACY;

                mask_block32=(1<<shifts_block32)-1;

                blocks_reserved_start=MAX(dosenvec->de_Reserved,1);
                blocks_reserved_end=MAX(dosenvec->de_PreAlloc,1);

                blocks_inbitmap=(globals->bytes_block-sizeof(struct fsBitmap))<<3;  /* must be a multiple of 32 !! */
                blocks_bitmap=(globals->blocks_total+blocks_inbitmap-1)/blocks_inbitmap;
                blocks_admin=32;

                printf("Partition start offset : 0x%llx   End offset : 0x%llx\n", (unsigned long long)globals->byte_low, (unsigned long long)globals->byte_high);
                printf("Surfaces         : %-5ld   Blocks/Track  : %ld\n", dosenvec->de_Surfaces, dosenvec->de_BlocksPerTrack);
                printf("Bytes/Block      : %-5ld   Sectors/Block : %ld\n", (long)globals->bytes_block, dosenvec->de_SectorPerBlock);
                printf("Total blocks     : %ld\n", (long)globals->blocks_total);
                printf("Device interface : ");

                switch(deviceapiused()) {
                case DAU_NSD:
                  printf("NSD (64-bit)\n");
                  break;
                case DAU_TD64:
                  printf("TD64\n");
                  break;
                case DAU_SCSIDIRECT:
                  printf("SCSI direct\n");
                  break;
                default:
                  printf("(standard)\n");
                  break;
                }

                if((buffer=AllocVec(globals->bytes_block, globals->bufmemtype))!=0) {
                  if((bitmap=AllocVec(((globals->blocks_total+31)>>5)<<3,MEMF_CLEAR))!=0) {
                    UBYTE *str;

                    bitmap[globals->blocks_total>>5]=L2BE(0xFFFFFFFF>>(globals->blocks_total & 0x0000001F));
                    if((str=mark(0,blocks_reserved_start))!=0) {
                      printf("Error while marking reserved blocks at start:\n%s",str);
                      freestring(str);
                    }
                    if((str=mark(globals->blocks_total-blocks_reserved_end,blocks_reserved_end))!=0) {
                      printf("Error while marking reserved blocks at end:\n%s",str);
                      freestring(str);
                    }

                    sfscheck();

                    FreeVec(bitmap);
                  }
                  else {
                    printf("Not enough memory\n");
                  }
                  FreeVec(buffer);
                }
                else {
                  printf("Not enough memory\n");
                }

                cleanupcachedio();
              }

              if(arglist.lock!=0) {
                Inhibit(arglist.device, DOSFALSE);
              }
            }
            else {
              errorcode = IoErr();
              PrintFault(errorcode, "error while locking the drive");
            }
          }
          else {
            VPrintf("Unknown device %s\n", (IPTR *)&arglist.device);
            UnLockDosList(LDF_DEVICES|LDF_READ);
          }

          FreeArgs(readarg);
        }
        DeletePool(pool);
      }
      CloseLibrary((struct Library *)IntuitionBase);
    }
    CloseLibrary((struct Library *)DOSBase);
  }
  return(returncode);
}



/*
            else if(SetSignal(0L,SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C) {
              PutStr("\n***Break\n");
              break;
            }
*/


BOOL validblock(ULONG block) {
  if(block<globals->blocks_total) {
    return(TRUE);
  }
  return(FALSE);
}



LONG read2(ULONG block) {
//  return(transfer(DIO_READ, buffer, block, 1));
  return(read(block, buffer, 1));
}



BOOL checkchecksum(void *d) {
  ULONG *data=(ULONG *)d;

  if(CALCCHECKSUM(globals->bytes_block,data)==0) {
    return(TRUE);
  }
  return(FALSE);
}



BOOL checkobjectcontainerblock(struct fsObjectContainer *b, ULONG block) {
  if(isblockvalid(b,block,OBJECTCONTAINER_ID)!=FALSE) {

  }

  return(FALSE);
}



BOOL checkbitmapblock(struct fsBitmap *b, ULONG block, ULONG sequencenumber) {
  if(isblockvalid(b,block,BITMAP_ID)!=FALSE) {
    ULONG l=(sequencenumber-1)*(blocks_inbitmap>>5);
    WORD n=blocks_inbitmap>>5;
    ULONG *bm=b->bitmap;

    if(sequencenumber==blocks_bitmap) {
      n=((globals->blocks_total - ((blocks_bitmap-1) * blocks_inbitmap))+31)>>5;
    }

    while(--n>=0 && ~bitmap[l++]==*bm++) {
    }


    if(n<0) {
      if(sequencenumber==blocks_bitmap) {
        if((bmffo(b->bitmap, blocks_inbitmap>>5, globals->blocks_total - ((blocks_bitmap-1) * blocks_inbitmap) ))==blocks_inbitmap) {
          return(TRUE);
        }
        else {
          printf("Last bitmap block wasn't correctly filled out with 0's\n");
        }
      }
      else {
        return(TRUE);
      }
    }
    else {
    printf("%ld %08lx ==  %08lx\n",(long)n,(unsigned long)~bitmap[--l],(unsigned long)*--bm);
      printf("Bitmap block %ld contents is incorrect!\n",(long)block);
      return(TRUE);
    }
  }

  return(FALSE);
}



BOOL checkbitmap(ULONG block) {
  LONG n;

  for(n=1; n<=blocks_bitmap; n++) {
    read2(block);
    if(checkbitmapblock((struct fsBitmap *)buffer, block, n)==FALSE) {
      printf("...error in bitmap block at block %ld\n",(long)block);
      return(FALSE);
    }
    block++;
  }

  return(TRUE);
}



BOOL checkrootblock(struct fsRootBlock *b, ULONG block) {
  if(isblockvalid(b,block,L2BE(DOSTYPE_ID))!=FALSE) {
    if(b->be_version==W2BE(STRUCTURE_VERSION)) {
      if(b->pad1==0 && b->be_pad2==0 && iszero(b->be_reserved1,8) && iszero(b->be_reserved2,8) && iszero(b->be_reserved3,32) && iszero(b->be_reserved4,16) && iszero((UBYTE *)b+sizeof(struct fsRootBlock),globals->bytes_block-sizeof(struct fsRootBlock))) {
        if(BE2L(b->be_totalblocks)==globals->blocks_total) {
          if(BE2L(b->be_blocksize)==globals->bytes_block) {
            return(TRUE);
          }
          else {
            printf("RootBlock's blocksize field is %ld, while it should be %ld.\n",(long)BE2L(b->be_blocksize),(long)globals->bytes_block);
          }
        }
        else {
          printf("RootBlock's total number of blocks is %ld, while it should be %ld.\n",(long)BE2L(b->be_totalblocks),(long)globals->blocks_total);
        }
      }
      else {
        printf("Reserved areas in RootBlock weren't all set to zero.\n");
      }
    }
    else {
      printf("RootBlock's version is unsupported by this version of SFScheck\n");
    }
  }

  return(FALSE);
}



BOOL isblockvalid(void *bh, ULONG block, ULONG id) {
  struct fsBlockHeader *b=(struct fsBlockHeader *)bh;

  if(checkchecksum((UBYTE *)b)!=FALSE) {
    if(b->id==id) {
      if(BE2L(b->be_ownblock)==block) {
        return(TRUE);
      }
      else {
        printf("Location of block is invalid.\n");
      }
    }
    else {
      printf("Incorrect block type at block %ld.  Expected was 0x%08lx but it was 0x%08lx.\n",(long)block,(unsigned long)BE2L(id),(unsigned long)BE2L(b->id));
    }
  }
  else {
    printf("Checksum failure.\n");
  }

  return(FALSE);
}



BOOL iszero(void *a,LONG size) {
  UBYTE *adr=(UBYTE *)a;

  while(--size>=0 && *adr++==0) {
  }

  if(size>=0) {
    return(FALSE);
  }
  return(TRUE);
}



struct fsObject *nextobject(struct fsObject *o) {
  UBYTE *p;

  /* skips the passed in fsObject and gives a pointer back to the place where
     a next fsObject structure could be located */

  p=(UBYTE *)&o->name[0];

  /* skip the filename */
  while(*p++!=0) {
  }

  /* skip the comment */
  while(*p++!=0) {
  }

  /* ensure WORD boundary */
  if((((ULONG)p) & 0x01)!=0) {
    p++;
  }

  return((struct fsObject *)p);
}



WORD isobject(struct fsObject *o, struct fsObjectContainer *oc) {
  UBYTE *endadr;

  endadr=(UBYTE *)oc+globals->bytes_block-sizeof(struct fsObject)-2;

  if((UBYTE *)o<endadr && o->name[0]!=0) {
    return(TRUE);
  }
  return(FALSE);
}



BOOL checkobjectcontainerfornode(ULONG block, ULONG node) {
  struct fsObjectContainer *oc=(struct fsObjectContainer *)buffer;

  read2(block);

  if(isblockvalid(oc,block,OBJECTCONTAINER_ID)!=FALSE) {
    struct fsObject *o=oc->object;

    while(isobject(o,oc)!=FALSE) {
      if(BE2L(o->be_objectnode)==node) {
        return(TRUE);
      }

      o=nextobject(o);
    }

    printf("ObjectContainer at block %ld doesn't contain node %ld\n",(long)block,(long)node);
    return(FALSE);
  }

  printf("ObjectContainer at block %ld is invalid and doesn't contain node %ld\n",(long)block,(long)node);
  return(FALSE);
}



BOOL checknodecontainers2(ULONG block, ULONG parent, ULONG nodenumber, ULONG nodes) {
  struct fsNodeContainer *b;

//  printf("Checking NodeContainer at block %ld, with parent %ld, nodenumber %ld and nodes %ld\n",block,parent,nodenumber,nodes);

  if((b=AllocVec(globals->bytes_block, globals->bufmemtype))!=0) {
    read2(block);
    CopyMemQuick(buffer,b,globals->bytes_block);

    if(isblockvalid(b,block,NODECONTAINER_ID)!=FALSE) {
      if(BE2L(b->be_nodenumber)==nodenumber) {
        if(nodes==0) {
          nodes=BE2L(b->be_nodes);
        }

        if(BE2L(b->be_nodes)==nodes) {
          if(nodes!=1) {
            WORD maxnodes=(globals->bytes_block-sizeof(struct fsNodeContainer))/4;
            LONG nodes2=nodes/maxnodes;
            WORD n;

            if(nodes2==0) {
              nodes2=1;
            }

            for(n=0; n<maxnodes; n++) {
              if(BE2L(b->be_node[n])!=0) {
                if(checknodecontainers2(BE2L(b->be_node[n])>>shifts_block32, block, nodenumber, nodes2)==FALSE) {
                  FreeVec(b);
                  return(FALSE);
                }
              }
              nodenumber+=nodes;
            }

            FreeVec(b);
            return(TRUE);
          }
          else {
            WORD maxnodes=(globals->bytes_block-sizeof(struct fsNodeContainer))/sizeof(struct fsObjectNode);
            WORD n;
            struct fsObjectNode *on=(struct fsObjectNode *)b->be_node;

            for(n=0; n<maxnodes; n++) {
              if(BE2L(on->node.be_data)!=0 && BE2L(on->node.be_data)!=-1) {
                if(checkobjectcontainerfornode(BE2L(on->node.be_data),nodenumber+n)==FALSE) {
                  FreeVec(b);
                  return(FALSE);
                }
              }

           /*
              else if(BE2L(on->node.be_data)==0 && (on->next!=0 || on->hash16!=0)) {
                printf("NodeContainer at block %ld has a not fully cleared node.\n",block);
                FreeVec(b);
                return(FALSE);
              }
           */

              on++;
            }

            FreeVec(b);
            return(TRUE);
          }
        }
        else {
          printf("NodeContainer at block %ld has nodes %ld while it should be %ld.\n",(long)block,(long)BE2L(b->be_nodes),(long)nodes);
        }
      }
      else {
        printf("NodeContainer at block %ld has nodenumber %ld while it should be %ld.\n",(long)block,(long)BE2L(b->be_nodenumber),(long)nodenumber);
      }
    }

    FreeVec(b);
  }
  else {
    printf("ERROR: Out of memory!\n");
  }

  return(FALSE);
}


BOOL checknodecontainers(ULONG block) {
  return(checknodecontainers2(block,0,1,0));
}



UBYTE *mark(LONG offset, LONG blocks) {
  LONG result;

  result=bmffo(bitmap, (globals->blocks_total+31)>>5, offset);

  if(result!=((globals->blocks_total+31)&(~0x1f)) && result < offset+blocks) {
    return(tostring("Block at offset %ld is already in use while marking %ld blocks from %ld.\n",result,blocks,offset));
  }

  if((result=bmset(bitmap, (globals->blocks_total+31)>>5, offset, blocks))!=blocks) {
    return(tostring("Error while marking %ld blocks from %ld.  Could only mark %ld blocks.\n",blocks,offset,result));
  }

  return(0);
}



LONG findnode(BLCK nodeindex,UWORD nodesize,NODE nodeno,struct fsNode **returned_node) {
  LONG errorcode;

  /* Finds a specific node by number.  It returns the cachebuffer which contains the fsNode
     structure and a pointer to the fsNode structure directly. */

  while((errorcode=read2(nodeindex))==0) {
    struct fsNodeContainer *nc=(struct fsNodeContainer *)buffer;

    if(BE2L(nc->be_nodes)==1) {
      /* We've descended the tree to a leaf NodeContainer */

      *returned_node=(struct fsNode *)((UBYTE *)nc->be_node+nodesize*(nodeno-BE2L(nc->be_nodenumber)));

      return(0);
    }
    else {
      UWORD containerentry=(nodeno-BE2L(nc->be_nodenumber))/BE2L(nc->be_nodes);

      nodeindex=BE2L(nc->be_node[containerentry])>>shifts_block32;
    }
  }

  return(errorcode);
}



BOOL checkobjectcontainers2(ULONG block, ULONG previous, ULONG parent) {
  struct fsObjectContainer *oc;
  UBYTE *str;

  if((oc=AllocVec(globals->bytes_block, globals->bufmemtype))!=0) {
    do {
      read2(block);
      CopyMemQuick(buffer,oc,globals->bytes_block);

      if(isblockvalid(oc,block,OBJECTCONTAINER_ID)!=FALSE) {
        if(BE2L(oc->be_previous)==previous) {
          if(BE2L(oc->be_parent)==parent) {
            struct fsObject *o=oc->object;
            struct fsObjectNode *node;
            LONG errorcode;

            while(isobject(o,oc)!=FALSE) {

              if((errorcode=findnode(block_objectnodesbase, sizeof(struct fsObjectNode), BE2L(o->be_objectnode), (struct fsNode **)&node))==0) {
                if(BE2L(node->node.be_data)==block) {
                  if((o->bits & OTYPE_LINK)==0) {
                    if((o->bits & OTYPE_DIR)!=0 && BE2L(o->object.dir.be_firstdirblock)!=0) {
                      if(checkobjectcontainers2(BE2L(o->object.dir.be_firstdirblock), 0, BE2L(o->be_objectnode))==FALSE) {
                        break;
                      }
                    }
                    else if((o->bits & OTYPE_DIR)==0) {
                      struct fsExtentBNode *ebn;
                      ULONG next=BE2L(o->object.file.be_data);
                      ULONG prev=0;
                      LONG errorcode;

                      while(next!=0) {
                        if((errorcode=findbnode(block_extentbnoderoot, next, (struct BNode **)&ebn))!=0) {
                          if(error("Errorcode %ld while locating BNode %ld of Object '%s' in ObjectContainer at block %ld.\n",errorcode,BE2L(o->object.file.be_data),o->name,block)) {
                            break;
                          }
                        }

                        if(BE2L(ebn->be_key)!=next) {
                          if(error("Error in Object '%s' in ObjectContainer at block %ld:\nBNode %ld has incorrect key.  It is %ld while it should be %ld.\n",o->name,block,next,BE2L(ebn->be_key),next)) {
                            break;
                          }
                        }

                        if((prev!=0 && BE2L(ebn->be_prev)!=prev) || (prev==0 && BE2L(ebn->be_prev)!=(BE2L(o->be_objectnode) | 0x80000000))) {
                          if(error("Error in Object '%s' in ObjectContainer at block %ld:\nBNode %ld has incorrect previous.  It is 0x%08lx.\n",o->name,block,next,BE2L(ebn->be_prev))) {
                            break;
                          }
                        }

                        if(BE2W(ebn->be_blocks)==0) {
                          if(error("Error in Object '%s' in ObjectContainer at block %ld:\nBNode %ld has a zero block count!\n",o->name,block,next)) {
                            break;
                          }
                        }

                        if((str=mark(BE2L(ebn->be_key), BE2W(ebn->be_blocks)))!=0) {
                          if(error("Error in Object '%s' in ObjectContainer at block %ld:\nBNode %ld points to space already in use:\n  %s",o->name,block,next,str)) {
                            freestring(str);
                            break;
                          }
                          freestring(str);
                        }

                        prev=next;
                        next=BE2L(ebn->be_next);
                      }

                      if(next!=0) {
                        break;
                      }
                    }
                  }
                }
                else {
                  printf("Node %ld of Object in ObjectContainer at block %ld points to wrong block (%ld)\n",(long)BE2L(o->be_objectnode),(long)block,(long)BE2L(node->node.be_data));
                  break;
                }
              }
              else {
                printf("Error %ld occured while locating Node %ld of Object in ObjectContainer at block %ld.\n",(long)errorcode,(long)BE2L(o->be_objectnode),(long)block);
                break;
              }

              o=nextobject(o);
            }

            if(isobject(o,oc)!=FALSE) {
              break;
            }
          }
          else {
            printf("ObjectContainer at block %ld has parent %ld while it should be %ld.\n",(long)block,(long)BE2L(oc->be_parent),(long)parent);
            break;
          }
        }
        else {
          printf("ObjectContainer at block %ld has previous %ld while it should be %ld.\n",(long)block,(long)BE2L(oc->be_previous),(long)previous);
          break;
        }
      }
      else {
        break;
      }

      previous=block;
      block=BE2L(oc->be_next);
    } while(block!=0);

    FreeVec(oc);

    if(block==0) {
      return(TRUE);
    }
  }
  else {
    printf("ERROR: Out of memory!\n");
  }

  return(FALSE);
}



BOOL checkobjectcontainers(ULONG block) {
  return(checkobjectcontainers2(block,0,0));
}



void dumpblock(ULONG block, void *data, ULONG bytes) {
  ULONG *d=(ULONG *)data;
  UBYTE *d2=(UBYTE *)data;
  UWORD off=0;
  UBYTE s[40];

  if(bytes<globals->bytes_block) {
    printf("Dump of first %ld bytes of block %ld.\n",(long)bytes,(long)block);
  }
  else {
    printf("Dump of block %ld.\n",(long)block);
  }

  while(bytes>0) {
    WORD n;
    UBYTE c;
    UBYTE *s2;

    n=16;
    s2=s;

    while(--n>=0) {
      c=*d2++;

      if(c<32) {
        c+=64;
      }
      if(c>=127 && c<=160) {
        c='.';
      }

      *s2++=c;
    }
    *s2=0;

    printf("0x%04lx: %08lx %08lx %08lx %08lx %s\n",(unsigned long)off,(unsigned long)BE2L(d[0]),(unsigned long)BE2L(d[1]),(unsigned long)BE2L(d[2]),(unsigned long)BE2L(d[3]),s);

    bytes-=16;
    d+=4;
    off+=16;
  }
}



BOOL checkadminspacecontainers(ULONG block) {
  struct fsAdminSpaceContainer *asc;

  if((asc=AllocVec(globals->bytes_block, globals->bufmemtype))!=0) {
    ULONG previous=0;

    while(block!=0) {
      read2(block);
      CopyMemQuick(buffer,asc,globals->bytes_block);

      if(isblockvalid(asc,block,ADMINSPACECONTAINER_ID)!=FALSE) {
        if(BE2L(asc->be_previous)==previous) {
//          if(asc->bits==32) {
            struct fsAdminSpace *as=asc->adminspace;

            while((UBYTE *)as<((UBYTE *)asc+globals->bytes_block) && BE2L(as->be_space)!=0) {
              ULONG adminblock=BE2L(as->be_space);
              LONG bits=BE2L(as->be_bits);
              WORD n=32;
              BYTE valid;
              UBYTE *str;

              if((str=mark(adminblock,32))!=0) {
                if(error("AdminSpaceContainer at %ld occupies already used space:\n  %s",block,str)) {
                  freestring(str);
                  break;
                }
                freestring(str);
              }

              while(--n>=0) {
                if(bits<0) {
                  struct fsBlockHeader *bh=(struct fsBlockHeader *)buffer;

                  read2(adminblock);

                  valid=FALSE;
                  if(checkchecksum((UBYTE *)bh)!=FALSE) {
                    if(bh->id==ADMINSPACECONTAINER_ID || bh->id==OBJECTCONTAINER_ID || bh->id==HASHTABLE_ID || bh->id==NODECONTAINER_ID || bh->id==BNODECONTAINER_ID || bh->id==TRANSACTIONOK_ID || bh->id==SOFTLINK_ID) {
                      if(BE2L(bh->be_ownblock)==adminblock) {
                        valid=TRUE;
                      }
                    }
                  }

                  if(valid==FALSE) {
                    printf("Block %ld is not a valid admin block but it is marked in use in AdminSpaceContainer at %ld\n",(long)adminblock,(long)block);
                    dumpblock(adminblock,buffer,64);
                    break;
                  }
                }
                bits<<=1;
                adminblock++;
              }

              if(n>=0) {
                break;
              }

              as++;
            }

            if((UBYTE *)as<((UBYTE *)asc+globals->bytes_block) && BE2L(as->be_space)!=0) {
              break;
            }

//          }
//          else {
//            printf("AdminSpaceContainer at block %ld hasn't got 32 blocks/entry (%ld)!\n",block,asc->bits);
//            break;
//          }
        }
        else {
          printf("AdminSpaceContainer at block %ld has previous %ld while it should be %ld.\n",(long)block,(long)BE2L(asc->be_previous),(long)previous);
          break;
        }
      }
      else {
        break;
      }

      previous=block;
      block=BE2L(asc->be_next);
    }

    FreeVec(asc);

    if(block==0) {
      return(TRUE);
    }
  }
  else {
    printf("ERROR: Out of memory!\n");
  }

  return(FALSE);
}



void sfscheck(void) {
  ULONG block_bitmapbase;
  ULONG block_adminspacecontainer;

  printf("\n");
  printf("Checking RootBlocks\n");

  read2(globals->blocks_total-1);

  if((checkrootblock((struct fsRootBlock *)buffer,globals->blocks_total-1))!=FALSE) {

    read2(0);

    if((checkrootblock((struct fsRootBlock *)buffer,0))!=FALSE) {
      struct fsRootBlock *b=(struct fsRootBlock *)buffer;
      UBYTE *str;

      printf("...okay\n");

      block_bitmapbase=BE2L(b->be_bitmapbase);
      block_root=BE2L(b->be_rootobjectcontainer);
      block_extentbnoderoot=BE2L(b->be_extentbnoderoot);
      block_adminspacecontainer=BE2L(b->be_adminspacecontainer);
      block_objectnodesbase=BE2L(b->be_objectnoderoot);

      if((str=mark(block_bitmapbase,blocks_bitmap))!=0) {
        Printf("Error while marking bitmap space:\n  %s",str);
        freestring(str);
      }

      printf("Checking AdminSpaceContainers at block %ld\n",(long)block_adminspacecontainer);
      if((checkadminspacecontainers(block_adminspacecontainer))!=FALSE) {
        printf("...okay\n");

        printf("Checking NodeContainers at block %ld\n",(long)block_objectnodesbase);
        if((checknodecontainers(block_objectnodesbase))!=FALSE) {
          printf("...okay\n");

          printf("Checking ObjectContainers at block %ld\n",(long)block_root);
          if((checkobjectcontainers(block_root))!=FALSE) {
            printf("...okay\n");

            printf("Checking Bitmap at block %ld (%ld blocks, %ld bits/bitmap)\n",(long)block_bitmapbase,(long)blocks_bitmap,(long)blocks_inbitmap);
            if((checkbitmap(block_bitmapbase))!=FALSE) {
              printf("...okay\n");

            }
            else {
              returncode=20;
              printf("...damaged\n");
            }
          }
          else {
            returncode=20;
            printf("...damaged\n");
          }
        }
        else {
          returncode=20;
          printf("...damaged\n");
        }
      }
      else {
        returncode=20;
        printf("...damaged\n");
      }
    }
    else {
      returncode=20;
      printf("...damaged\n");
    }
  }
  else {
    returncode=20;
    printf("...damaged\n");
  }


  if(errors!=0) {
    returncode=20;
  }
}



struct BNode *searchforbnode(ULONG key,struct BTreeContainer *tc) {
  struct BNode *tn;
  WORD n=BE2W(tc->be_nodecount)-1;

  tn=(struct BNode *)((UBYTE *)tc->bnode+n*tc->nodesize);

  for(;;) {
    if(n<=0 || key >= BE2L(tn->be_key)) {
      return(tn);
    }
    tn=(struct BNode *)((UBYTE *)tn-tc->nodesize);
    n--;
  }
}



LONG findbnode(BLCK rootblock,ULONG key,struct BNode **returned_bnode) {
  LONG errorcode;

  while((errorcode=read2(rootblock))==0) {
    struct fsBNodeContainer *bnc=(struct fsBNodeContainer *)buffer;
    struct BTreeContainer *btc=&bnc->btc;

    *returned_bnode=searchforbnode(key,btc);
    if(btc->isleaf==TRUE) {
      break;
    }
    rootblock=BE2L((*returned_bnode)->be_data);
  }

  return(errorcode);
}



void freestring(UBYTE *str) {
  if(str!=emptystring) {
    FreeVec(str);
  }
}



UBYTE *tostring(UBYTE *fmt, ... ) {
  UBYTE *buf;

  if((buf=AllocVec(200,MEMF_CLEAR))!=0) {
    ULONG *args;

    args=(ULONG *)&fmt;
    args++;

    RawDoFmt(fmt,args,(void (*)())"\x16\xC0\x4E\x75",buf);

    return(buf);
  }
  else {
    return(emptystring);
  }
}



BOOL error(UBYTE *fmt, ... ) {

  VPrintf(fmt,((IPTR *)&fmt)+1);

  errors++;

  if(errors>maxerrors) {
    return(TRUE);
  }

  return(FALSE);
}


#if 0
/* cachedio.o already implements this function, so the
   dummy function below is not needed. */

LONG getbuffer(UBYTE **tempbuffer, ULONG *maxblocks) {

  /* Used by deviceio.o to get a piece of memory which it can
     use for buffering transfers when the Mask prevents a
     direct transfer.

     You must return 0, a pointer to the buffer and the number
     of blocks (each /bytes_block/ bytes in size) you allocated.
     For the purpose of SFScheck this function is probably never
     called since all our buffers are atleast LONG aligned and
     allocated with the correct bufmemtype. */

  return(ERROR_NO_FREE_STORE);
}

#endif


void starttimeout(void) {
  /* Called by deviceio.o each time there is a physical
     disk access.  You can use this to start a timer and
     call motoroff() when the disk hasn't been accessed
     for a specific amount of time (SFS uses 1 second).

     SFScheck doesn't use this function. */
}
