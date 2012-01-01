#include "asmsupport.h"

#include <exec/types.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/utility.h>

#include <utility/tagitem.h>

#include "objects.h"
#include "objects_protos.h"

#include "adminspaces_protos.h"
#include "cachebuffers_protos.h"
#include "debug.h"
#include "locks_protos.h"
#include "nodes_protos.h"
#include "support_protos.h"
#include "transactions_protos.h"
#include "req_protos.h"
#include "globals.h"

#include <string.h>

extern UBYTE *fullpath(struct CacheBuffer *cbstart,struct fsObject *o);
extern LONG readcachebuffercheck(struct CacheBuffer **,ULONG,ULONG);

extern BOOL freeupspace(void);

extern LONG deleteextents(ULONG key);

extern void checknotifyforobject(struct CacheBuffer *cb,struct fsObject *o,UBYTE notifyparent);
extern void checknotifyforpath(UBYTE *path,UBYTE notifyparent);

extern LONG deletefileslowly(struct CacheBuffer *cbobject, struct fsObject *o);

static LONG dehashobjectquick(NODE objectnode, UBYTE *name, NODE parentobjectnode);
LONG simpleremoveobject(struct CacheBuffer *cb,struct fsObject *o);
LONG locatelockableparent(struct ExtFileLock *lock,UBYTE *path,struct CacheBuffer **returned_cb,struct fsObject **returned_o);
#ifdef DEBUGCODE
static void copyobject(struct fsObject *o2,struct fsObject *o);
#endif
LONG findobjectspace(struct CacheBuffer **io_cb,struct fsObject **io_o,ULONG bytesneeded);
static UBYTE *emptyspaceinobjectcontainer(struct fsObjectContainer *oc);
WORD objectsize(struct fsObject *o);
UBYTE *getcomment(struct fsObject *o);
static WORD changeobjectsize(struct CacheBuffer *cb, struct fsObject *o, WORD bytes);
static LONG deleteobjectnode(NODE objectnode);



LONG hashobject(BLCK hashblock, struct fsObjectNode *on, NODE nodeno, UBYTE *objectname) {
  struct CacheBuffer *cbhash;
  LONG errorcode=0;

  /* This function takes a HashBlock pointer, an ObjectNode and an ObjectName.
     If there is a hashblock, then this function will correctly link the object
     into the hashchain.  If there isn't a hashblock (=0) then this function
     does nothing.

     A transaction must be in progress before calling this function, and the
     ObjectNode's CacheBuffer must be prepared. */

  if(hashblock!=0 && (errorcode=readcachebuffercheck(&cbhash, hashblock, HASHTABLE_ID))==0) {
    struct fsHashTable *ht=cbhash->data;
    NODE nexthash;
    UWORD hashvalue, hashchain;

    hashvalue=hash(objectname, globals->is_casesensitive);
    hashchain=HASHCHAIN(hashvalue);
    nexthash=BE2L(ht->be_hashentry[hashchain]);

    preparecachebuffer(cbhash);

    checksum_writelong_be(cbhash->data, &ht->be_hashentry[hashchain], nodeno);

    // ht->hashentry[hashchain]=nodeno;

    if((errorcode=storecachebuffer_nochecksum(cbhash))==0) {
      on->be_next=L2BE(nexthash);
      on->be_hash16=W2BE(hashvalue);
    }
  }

  return(errorcode);
}



LONG setrecycledinfo(ULONG deletedfiles, ULONG deletedblocks) {
  struct CacheBuffer *cb;
  LONG errorcode;

  if((errorcode=readcachebuffercheck(&cb,globals->block_root,OBJECTCONTAINER_ID))==0) {
    struct fsRootInfo *ri=(struct fsRootInfo *)((UBYTE *)cb->data+globals->bytes_block-sizeof(struct fsRootInfo));

    preparecachebuffer(cb);

    checksum_writelong_be(cb->data, &ri->be_deletedfiles, deletedfiles);
    checksum_writelong_be(cb->data, &ri->be_deletedblocks, deletedblocks);

    // ri->deletedfiles=deletedfiles;
    // ri->deletedblocks=deletedblocks;

    errorcode=storecachebuffer_nochecksum(cb);
  }

  return(errorcode);
}



LONG setrecycledinfodiff(LONG deletedfiles, LONG deletedblocks) {
  struct CacheBuffer *cb;
  LONG errorcode;

  if((errorcode=readcachebuffercheck(&cb,globals->block_root,OBJECTCONTAINER_ID))==0) {
    struct fsRootInfo *ri=(struct fsRootInfo *)((UBYTE *)cb->data+globals->bytes_block-sizeof(struct fsRootInfo));

    preparecachebuffer(cb);

    checksum_writelong_be(cb->data, &ri->be_deletedfiles, BE2L(ri->be_deletedfiles) + deletedfiles);
    checksum_writelong_be(cb->data, &ri->be_deletedblocks, BE2L(ri->be_deletedblocks) + deletedblocks);

    // ri->deletedfiles+=deletedfiles;
    // ri->deletedblocks+=deletedblocks;

    errorcode=storecachebuffer_nochecksum(cb);
  }

  return(errorcode);
}



LONG getrecycledinfo(ULONG *returned_deletedfiles, ULONG *returned_deletedblocks) {
  struct CacheBuffer *cb;
  LONG errorcode;

  if((errorcode=readcachebuffercheck(&cb,globals->block_root,OBJECTCONTAINER_ID))==0) {
    struct fsRootInfo *ri=(struct fsRootInfo *)((UBYTE *)cb->data+globals->bytes_block-sizeof(struct fsRootInfo));

    *returned_deletedfiles=BE2L(ri->be_deletedfiles);
    *returned_deletedblocks=BE2L(ri->be_deletedblocks);
  }

  return(errorcode);
}



static LONG safedeleteobjectquick(struct CacheBuffer *cb, struct fsObject *o, WORD sendnotify) {
  struct CacheBuffer *cbdd;
  struct fsObject *odd;
  UBYTE *s=globals->string2;
  UBYTE *n=o->name;
  WORD len=0;
  LONG errorcode;

  while((*s++=*n++)!=0) {
    len++;
  }

  lockcachebuffer(cb);
  len-=(globals->max_name_length-4);

  if(len>0) {
    s-=len;
  }
  s--;

  if((errorcode=readobject(RECYCLEDNODE, &cbdd, &odd))==0) {
    struct CacheBuffer *cb2;
    struct fsObject *o2;
    UWORD count=0;

    lockcachebuffer(cbdd);

    for(;;) {
      cb2=cbdd;
      o2=odd;

      if((errorcode=locateobject(globals->string2, &cb2, &o2))==ERROR_OBJECT_NOT_FOUND) {
        ULONG blocks=(BE2L(o->object.file.be_size)+globals->bytes_block-1)>>globals->shifts_block;

        globals->internalrename=TRUE;

        if((errorcode=renameobject2(cb, o, cbdd, odd, globals->string2, sendnotify))==0) {
          errorcode=setrecycledinfodiff(1, blocks);
        }

        globals->internalrename=FALSE;
        break;
      }
      else {
        UBYTE *s2=s;

        *s2++='$';
        *s2++=((count & 0xF00)>>8)+65;
        *s2++=((count & 0x0F0)>>4)+65;
        *s2++=(count & 0x00F)+65;
        *s2=0;

        if(count++>0xFFF) {
          errorcode=INTERR_SAFE_DELETE;
          break;
        }
      }
    }

    unlockcachebuffer(cbdd);
  }

  unlockcachebuffer(cb);

  _XDEBUG((DEBUG_OBJECTS,"safedeleteobjectquick: exiting with errorcode %ld\n",errorcode));

  return(errorcode);
}




BOOL cleanupdeletedfiles(void) {
  /* This function returns TRUE if atleast one or more files were succesfully
     deleted.  FALSE is returned if no space at all could be freed.

     This function returns FALSE if there's no recycled directory. */

  _DEBUG(("cleanupdeletedfiles: entry\n"));

  if(globals->has_recycled!=FALSE) {
    struct CacheBuffer *cbdd;
    struct fsObject *odd;
    ULONG kbdeleted=0;
    ULONG filesdeleted=0;
    UBYTE filedeleted;

    if(readobject(RECYCLEDNODE, &cbdd, &odd)==0 && odd->object.dir.be_firstdirblock!=0) {

      lockcachebuffer(cbdd);

      while(kbdeleted<1024 && filesdeleted<10) {
        struct CacheBuffer *cb=0;
        struct fsObjectContainer *oc;
        struct fsObject *o;
        BLCK nextblock=BE2L(odd->object.dir.be_firstdirblock);

        filedeleted=FALSE;

        while(nextblock!=0 && readcachebuffercheck(&cb, nextblock, OBJECTCONTAINER_ID)==0) {
          oc=cb->data;
          nextblock=BE2L(oc->be_next);
        }

        if(nextblock!=0) {
          break;
        }

        do {
          oc=cb->data;
          o=oc->object;

          do {
            if(lockable(BE2L(o->be_objectnode),EXCLUSIVE_LOCK)!=DOSFALSE) {
              ULONG size=BE2L(o->object.file.be_size)>>10;

              _DEBUG(("cleanupdeletedfiles: deleting %s, objectnode = %ld\n",o->name,BE2L(o->be_objectnode)));

              if(deletefileslowly(cb, o)==0) {
                kbdeleted+=size;
                filesdeleted++;
                filedeleted=TRUE;
                break;
              }
            }

            o=nextobject(o);
          } while(isobject(o,oc)!=FALSE);

        } while(filedeleted==FALSE && oc->be_previous!=0 && readcachebuffercheck(&cb, BE2L(oc->be_previous), OBJECTCONTAINER_ID)==0);

        if(oc->be_previous==0) {
          break;
        }
      }

      unlockcachebuffer(cbdd);
    }

    _DEBUG(("cleanupdeletedfiles: Deleted %ld files for %ld kB worth of space\n",filesdeleted,kbdeleted));

    if(filesdeleted!=0) {
      return(TRUE);
    }
  }

  return(FALSE);
}



static LONG deleteobjectquick(struct CacheBuffer *cb, struct fsObject *o, WORD sendnotify) {
  UWORD bits=o->bits;
  BLCK hashblckno=BE2L(o->object.dir.be_hashtable);
  BLCK extentbnode=BE2L(o->object.file.be_data);
  UBYTE *notifypath=0;
  LONG errorcode;

  /* cb & o refer to the object to be deleted.

     Note: This function deletes an object without first checking if
           this is allowed.  Use deleteobject() instead. */

  if(sendnotify!=FALSE) {
    notifypath=fullpath(cb,o);
  }

  if((errorcode=removeobject(cb, o))==0) {

    /* What we need to do now depends on the type of object we are
       deleting.  For a directory we need to kill its HashTable block,
       and for a File we need to remove all of its Extents and the
       used space associated with them. */

    if((bits & OTYPE_LINK)!=0) {
      _XDEBUG((DEBUG_OBJECTS,"deleteobject: Object is soft link!\n"));

      errorcode=freeadminspace(extentbnode);
    }
    else if((bits & OTYPE_DIR)!=0) {
      _XDEBUG((DEBUG_OBJECTS,"deleteobject: Object is a directory!\n"));

      errorcode=freeadminspace(hashblckno);
    }
    else {
      _XDEBUG((DEBUG_OBJECTS,"deleteobject: Object is a file\n"));

      errorcode=deleteextents(extentbnode);
    }

    if(errorcode==0 && sendnotify!=FALSE) {
      checknotifyforpath(notifypath,TRUE);
    }
  }

  return(errorcode);
}



LONG deleteobject(struct ExtFileLock *lock, UBYTE *path, WORD sendnotify) {
  struct CacheBuffer *cb;
  struct fsObject *o;
  NODE objectnode;
  LONG errorcode;

  /* This function deletes the specified object.  It will only delete directories
     if they are empty.  All space associated with a file will be marked free. */

  _XDEBUG((DEBUG_OBJECTS,"deleteobject: Entry -- deleting object %s\n",path));

  if(lock==0) {
    objectnode=ROOTNODE;
  }
  else {
    objectnode=lock->objectnode;
  }

  if((errorcode=readobject(objectnode,&cb,&o))==0) {
    errorcode=locateobject2(&path,&cb,&o);

    while(*path!=0) {
      if(*path=='/') {
        break;
      }
      path++;
    }

    if(errorcode==0 || (errorcode==ERROR_IS_SOFT_LINK && *path==0)) {
      if((o->be_protection & L2BE(FIBF_DELETE))!=0) {
        if(lockable(BE2L(o->be_objectnode),EXCLUSIVE_LOCK)!=DOSFALSE && (o->bits & OTYPE_UNDELETABLE)==0) {
          /* Object to be deleted was read correctly by the above locateobject call */

          if((o->bits & OTYPE_DIR)==0 || o->object.dir.be_firstdirblock==0) {
            struct fsObjectContainer *oc=cb->data;

            /* At this point we can be sure that the object is allowed
               to be deleted. */

            if((o->bits & OTYPE_DIR)!=0 || (o->bits & OTYPE_LINK)!=0 || globals->has_recycled==FALSE || oc->be_parent==L2BE(RECYCLEDNODE)) {
              errorcode=deleteobjectquick(cb, o, sendnotify);
            }
            else {
              errorcode=safedeleteobjectquick(cb, o, sendnotify);
            }
          }
          else {
            errorcode=ERROR_DIRECTORY_NOT_EMPTY;
          }
        }
        else {
          errorcode=ERROR_OBJECT_IN_USE;
        }
      }
      else {
        errorcode=ERROR_DELETE_PROTECTED;
      }
    }
  }

  _XDEBUG((DEBUG_OBJECTS,"deleteobject: Exiting with errorcode %ld\n",errorcode));

  return(errorcode);
}



LONG removeobjectcontainer(struct CacheBuffer *cb) {
  struct fsObjectContainer *oc=cb->data;
  LONG errorcode;

  /* Removes an ObjectContainer from a directory chain.  Make
     sure it is empty before removing it! */

  _XDEBUG((DEBUG_OBJECTS,"removeobjectcontainer: entry\n"));

  lockcachebuffer(cb);

  if(oc->be_next!=0 && oc->be_next!=oc->bheader.be_ownblock) {  // BE-BE compare!
    struct CacheBuffer *next_cb;
    struct fsObjectContainer *next_oc;

    if((errorcode=readcachebuffercheck(&next_cb,BE2L(oc->be_next),OBJECTCONTAINER_ID))!=0) {
      return(errorcode);
    }

    preparecachebuffer(next_cb);

    next_oc=next_cb->data;

    next_oc->be_previous=oc->be_previous;   // BE-BE copy!
    if((errorcode=storecachebuffer(next_cb))!=0) {
      return(errorcode);
    }
  }

  if(oc->be_previous!=0 && oc->be_previous!=oc->bheader.be_ownblock) { // BE-BE compare!
    struct CacheBuffer *previous_cb;
    struct fsObjectContainer *previous_oc;

    if((errorcode=readcachebuffercheck(&previous_cb,BE2L(oc->be_previous),OBJECTCONTAINER_ID))!=0) {
      return(errorcode);
    }

    preparecachebuffer(previous_cb);

    previous_oc=previous_cb->data;

    previous_oc->be_next=oc->be_next;   // BE-BE copy
    if((errorcode=storecachebuffer(previous_cb))!=0) {
      return(errorcode);
    }
  }
  else {
    struct CacheBuffer *parent_cb;
    struct fsObject *parent_o;

    if((errorcode=readobject(BE2L(oc->be_parent),&parent_cb,&parent_o))!=0) {
      return(errorcode);
    }

    preparecachebuffer(parent_cb);

    if((parent_o->bits & OTYPE_RINGLIST)!=0) {
      parent_o->object.dir.be_firstdirblock=0;
    }
    else {
      parent_o->object.dir.be_firstdirblock=oc->be_next;   // BE-BE copy
    }

    if((errorcode=storecachebuffer(parent_cb))!=0) {
      return(errorcode);
    }
  }

  unlockcachebuffer(cb);

  if((errorcode=freeadminspace(cb->blckno))!=0) {
    return(errorcode);
  }

  // emptycachebuffer(cb);

  return(0);
}



LONG bumpobject(struct CacheBuffer *cb, struct fsObject *o) {
  ULONG newdate=getdate();
  ULONG newprotection=BE2L(o->be_protection) & ~FIBF_ARCHIVE;

  /* Updates the date of the object and clears the archived bit. */

  if(newdate!=BE2L(o->be_datemodified) || BE2L(o->be_protection)!=newprotection) {
    preparecachebuffer(cb);

    checksum_writelong_be(cb->data, &o->be_datemodified, newdate);
    checksum_writelong_be(cb->data, &o->be_protection, newprotection);

    // o->datemodified=newdate;
    // o->protection=newprotection;

    return(storecachebuffer_nochecksum(cb));
  }

  return(0);
}


LONG simpleremoveobject(struct CacheBuffer *cb, struct fsObject *o) {
  struct fsObjectContainer *oc=cb->data;
  BLCK parent=BE2L(oc->be_parent);
  LONG errorcode;

  /* This function removes the fsObject structure passed in from the passed
     in CacheBuffer (a fsObjectContainer).  If the ObjectContainer becomes
     completely empty it will be delinked from the ObjectContainer chain and
     marked free for reuse.  newtransaction() must have been called before
     calling this function.

     This function doesn't delink the object from the hashchain! */

  _XDEBUG((DEBUG_OBJECTS,"simpleremoveobject: Entry\n"));

  if(BE2L(oc->be_parent)==RECYCLEDNODE) {

    /* This object is removed from the Recycled directory. */

    if((errorcode=setrecycledinfodiff(-1, -((BE2L(o->object.file.be_size)+globals->bytes_block-1)>>globals->shifts_block)))!=0) {
      return(errorcode);
    }
  }

  if(isobject(nextobject(oc->object),oc)==FALSE) {
    errorcode=removeobjectcontainer(cb);
  }
  else {
    struct fsObject *nexto;
    WORD words;
    UWORD *src;
    UWORD *dst;

    preparecachebuffer(cb);

    nexto=nextobject(o);
    src=(UWORD *)nexto;
    dst=(UWORD *)o;
    words=(globals->bytes_block-((UBYTE *)nexto-(UBYTE *)oc))>>1;

    while(words-->0) {
      *dst++=*src++;
    }

    words=((UBYTE *)nexto-(UBYTE *)o)>>1;

    while(words-->0) {
      *dst++=0;
    }

    errorcode=storecachebuffer(cb);
  }

  if(errorcode==0 && (errorcode=readobject(parent,&cb,&o))==0) {   // reading parent block
    errorcode=bumpobject(cb, o);   /* An object was removed from this directory -- update date. */
  }

  return(errorcode);
}



static LONG dehashobjectquick(NODE objectnode, UBYTE *name, NODE parentobjectnode) {
  struct CacheBuffer *cb;
  struct fsObject *o;
  LONG errorcode;

  _XDEBUG((DEBUG_OBJECTS,"dehashobject: Delinking object %ld (=ObjectNode) from hashchain.  Parentnode = %ld\n",objectnode,parentobjectnode));

  /* This function delinks the passed in ObjectNode from its hash-chain.  Handy when deleting
     the object, or when renaming/moving it.  newtransaction() must have been called before
     calling this function. */

  if((errorcode=readobject(parentobjectnode,&cb,&o))==0 && o->object.dir.be_hashtable!=0) {
    if((errorcode=readcachebuffercheck(&cb,BE2L(o->object.dir.be_hashtable),HASHTABLE_ID))==0) {
      struct CacheBuffer *cbnode;
      struct fsObjectNode *on;
      struct fsHashTable *ht=cb->data;
      NODE nexthash;

      lockcachebuffer(cb);

      if((errorcode=findnode(globals->block_objectnoderoot, sizeof(struct fsObjectNode), objectnode, &cbnode, (struct fsNode **)&on))==0) {
        UWORD hashchain;

        _XDEBUG((DEBUG_OBJECTS,"dehashobject: Read HashTable block of parent object of object to be delinked\n"));

        lockcachebuffer(cbnode);

        hashchain=HASHCHAIN(hash(name, globals->is_casesensitive));
        nexthash=BE2L(ht->be_hashentry[hashchain]);

        if(nexthash==objectnode) {
          /* The hashtable directly points to the fsObject to be delinked.  We simply
             modify the Hashtable to point to the new nexthash entry. */

          _XDEBUG((DEBUG_OBJECTS,"dehashobject: The hashtable points directly to the to be delinked object\n"));

          preparecachebuffer(cb);

          checksum_writelong_be(cb->data, &ht->be_hashentry[hashchain], BE2L(on->be_next));

          // ht->hashentry[hashchain]=on->next;

          errorcode=storecachebuffer_nochecksum(cb);
        }
        else {
          struct CacheBuffer *cb=0;
          struct fsObjectNode *onsearch=0;

          _XDEBUG((DEBUG_OBJECTS,"dehashobject: Walking through hashchain\n"));

          while(nexthash!=0 && nexthash!=objectnode) {
            if((errorcode=findnode(globals->block_objectnoderoot, sizeof(struct fsObjectNode), nexthash, &cb, (struct fsNode **)&onsearch))!=0) {
              break;
            }

            nexthash=BE2L(onsearch->be_next);
          }

          if(errorcode==0) {
            if(nexthash!=0) {
              /* Previous fsObjectNode found in hash chain.  Modify the fsObjectNode to 'skip' the
                 ObjectNode which is being delinked from the hash chain. */

              preparecachebuffer(cb);

              checksum_writelong_be(cb->data, &onsearch->be_next, BE2L(on->be_next));

              // onsearch->next=on->next;

              errorcode=storecachebuffer_nochecksum(cb);
            }
            else {
              req("Hashchain of object %ld is\ncorrupt or incorrectly linked.", "Ok", objectnode);

              /*** This is strange.  We have been looking for the fsObjectNode which is located before the
                   passed in fsObjectNode in the hash-chain.  However, we never found the
                   fsObjectNode reffered to in the hash-chain!  Has to be somekind
                   of internal error... */

              errorcode=ERROR_OBJECT_NOT_FOUND;
            }
          }
        }

        unlockcachebuffer(cbnode);
      }

      unlockcachebuffer(cb);
    }
  }

  return(errorcode);
}



static LONG createobjecttagitem(struct CacheBuffer **io_cb, struct fsObject **io_o, UBYTE *objectname, struct TagItem *tags) {
  struct CacheBuffer *cb=*io_cb;
  struct fsObject *o=*io_o;
  LONG errorcode;

  /* io_cb & io_o refer to the direct parent of the new object.  Objectname is the
     name of the new object (name only).  Use the Tags to pass a comment (important
     since you can't change the size of the Object afterwards to accomodate one).

     io_cb & io_o will not be locked, so make sure you lock the parent if you intend
     to use it after this function.

     This function updates the parent's date and Archive bit.  It also check if the
     object already exists to avoid duplicates.

     If this function returns no error it will return in io_cb & io_o the new object
     (prepared). */

  _XDEBUG((DEBUG_OBJECTS,"createobjecttags: Creating object '%s' in dir '%s'.\n",objectname,(*io_o)->name));

  if(BE2L((*io_o)->be_objectnode)!=RECYCLEDNODE || globals->internalrename!=FALSE) {
    struct TagItem *tag;

    lockcachebuffer(*io_cb);

    /* Either duplicates are allowed or the object doesn't exist yet: */

    if(((tag=FindTagItem(CO_ALLOWDUPLICATES, tags))!=0 && tag->ti_Data!=FALSE) || (errorcode=locateobject(objectname, &cb, &o))==ERROR_OBJECT_NOT_FOUND) {

      unlockcachebuffer(*io_cb);

      _XDEBUG((DEBUG_OBJECTS,"createobjecttags: Object didn't exist, so we can safely create it.\n"));

      errorcode=0;

      if((tag=FindTagItem(CO_UPDATEPARENT, tags))!=0 && tag->ti_Data!=FALSE) {
        errorcode=bumpobject(*io_cb, *io_o);  /* Update the date and the ARCHIVE bit of the parent directory. */
      }

      if(errorcode==0) {
        ULONG objectsize;
        ULONG hashblock=BE2L((*io_o)->object.dir.be_hashtable);

        objectsize=sizeof(struct fsObject)+strlen(objectname)+2;

        if((tag=FindTagItem(CO_COMMENT, tags))!=0) {
          objectsize+=strlen((UBYTE *)tag->ti_Data);
        }

        if((errorcode=findobjectspace(io_cb, io_o, objectsize))==0) {
          struct TagItem *tstate=tags;
          struct fsObject *o=*io_o;
          UBYTE *name=o->name;
          UBYTE *objname=objectname;

          o->bits=0;

          if((tag=FindTagItem(CO_BITS, tags))!=0) {
            o->bits=tag->ti_Data;
          }

          /* Setting defaults in case tags aren't specified: */

          o->be_owneruid=0;
          o->be_ownergid=0;
          o->be_protection=L2BE(FIBF_READ|FIBF_WRITE|FIBF_EXECUTE|FIBF_DELETE);
          o->be_datemodified=L2BE(getdate());

          /* Copying name */

          while(*objname!=0) {
            *name++=*objname++;
          }
          *name++=0;
          *name=0;   /* zero byte for comment */

          while((tag=NextTagItem(&tstate))!=0) {
            switch(tag->ti_Tag) {
            case CO_OWNER:
              o->be_owneruid=W2BE(tag->ti_Data>>16);
              o->be_ownergid=W2BE(tag->ti_Data & 0xFFFF);
              break;
            case CO_DATEMODIFIED:
              o->be_datemodified=L2BE(tag->ti_Data);
              break;
            case CO_PROTECTION:
              o->be_protection=L2BE(tag->ti_Data);
              break;
            case CO_COMMENT:
              {
                UBYTE *comment=(UBYTE *)tag->ti_Data;

                while(*comment!=0) {
                  *name++=*comment++;
                }
                *name=0;
              }
              break;
            case CO_SIZE:
              if((o->bits & OTYPE_DIR)==0) {
                o->object.file.be_size=L2BE(tag->ti_Data);
              }
              break;
            case CO_DATA:
              if((o->bits & OTYPE_DIR)==0) {
                o->object.file.be_data=L2BE(tag->ti_Data);
              }
              break;
            case CO_FIRSTDIRBLOCK:
              if((o->bits & OTYPE_DIR)!=0) {
                o->object.dir.be_firstdirblock=L2BE(tag->ti_Data);
              }
              break;
            }
          }

          if(errorcode==0) {  // ObjectNode reuse or creation:
            struct CacheBuffer *cbnode;
            struct fsObjectNode *on;

            if((tag=FindTagItem(CO_OBJECTNODE, tags))!=0) {

              o->be_objectnode=L2BE(tag->ti_Data);

              if((errorcode=findnode(globals->block_objectnoderoot, sizeof(struct fsObjectNode), BE2L(o->be_objectnode), &cbnode, (struct fsNode **)&on))==0) {
                preparecachebuffer(cbnode);
              }
            }
            else {
              NODE nodeno;

              if((errorcode=createnode(globals->block_objectnoderoot, sizeof(struct fsObjectNode), &cbnode, (struct fsNode **)&on, &nodeno))==0) {
                on->be_hash16=W2BE(hash(o->name, globals->is_casesensitive));
                o->be_objectnode=L2BE(nodeno);
              }
            }

            if(errorcode==0) {
              /* CacheBuffer cbnode is prepared. */

              on->node.be_data=L2BE((*io_cb)->blckno);

              if((tag=FindTagItem(CO_HASHOBJECT, tags))!=0 && tag->ti_Data!=FALSE) {
                errorcode=hashobject(hashblock, on, BE2L(o->be_objectnode), objectname);
              }

              if(errorcode==0) {
                errorcode=storecachebuffer(cbnode);
              }
              else {
                dumpcachebuffer(cbnode);
              }
            }
          }

          if(errorcode==0) {  // HashBlock reuse or creation:
            if((o->bits & OTYPE_DIR)!=0) {
              struct CacheBuffer *hashcb;

              if((tag=FindTagItem(CO_HASHBLOCK, tags))!=0) {
                o->object.dir.be_hashtable=L2BE(tag->ti_Data);
              }
              else if((errorcode=allocadminspace(&hashcb))==0) {  // Create a new HashTable block.
                struct fsHashTable *ht=hashcb->data;

                o->object.dir.be_hashtable=L2BE(hashcb->blckno);

                ht->bheader.id=HASHTABLE_ID;
                ht->bheader.be_ownblock=L2BE(hashcb->blckno);
                ht->be_parent=o->be_objectnode; // BE-BE copy!

                errorcode=storecachebuffer(hashcb);
              }
            }
          }

          if(errorcode==0) {  // SoftLink creation:
            if((o->bits & (OTYPE_LINK|OTYPE_HARDLINK))==OTYPE_LINK) {
              if((tag=FindTagItem(CO_SOFTLINK, tags))!=0) {
                struct CacheBuffer *cb;

                if((errorcode=allocadminspace(&cb))==0) {
                  struct fsSoftLink *sl=cb->data;
                  UBYTE *dest=sl->string;
                  UBYTE *softlink=(UBYTE *)tag->ti_Data;

                  o->object.file.be_data=L2BE(cb->blckno);

                  sl->bheader.id=SOFTLINK_ID;
                  sl->bheader.be_ownblock=L2BE(cb->blckno);
                  sl->be_parent=o->be_objectnode;   // BE-BE copy
                  sl->be_next=0;
                  sl->be_previous=0;

                  while((*dest++=*softlink++)!=0) {
                  }

                  errorcode=storecachebuffer(cb);
                }
              }
            }
          }

          if(errorcode!=0) {
            dumpcachebuffer(*io_cb);
          }
        }
      }
    }
    else {
      if(errorcode==0) {
        errorcode=ERROR_OBJECT_EXISTS;
      }

      unlockcachebuffer(*io_cb);
    }
  }
  else {
    errorcode=ERROR_OBJECT_IN_USE;   /* Occurs when someone tries to create something in the recycled directory. */
  }

  return(errorcode);
}


#ifdef DEBUGCODE
static void copyobject(struct fsObject *o2,struct fsObject *o) {
  o->be_objectnode=o2->be_objectnode;   // BE-BE copy
  o->be_owneruid=o2->be_owneruid;   // BE-BE copy
  o->be_ownergid=o2->be_ownergid;   // BE-BE copy
  o->be_protection=o2->be_protection;   // BE-BE copy
  o->be_datemodified=o2->be_datemodified;   // BE-BE copy
  o->bits=o2->bits;

  if((o->bits & OTYPE_DIR)!=0) {
    o->object.dir.be_firstdirblock=o2->object.dir.be_firstdirblock;   // BE-BE copy
    o->object.dir.be_hashtable=o2->object.dir.be_hashtable;   // BE-BE copy
  }
  else {
    o->object.file.be_data=o2->object.file.be_data;   // BE-BE copy
    o->object.file.be_size=o2->object.file.be_size;   // BE-BE copy
  }
}
#endif


LONG setcomment2(struct CacheBuffer *cb, struct fsObject *o, UBYTE *comment) {
  UWORD commentlength=strlen(comment);
  LONG errorcode=0;

  if(commentlength<80) {
    UBYTE *oldcomment=getcomment(o);
    UWORD oldcommentlength=strlen(oldcomment);

    if(oldcommentlength!=0 || commentlength!=0) {
      if(changeobjectsize(cb, o, commentlength - oldcommentlength)==TRUE) {
        while((*oldcomment++=*comment++)!=0) {
        }

        if((errorcode=storecachebuffer(cb))==0) {
          checknotifyforobject(cb,o,TRUE);
        }
      }
      else {
        struct fsObject *oldo;
        UBYTE object[sizeof(struct fsObject)+110+80];
        NODE parent=BE2L(((struct fsObjectContainer *)cb->data)->be_parent);

        oldo=(struct fsObject *)object;

        CopyMem(o,object,(UBYTE *)nextobject(o)-(UBYTE *)o);

        if((errorcode=simpleremoveobject(cb,o))==0) {
          if((errorcode=readobject(parent, &cb, &o))==0) {         // out: cb & o = Parent of Object to get new comment.
            ULONG tag1,tag2;
            ULONG val1,val2;

            if((oldo->bits & OTYPE_DIR)!=0) {
              tag1=CO_HASHBLOCK;
              tag2=CO_FIRSTDIRBLOCK;

              val1=BE2L(oldo->object.dir.be_hashtable);
              val2=BE2L(oldo->object.dir.be_firstdirblock);
            }
            else {
              tag1=CO_DATA;
              tag2=CO_SIZE;

              val1=BE2L(oldo->object.file.be_data);
              val2=BE2L(oldo->object.file.be_size);
            }

            /* We've removed the object, but didn't remove it from the hashchain.
               This means functions like locateobject could stumble upon the
               removed object's ObjectNode, and follow it to a non-existing
               object.  That's why the CO_ALLOWDUPLICATES tag is passed. */

            /* In goes the Parent cb & o, out comes the New object's cb & o :-) */

            struct TagItem tags[] = {
            		{	CO_ALLOWDUPLICATES, TRUE },
            		{	CO_COMMENT, (IPTR)comment },
            		{	CO_OBJECTNODE, BE2L(oldo->be_objectnode) },
            		{	CO_PROTECTION, BE2L(oldo->be_protection) },
            		{	CO_DATEMODIFIED, BE2L(oldo->be_datemodified) },
            		{	CO_OWNER, (BE2W(oldo->be_owneruid)<<16) + BE2W(oldo->be_ownergid) },
            		{	CO_BITS, oldo->bits },
            		{	tag1, val1 },
            		{	tag2, val2 },
            		{	TAG_DONE, 0 }
            };

            if((errorcode=createobjecttagitem(&cb, &o, oldo->name, tags))==0) {

              if((errorcode=storecachebuffer(cb))==0) {
                checknotifyforobject(cb,o,TRUE);
              }
            }
          }
        }
      }
    }
  }
  else {
    errorcode=ERROR_COMMENT_TOO_BIG;
  }

  return(errorcode);

}



LONG setcomment(struct ExtFileLock *lock, UBYTE *path, UBYTE *comment) {
  struct CacheBuffer *cb;
  struct fsObject *o;
  LONG errorcode;

  if((errorcode=locatelockableobject(lock, path, &cb, &o))==0) {    // out: cb & o = Object to get new comment.
    errorcode=setcomment2(cb, o, comment);
  }

  return(errorcode);
}



LONG locatelockableparent(struct ExtFileLock *lock,UBYTE *path,struct CacheBuffer **returned_cb,struct fsObject **returned_o) {
  LONG errorcode;

  if(*path=='\0') {
    errorcode=locatelockableobject(lock,"/",returned_cb,returned_o);
  }
  else {
    UBYTE *objectname=FilePart(path);
    UBYTE c=*objectname;

    *objectname=0;

    errorcode=locatelockableobject(lock,path,returned_cb,returned_o);

    *objectname=c;
  }

  return(errorcode);
}



LONG locateparent(struct ExtFileLock *lock, UBYTE *path, struct CacheBuffer **returned_cb, struct fsObject **returned_o) {
  LONG errorcode;

  if(*path=='\0') {
    errorcode=locateobjectfromlock(lock,"/",returned_cb,returned_o);
  }
  else {
    UBYTE *objectname=FilePart(path);
    UBYTE c=*objectname;

    *objectname=0;

    errorcode=locateobjectfromlock(lock,path,returned_cb,returned_o);

    *objectname=c;
  }

  return(errorcode);
}



LONG lockfile(struct ExtFileLock *lock, UBYTE *path, LONG accessmode,struct ExtFileLock **returned_lock) {
  LONG errorcode;

  /* Locks a file -- ERROR_OBJECT_WRONG_TYPE is returned if a dir was attempted to be locked. */

  if((errorcode=lockobject(lock, path, accessmode, returned_lock))==0) {
    if(((*returned_lock)->bits & EFL_FILE)==0) {     // You're not allowed to open directories or softlinks as files.
      freelock(*returned_lock);
      errorcode=ERROR_OBJECT_WRONG_TYPE;
    }
  }

  return(errorcode);
}



LONG findcreate(struct ExtFileLock **returned_lock, UBYTE *path, LONG packettype, UBYTE *softlink) {
  struct ExtFileLock *lock;
  UBYTE create,delete;
  LONG accessmode;
  LONG errorcode;

  /* Used to open files, create files and create directories.

     First we check if the file mentioned exists, so we try to locate it.
     If the file does exist then depending on the packettype we need to
     open it or delete it.  If we had to delete the file/directory then
     we create a new one in its place. */

  /* There seems to be some confusion as to what ACTION_FINDUPDATE is supposed
     to do.  The 3 modes defined for the Amiga all work slightly different:

     - SHARED_LOCK or EXCLUSIVE_LOCK.
     - Creates a new file if one didn't exist.
     - Deletes existing file if one existed.

     ACTION_FINDINPUT:        SHARED_LOCK,    NO CREATE, NO DELETE
     ACTION_FINDUPDATE (1.3): EXCLUSIVE_LOCK, NO CREATE, NO DELETE
     ACTION_FINDUPDATE (2.0): SHARED_LOCK,    CREATE,    NO DELETE
     ACTION_FINDOUTPUT:       EXCLUSIVE_LOCK, CREATE,    DELETE
     ACTION_DELETE_OBJECT:    EXCLUSIVE_LOCK, NO CREATE, DELETE :-)    */

  path=validatepath(path);
  {
    UBYTE *s=FilePart(path);

    if(*s==0) {
      return(ERROR_INVALID_COMPONENT_NAME);
    }
  }

  if(packettype==ACTION_FINDINPUT) {
    accessmode=SHARED_LOCK;
    create=0;
    delete=0;

    _XDEBUG((DEBUG_OBJECTS,"ACTION_FINDINPUT: %s\n",path));
  }
  else if(packettype==ACTION_FINDOUTPUT || packettype==ACTION_MAKE_LINK) {
    accessmode=EXCLUSIVE_LOCK;
    create=1;
    delete=1;

    _XDEBUG((DEBUG_OBJECTS,"ACTION_FINDOUTPUT/ACTION_MAKE_LINK: %s\n",path));
  }
  else if(packettype==ACTION_CREATE_DIR) {
    accessmode=EXCLUSIVE_LOCK;
    create=1;
    delete=0;

    _XDEBUG((DEBUG_OBJECTS,"ACTION_CREATE_DIR: %s\n",path));
  }
  else {  // if(packettype==ACTION_FINDUPDATE)
    accessmode=SHARED_LOCK;
    create=1;
    delete=0;

    _XDEBUG((DEBUG_OBJECTS,"ACTION_FINDUPDATE: %s\n",path));
  } /*
  else if(packettype==ACTION_DELETE_OBJECT) {
    accessmode= ?? ;
    create=0;
    delete=1;
  } just an example... :-) */




  lock=*returned_lock;

  if(delete==0 && create==0) {
    errorcode=lockfile(lock,path,accessmode,returned_lock);
  }
  else {
    struct CacheBuffer *cb;
    struct fsObject *o;

    errorcode=locateobjectfromlock(lock,path,&cb,&o);

    /* 0,                NO DELETE, CREATE
       0,                DELETE,    NO CREATE
       0,                DELETE,    CREATE
       OBJECT_NOT_FOUND, NO DELETE, CREATE
       OBJECT_NOT_FOUND, DELETE,    NO CREATE
       OBJECT_NOT_FOUND, DELETE,    CREATE    */

    if(errorcode==0 && delete==0) {
      if(packettype==ACTION_CREATE_DIR) {
        errorcode=ERROR_OBJECT_EXISTS;
      }
      else {
        errorcode=lockfile(lock,path,accessmode,returned_lock);
      }
    }
    else if(errorcode==0 || errorcode==ERROR_OBJECT_NOT_FOUND) {
      if(delete==1 && create==0) {  // unused
        if(errorcode==0) {
          newtransaction();
          if((errorcode=deleteobject(lock, path, TRUE))==0) {
            endtransaction();
          }
          else {
            deletetransaction();
          }
        }
      }
      else {
        LONG lasterrorcode=errorcode;

        do {

          /* When we get here, we ALWAYS need to create a new object (possible
             after having deleted any existing objects) */

          newtransaction();

          if(lasterrorcode==0 && delete==1) {
            if((o->bits & OTYPE_DIR)!=0) {

              /* We are not allowed to overwrite directories (even if they're empty) */

              errorcode=ERROR_OBJECT_EXISTS;
            }
            else {
              errorcode=deleteobject(lock, path, FALSE);  /* Implicit deletes should not send a notify! */
            }
          }
          else {
            errorcode=0;
          }

          _XDEBUG((DEBUG_OBJECTS,"findcreate: locating lockable parent\n"));

          if(errorcode==0 && (errorcode=locateparent(lock,path,&cb,&o))==0) {  // was locatelockableparent()
            UBYTE bits=0;

            _XDEBUG((DEBUG_OBJECTS,"findcreate: creating object\n"));

            if(packettype==ACTION_CREATE_DIR) {
              bits|=OTYPE_DIR;
            }
            else if(packettype==ACTION_MAKE_LINK) {
              bits|=OTYPE_LINK;
            }

            struct TagItem tags[] = {
            		{ CO_BITS, bits },
            		{ CO_HASHOBJECT, TRUE },
            		{ CO_SOFTLINK, (IPTR)softlink },
            		{ CO_UPDATEPARENT, TRUE },
            		{ TAG_DONE, 0 }
            };

            if((errorcode=createobjecttagitem(&cb, &o, FilePart(path), tags))==0) {

              _XDEBUG((DEBUG_OBJECTS,"findcreate: New object is now complete\n"));

              if((errorcode=storecachebuffer(cb))==0) {

                lockcachebuffer(cb);

                if(packettype==ACTION_MAKE_LINK) {
                  checknotifyforobject(cb,o,TRUE);
                }
                else if((errorcode=lockobject(lock, path, accessmode, returned_lock))==0) {
                  if(packettype==ACTION_CREATE_DIR) {
                    checknotifyforobject(cb,o,TRUE);
                  }
                  else {
                    (*returned_lock)->bits|=EFL_MODIFIED;
                  }
                }

                unlockcachebuffer(cb);
              }
            }
          }

          if(errorcode!=0) {
            deletetransaction();
          }
          else {
            endtransaction();
          }
        } while(errorcode==ERROR_DISK_FULL && freeupspace()!=FALSE);
      }
    }
  }

  return(errorcode);
}



LONG findobjectspace(struct CacheBuffer **io_cb, struct fsObject **io_o, ULONG bytesneeded) {
  struct CacheBuffer *cbparent=*io_cb;
  struct fsObject *oparent=*io_o;
  struct CacheBuffer *cb;
  ULONG nextblock=BE2L(oparent->object.dir.be_firstdirblock);
  LONG errorcode=0;

  _XDEBUG((DEBUG_OBJECTS,"findobjectspace: Looking for %ld bytes in directory with ObjectNode number %ld (in block %ld)\n",bytesneeded,BE2L((*io_o)->be_objectnode),(*io_cb)->blckno));

  /* This function will look in the directory indicated by io_o
     for an ObjectContainer block which contains bytesneeded free
     bytes.  If none is found then this function simply creates a
     new ObjectContainer and adds that to the indicated directory.

     If OTYPE_QUICKDIR is set then only the first dir block is
     searched for enough free space, and if none is found a new
     block is added immediately.

     As this function may need to modify the disk, an operation
     should be in progress before calling this function. */

  lockcachebuffer(cbparent);

  while(nextblock!=0 && (errorcode=readcachebuffercheck(&cb,nextblock,OBJECTCONTAINER_ID))==0) {
    struct fsObjectContainer *oc=cb->data;
    UBYTE *emptyspace;

    /* We need to find out how much free space this ObjectContainer has */

    emptyspace=emptyspaceinobjectcontainer(oc);

    if((UBYTE *)oc+globals->bytes_block-emptyspace >= bytesneeded) {
      /* We found enough space in one of the ObjectContainer blocks!!
         We return a struct fsObject *. */

      preparecachebuffer(cb);

      *io_cb=cb;
      *io_o=(struct fsObject *)emptyspace;

      break;
    }

    if((oparent->bits & OTYPE_QUICKDIR)!=0 || ((oparent->bits & OTYPE_RINGLIST)!=0 && oc->be_next==oparent->object.dir.be_firstdirblock)) {    // BE-BE compare
      nextblock=0;
    }
    else {
      nextblock=BE2L(oc->be_next);
    }
  }

  if(nextblock==0) {
    struct CacheBuffer *cb;

    /* If we get here, we traversed the *entire* directory (ough!) and found no empty
       space large enough for our entry.  We allocate new space and add it to this
       directory. */

    if((errorcode=allocadminspace(&cb))==0) {
      struct fsObjectContainer *oc=cb->data;
      UBYTE ringlist=oparent->bits & OTYPE_RINGLIST;

      _XDEBUG((DEBUG_OBJECTS,"findobjectspace: No room was found, allocated new block at %ld\n",cb->blckno));

      /* Allocated new block.  We will now link it to the START of the directory chain
         so the new free space can be found quickly when more entries need to be added. */

      oc->bheader.id=OBJECTCONTAINER_ID;
      oc->bheader.be_ownblock=L2BE(cb->blckno);
      oc->be_parent=oparent->be_objectnode;    // BE-BE copy
      oc->be_next=oparent->object.dir.be_firstdirblock;   // BE-BE copy
      oc->be_previous=0;

      preparecachebuffer(cbparent);

      oparent->object.dir.be_firstdirblock=L2BE(cb->blckno);

      if((errorcode=storecachebuffer(cbparent))==0) {
        struct CacheBuffer *cbnext;

        if(oc->be_next!=0 && (errorcode=readcachebuffercheck(&cbnext,BE2L(oc->be_next),OBJECTCONTAINER_ID))==0) {
          struct fsObjectContainer *ocnext=cbnext->data;
          BLCK lastblock=BE2L(ocnext->be_previous);

          preparecachebuffer(cbnext);

          ocnext->be_previous=L2BE(cb->blckno);

          if((errorcode=storecachebuffer(cbnext))==0 && ringlist!=0) {

            oc->be_previous=L2BE(lastblock);

            if((errorcode=readcachebuffercheck(&cbnext,lastblock,OBJECTCONTAINER_ID))==0) {
              ocnext=cbnext->data;

              preparecachebuffer(cbnext);

              checksum_writelong_be(cbnext->data, &ocnext->be_next, cb->blckno);

              // ocnext->next=cb->blckno;

              errorcode=storecachebuffer_nochecksum(cbnext);
            }
          }
        }
        else if(ringlist!=0) {
          oc->be_previous=L2BE(cb->blckno);
          oc->be_next=L2BE(cb->blckno);
        }

        *io_cb=cb;
        *io_o=oc->object;
      }

      if(errorcode!=0) {
        dumpcachebuffer(cb);
      }
    }
  }

  unlockcachebuffer(cbparent);

  return(errorcode);
}



static LONG deleteobjectnode(NODE objectnode) {
  struct CacheBuffer *cb;
  struct fsObjectNode *on;
  LONG errorcode;

  _XDEBUG((DEBUG_NODES,"deleteobjectnode: Deleting Node %ld",objectnode));

  if((errorcode=findnode(globals->block_objectnoderoot, sizeof(struct fsObjectNode), objectnode, &cb, (struct fsNode **)&on))==0) {
    errorcode=deletenode(globals->block_objectnoderoot, cb, (struct fsNode *)on, sizeof(struct fsObjectNode));
  }

  return(errorcode);
}



struct fsObject *findobjectbyname(struct fsObjectContainer *oc, UBYTE *name) {
  struct fsObject *o=oc->object;

  while(isobject(o, oc)!=FALSE) {
    UBYTE *name1=o->name;
    UBYTE *name2=name;

    if(globals->is_casesensitive!=FALSE) {
      while(*name1!=0) {
        if(*name1!=*name2) {
          break;
        }
        name1++;
        name2++;
      }
    }
    else {
      while(*name1!=0) {
        if(upperchar(*name1)!=upperchar(*name2)) {
          break;
        }
        name1++;
        name2++;
      }
    }

    if(*name1==0 && (*name2==0 || *name2=='/')) {
      return(o);
    }

    o=nextobject(o);
  }

  return(0);
}



LONG scandir(struct CacheBuffer **io_cb, struct fsObject **io_o, UBYTE *name) {
  struct CacheBuffer *cb;
  BLCK hashblock=BE2L((*io_o)->object.dir.be_hashtable);
  LONG errorcode=0;

  /* Scans a directory for the object with the name /name/.  The directory
     to be scanned is given in *io_cb & *io_o.  The resulting object (if any)
     is returned in *io_cb and *io_o.

     This function handles directories with no hashblock correctly. */

  if(hashblock!=0) {
    if((errorcode=readcachebuffercheck(&cb, hashblock, HASHTABLE_ID))==0) {
      struct fsHashTable *ht=cb->data;
      struct fsObjectNode *on;
      UWORD hash16;
      NODE nextobjectnode,objectnode;

      /* cb is a HashTable block.  name is used to calculate the correct
         hash-value and the correct hash-chain is traversed until either
         the hash chain ends, or the name is located. */

      hash16=hash(name, globals->is_casesensitive);
      objectnode=BE2L(ht->be_hashentry[HASHCHAIN(hash16)]);

      while(objectnode!=0) {
        if((errorcode=findnode(globals->block_objectnoderoot, sizeof(struct fsObjectNode), objectnode, &cb, (struct fsNode **)&on))!=0) {
          return(errorcode);
        }

        nextobjectnode=BE2L(on->be_next);

        if(BE2W(on->be_hash16)==hash16) {
          if((errorcode=readcachebuffercheck(&cb, BE2L(on->node.be_data), OBJECTCONTAINER_ID))==0) {
            struct fsObject *o;

            if((o=findobjectbyname((struct fsObjectContainer *)cb->data, name))!=0) {
              *io_cb=cb;
              *io_o=o;

              if((o->bits & OTYPE_LINK)!=0) {
                return(ERROR_IS_SOFT_LINK);
              }

              return(0);
            }
          }

          /* Code below doesn't handle Soft Links correctly.  There is a slight chance
             that it returns ERROR_IS_SOFT_LINK before knowing that the names really
             match.

          if((errorcode=readobjectquick(on->node.data,objectnode,&cb,&o))!=0) {
            if(errorcode==ERROR_IS_SOFT_LINK) {
              *io_cb=cb;
              *io_o=o;
            }

            return(errorcode);
          }

          if(compareobjectnames(o->name, name)!=FALSE) {
            *io_cb=cb;
            *io_o=o;

            return(0);
          }

          */

        }

        objectnode=nextobjectnode;
      }

      return(ERROR_OBJECT_NOT_FOUND);
    }
  }
  else {
    struct fsObjectContainer *oc;
    struct fsObject *o;
    BLCK nextdirblock=BE2L((*io_o)->object.dir.be_firstdirblock);

    while(nextdirblock!=0 && (errorcode=readcachebuffercheck(&cb, nextdirblock, OBJECTCONTAINER_ID))==0) {
      oc=cb->data;

      if((o=findobjectbyname(oc, name))!=0) {
        *io_cb=cb;
        *io_o=o;

        if((o->bits & OTYPE_LINK)!=0) {
          return(ERROR_IS_SOFT_LINK);
        }

        return(0);
      }

      nextdirblock=BE2L(oc->be_next);
    }

    if(nextdirblock==0) {
      errorcode=ERROR_OBJECT_NOT_FOUND;
    }
  }

  return(errorcode);
}



LONG readobject(NODE objectnode,struct CacheBuffer **returned_cb,struct fsObject **returned_object) {
  struct CacheBuffer *cb;
  struct fsObjectNode *on;
  LONG errorcode;

  if((errorcode=findnode(globals->block_objectnoderoot, sizeof(struct fsObjectNode), objectnode, &cb, (struct fsNode **)&on))==0) {
    errorcode=readobjectquick(BE2L(on->node.be_data),objectnode,returned_cb,returned_object);
  }

  return(errorcode);
}



LONG readobjectquick(BLCK objectcontainer,NODE objectnode,struct CacheBuffer **returned_cb,struct fsObject **returned_object) {
  LONG errorcode;

  if((errorcode=readcachebuffercheck(returned_cb,objectcontainer,OBJECTCONTAINER_ID))==0) {
    struct fsObjectContainer *oc=(*returned_cb)->data;

    if((*returned_object=findobject(oc,objectnode))==0) {
      /**** if the system gets here then there was an ObjectNode which didn't point
            to a block in which the object exists.  This means there MUST be something wrong
            with the filesystem or the disk. */

      req("ObjectNode %ld points to a block which\ndoesn't contain the intended Object.", "Ok", objectnode);
      errorcode=ERROR_OBJECT_NOT_FOUND;
    }
    else if(((*returned_object)->bits & OTYPE_LINK)!=0) {
      errorcode=ERROR_IS_SOFT_LINK;
    }
  }

  return(errorcode);
}



struct fsObject *findobject(struct fsObjectContainer *oc,NODE objectnode) {
  struct fsObject *o=oc->object;
  UBYTE *endadr;

  endadr=(UBYTE *)oc+globals->bytes_block-sizeof(struct fsObject)-2;

  while((UBYTE *)o<endadr && o->name[0]!=0) {
    if(BE2L(o->be_objectnode)==objectnode) {
      /* a match, which means the correct object was located. */
      return(o);
    }
    o=nextobject(o);
  }

  return(0);
}



WORD objectsize(struct fsObject *o) {
  UBYTE *p;

  /* Returns the exact size of this object in bytes. */

  p=(UBYTE *)&o->name[0];

  /* skip the filename */
  while(*p++!=0) {
  }

  /* skip the comment */
  while(*p++!=0) {
  }

  return((WORD)(p-(UBYTE *)o));
}



UBYTE *getcomment(struct fsObject *o) {
  UBYTE *p;

  p=(UBYTE *)&o->name[0];

  /* skip the filename */
  while(*p++!=0) {
  }

  return(p);
}



struct fsObject *lastobject(struct fsObjectContainer *oc) {
  struct fsObject *oprev;
  struct fsObject *o=oc->object;

  /* returns the last object. */

  do {
    oprev=o;
    o=nextobject(o);
  } while(isobject(o, oc)!=FALSE);

  return(oprev);
}



struct fsObject *prevobject(struct fsObject *o, struct fsObjectContainer *oc) {
  struct fsObject *oprev=0;
  struct fsObject *o2=oc->object;

  /* returns the previous object.  If there is no previous, zero is returned. */

  while(o2!=o) {
    oprev=o2;
    o2=nextobject(o2);
    if(isobject(o2, oc)==FALSE) {
      return(0);
    }
  }

  return(oprev);
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
  if((((IPTR)p) & 0x01)!=0) {
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



static UBYTE *emptyspaceinobjectcontainer(struct fsObjectContainer *oc) {
  struct fsObject *o=oc->object;
  UBYTE *endadr;

  /* This function returns a pointer to the first unused byte in
     an ObjectContainer. */

  endadr=(UBYTE *)oc+globals->bytes_block-sizeof(struct fsObject)-2;

  while((UBYTE *)o<endadr && o->name[0]!=0) {
    o=nextobject(o);
  }

  return((UBYTE *)o);
}



LONG removeobject(struct CacheBuffer *cb, struct fsObject *o) {
  struct fsObjectContainer *oc=cb->data;
  LONG errorcode;

  /* This function removes an object from any directory.  It takes care
     of delinking the object from the hashchain and also frees the
     objectnode number.

     This function must be called from within a transaction! */

  lockcachebuffer(cb);

  if((errorcode=dehashobjectquick(BE2L(o->be_objectnode), o->name, BE2L(oc->be_parent)))==0) {
    NODE objectnode=BE2L(o->be_objectnode);

    if((errorcode=simpleremoveobject(cb, o))==0) {
      errorcode=deleteobjectnode(objectnode);
    }
  }

  unlockcachebuffer(cb);

  return(errorcode);
}






LONG renameobject(struct CacheBuffer *cb, struct fsObject *o, struct ExtFileLock *lock, UBYTE *path) {
  struct CacheBuffer *cbparent;
  struct fsObject *oparent;
  LONG errorcode;

  /* cb & o is the original object.  The lock and path is the new name and location of the object. */

  lockcachebuffer(cb);

  if((errorcode=locatelockableparent(lock, path, &cbparent, &oparent))==0) {
    unlockcachebuffer(cb);
    errorcode=renameobject2(cb, o, cbparent, oparent, FilePart(path), TRUE);
  }
  else {
    unlockcachebuffer(cb);
  }

  return(errorcode);
}



LONG renameobject2(struct CacheBuffer *cb, struct fsObject *o, struct CacheBuffer *cbparent, struct fsObject *oparent, UBYTE *newname, WORD sendnotify) {
  struct fsObjectContainer *oc=cb->data;
  struct fsObject *oldo;
  UBYTE object[sizeof(struct fsObject)+110+80];
  UBYTE *comment;
  UBYTE *notifypath=0;
  NODE objectnode=BE2L(o->be_objectnode);
  NODE sourceparentobjectnode=BE2L(oc->be_parent);
  LONG errorcode=0;

  /* The Object indicated by cb & o, gets renamed to newname and placed
     in the directory indicated by cbparent & oparent. */

  _XDEBUG((DEBUG_OBJECTS,"renameobject2: Renaming '%s' to '%s' in dir '%s'\n",o->name,newname,oparent->name));

  oldo=(struct fsObject *)object;

  lockcachebuffer(cb);
  lockcachebuffer(cbparent);

  if((o->bits & OTYPE_DIR)!=0 && oc->be_parent!=oparent->be_objectnode) {
    NODE objectnode=BE2L(o->be_objectnode);

    {
      struct CacheBuffer *cb=cbparent;
      struct fsObject *o=oparent;
      struct fsObjectContainer *oc;

      /* We should check if o isn't a parent of oparent so to avoid renaming
         a directory into one of its children.  Of course, we only need to
         do this when o is a directory, and oparent isn't the parent of o. */

      do {
        oc=cb->data;

        if(objectnode==BE2L(o->be_objectnode)) {
          errorcode=ERROR_OBJECT_IN_USE;
          break;
        }

      } while(oc->be_parent!=0 && (errorcode=readobject(BE2L(oc->be_parent), &cb, &o))==0);
    }
  }

  if(errorcode==0) {
    if(sendnotify!=FALSE) {
      notifypath=fullpath(cb,o);
    }

    CopyMem(o, object, (UBYTE *)nextobject(o)-(UBYTE *)o);

    comment=oldo->name;

    /* skip the filename */
    while(*comment++!=0) {
    }

    if((errorcode=dehashobjectquick(objectnode, o->name ,BE2L(oc->be_parent)))==0) {
      ULONG parentobjectnode=BE2L(oparent->be_objectnode);

      unlockcachebuffer(cb);

      /* If moving an Object from a directory to a subdirectory of that directory and
         the Object is in the same ObjectContainer, then it is possible that oparent
         won't point to a valid object anymore after calling simpleremoveobject(). */

      if((errorcode=simpleremoveobject(cb,o))==0) {
        struct CacheBuffer *cb=cbparent;
        struct fsObject *o;
        ULONG tag1,tag2;
        ULONG val1,val2;

        o=findobject((struct fsObjectContainer *)cb->data,parentobjectnode);

        if((oldo->bits & OTYPE_DIR)!=0) {
          tag1=CO_HASHBLOCK;
          tag2=CO_FIRSTDIRBLOCK;

          val1=BE2L(oldo->object.dir.be_hashtable);
          val2=BE2L(oldo->object.dir.be_firstdirblock);
        }
        else {
          tag1=CO_DATA;
          tag2=CO_SIZE;

          val1=BE2L(oldo->object.file.be_data);
          val2=BE2L(oldo->object.file.be_size);
        }

        /* In goes the Parent cb & o, out comes the New object's cb & o :-) */

        struct TagItem tags[] = {
        		{ CO_COMMENT, (IPTR)comment },
        		{ CO_OBJECTNODE, BE2L(oldo->be_objectnode) },
        		{ CO_PROTECTION, BE2L(oldo->be_protection) },
        		{ CO_DATEMODIFIED, BE2L(oldo->be_datemodified) },
        		{ CO_OWNER, (BE2W(oldo->be_owneruid)<<16) + BE2W(oldo->be_ownergid) },
        		{ CO_BITS, oldo->bits },
        		{ CO_HASHOBJECT, TRUE },
        		{ CO_UPDATEPARENT, TRUE },
        		{ tag1, val1 },
        		{ tag2, val2 },
        		{ TAG_DONE, 0 }
        };

        if((errorcode=createobjecttagitem(&cb, &o, newname, tags))==0) {

          if((errorcode=storecachebuffer(cb))==0 && sendnotify!=FALSE) {    // Object itself has been completed.

            _XDEBUG((DEBUG_OBJECTS,"renameobject2: Succesfully created & stored new object.\n"));

            if(parentobjectnode!=sourceparentobjectnode) {
              /* Object was moved! */
              checknotifyforpath(notifypath,TRUE);
            }
            else {
              /* Object was renamed within same directory */
              checknotifyforpath(notifypath,FALSE);
            }
            checknotifyforobject(cb,o,TRUE);   /* fullpath points to a global string so put this notify after the 2 others. */
          }
        }
      }
    }
    else {
      unlockcachebuffer(cb);
    }
  }
  else {
    unlockcachebuffer(cb);
  }

  unlockcachebuffer(cbparent);

  return(errorcode);
}



static WORD changeobjectsize(struct CacheBuffer *cb, struct fsObject *o, WORD bytes) {
  struct fsObjectContainer *oc=cb->data;
  WORD currentobjectsize;
  WORD newobjectsize;
  WORD words;

  /* This function resizes the passed in Object, if possible.
     A negative bytes number reduces the size, a positive number
     increases the size.  This function will return TRUE on
     success.  newtransaction() must have been called before calling
     this function. */

  // currentobjectsize & newobjectsize in bytes:

  currentobjectsize=objectsize(o);
  newobjectsize=currentobjectsize+bytes;

  // currentobjectsize & newobjectsize in words:

  currentobjectsize++;
  currentobjectsize>>=1;

  newobjectsize++;
  newobjectsize>>=1;

  words=newobjectsize-currentobjectsize;

  if(words==0) {
    preparecachebuffer(cb);

    return(TRUE);
  }

  if(words<0) {
    UWORD *src;
    UWORD *dst;
    WORD copywords;

    preparecachebuffer(cb);

    words=-words;

    src=(UWORD *)o + currentobjectsize;
    dst=src-words;

    copywords=(globals->bytes_block>>1)-(src-(UWORD *)oc);

    while(--copywords>=0) {
      *dst++=*src++;
    }

    while(--words>=0) {
      *dst++=0;
    }
  }
  else if(emptyspaceinobjectcontainer(oc)-(UBYTE *)oc+(words<<1)<=globals->bytes_block) {
    UWORD *src;
    UWORD *dst;
    WORD copywords;

    preparecachebuffer(cb);

    dst=(UWORD *)((UBYTE *)oc+globals->bytes_block);
    src=dst-words;

    copywords=(globals->bytes_block>>1) - (((UWORD *)o + currentobjectsize + words)-(UWORD *)oc);

    while(--copywords>=0) {
      *--dst=*--src;
    }

    *--dst=0;
  }
  else {
    return(FALSE);
  }

  return(TRUE);
}
