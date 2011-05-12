#include "asmsupport.h"

#include <dos/dosextens.h>
#include <exec/memory.h>
#include <exec/types.h>
#include <proto/exec.h>

#include "locks.h"
#include "locks_protos.h"

#include "debug.h"
#include "fs.h"
#include "objects_protos.h"
#include "support_protos.h"

#include "globals.h"

extern LONG req(UBYTE *fmt, UBYTE *gads, ... );

void freeglobalhandle(struct GlobalHandle *gh);

LONG lockable(NODE objectnode,LONG accessmode) {
  struct ExtFileLock *lock;

  /* returns DOSTRUE if the object passed in is lockable, taking the accessmode
     into account */

/** If looking through the locklist linearly turns out to be slow then
    this routine may well be enhanced later with a hashing scheme */

  lock=globals->locklist;

  while(lock!=0) {
    if(lock->objectnode==objectnode) {
      if(lock->access==EXCLUSIVE_LOCK || accessmode==EXCLUSIVE_LOCK) {
        return(DOSFALSE);
      }
      break;
    }
    lock=lock->next;
  }

  if(objectnode==globals->templockedobjectnode && accessmode==EXCLUSIVE_LOCK) {
    return(DOSFALSE);
  }

  return(DOSTRUE);
}



void __inline settemporarylock(NODE objectnode) {
  globals->templockedobjectnode=objectnode;
}



void __inline cleartemporarylock(void) {
  globals->templockedobjectnode=0;
}


LONG freelock(struct ExtFileLock *lock) {

  /* You may assume that this function never fails for locks
     temporarily allocated internally. */

  if(lock!=0) {
    if(lock->id==DOSTYPE_ID) {
      lock->task=0;

      if(lock->next!=0) {
        lock->next->prev=lock->prev;
      }

      if(lock->prev!=0) {
        lock->prev->next=lock->next;
        lock->prev->link=TOBADDR(lock->next);
      }
      else {

#if 0
//            #ifdef STARTDEBUG
              {
                struct DeviceList *vn=BADDR(lock->volume);

                if(vn->dl_LockList!=0) {
                  req("Freeing lock!", "Ok");

                  if(lock->next==0) {
                    req("This was the last lock!", "Ok");
                  }
                }
              }
//            #endif
#endif

#if 0
        struct DeviceList *vn=BADDR(lock->volume);

        if(vn->dl_LockList!=0) {
          _DEBUG(("freelock: Freeing a lock of a volume which isn't currently inserted.\n"));

          if(lock->next==0) {
            /* Whoops... this was the last lock which was associated with a volume
               not currently inserted.  This means the volumenode's LockList pointer
               becomes zero if there are still some NotifyRequest's left, or it is
               completely removed. */

            _DEBUG(("freelock: This was the last lock associated with that volume!\n"));

//            #ifdef STARTDEBUG
//              req("Last lock was freed!", "Ok");
//            #endif

            if(vn->dl_unused==0) {
              /* remove the volumenode! */
            }
          }
          else {
            vn->dl_LockList=TOBADDR(lock->next);
          }
        }
        else {
          locklist=lock->next;
        }

#endif

        globals->locklist=lock->next;
      }

      if(lock->gh!=0) {
        freeglobalhandle(lock->gh);
      }

      FreeMem(lock,sizeof(struct ExtFileLock));
    }
    else {
      return(ERROR_INVALID_LOCK);
    }
  }
  return(0);
}


LONG lockobject2(struct fsObject *o, LONG accessmode, struct ExtFileLock **returned_efl) {
  struct ExtFileLock *lock;

  /* This function locks the passed object and returns an
     ExtFileLock structure if successful. */

  if(BE2L(o->be_objectnode)==ROOTNODE && accessmode==EXCLUSIVE_LOCK) {
    /* Exclusive locks on the ROOT directory are not allowed */
    _DEBUG(("lockobject: someone tried to lock the ROOT directory exclusively -- denied\n"));

    return(ERROR_OBJECT_IN_USE);
  }

  /* Below we check if a lock on this object already exists.  If a lock
     already exists and is exclusive then this locking attempt is
     rejected.  If we are requesting an exclusive lock while another
     lock already exists then the locking attempt is rejected as well. */

  if(lockable(BE2L(o->be_objectnode), accessmode)!=DOSFALSE) {

    _XDEBUG((DEBUG_LOCK,"lockobject: lockable returned TRUE\n"));

    /* If we got this far then we can safely create the requested lock and
       add it to the locklist. */

    if((lock=AllocMem(sizeof(struct ExtFileLock), MEMF_PUBLIC|MEMF_CLEAR))!=0) {
      lock->objectnode=BE2L(o->be_objectnode);

      /* accessmode is not simply copied because some apps think anything which isn't
         an exclusive lock is a shared lock. */

      if(accessmode==EXCLUSIVE_LOCK) {
        lock->access=EXCLUSIVE_LOCK;
      }
      else {
        lock->access=SHARED_LOCK;
      }

      lock->task=&globals->mytask->pr_MsgPort;
      lock->volume=TOBADDR(globals->volumenode);

      lock->link=TOBADDR(globals->locklist);
      lock->next=globals->locklist;
      lock->prev=0;
      if(globals->locklist!=0) {
        globals->locklist->prev=lock;
      }
      globals->locklist=lock;

      lock->id=DOSTYPE_ID;

      lock->curextent=BE2L(o->object.file.be_data);

      if((o->bits & (OTYPE_LINK|OTYPE_DIR))==0) {
        lock->bits=EFL_FILE;
      }

      *returned_efl=lock;
      return(0);
    }
    else {
      return(ERROR_NO_FREE_STORE);
    }
  }
  else {
    return(ERROR_OBJECT_IN_USE);
  }
}



LONG lockobject(struct ExtFileLock *efl, UBYTE *path, LONG accessmode, struct ExtFileLock **returned_efl) {
  struct CacheBuffer *cb;
  struct fsObject *o;
  LONG errorcode;

  /* Creates a new ExtFileLock if possible.

     ERROR_OBJECT_IN_USE is returned if an exclusive lock is present on the object
     to be locked, or if a shared lock is present while you're trying to lock it
     exclusively.

     ERROR_INVALID_LOCK is returned if the lock is not a lock created by this
     filesystem. */

  if(efl==0 || efl->id==DOSTYPE_ID) {
    if((errorcode=locateobjectfromlock(efl, path, &cb, &o))==0) {
      return(lockobject2(o, accessmode, returned_efl));
    }
    else {
      _XDEBUG((DEBUG_LOCK,"lockobject: locateobject failed\n"));

      return(errorcode);
    }
  }
  else {
    return(ERROR_INVALID_LOCK);
  }
}



LONG locateobjectfromlock(struct ExtFileLock *lock, UBYTE *path, struct CacheBuffer **returned_cb, struct fsObject **returned_o) {
  NODE objectnode;
  LONG errorcode;

  if(lock==0) {
    objectnode=ROOTNODE;
  }
  else {
    objectnode=lock->objectnode;
  }

  if((errorcode=readobject(objectnode, returned_cb, returned_o))==0) {
    errorcode=locateobject2(&path, returned_cb, returned_o);
  }

  return(errorcode);
}



LONG locatelockableobject(struct ExtFileLock *lock, UBYTE *path, struct CacheBuffer **returned_cb, struct fsObject **returned_o) {
  NODE objectnode;
  LONG errorcode;

  if(lock==0) {
    objectnode=ROOTNODE;
  }
  else {
    objectnode=lock->objectnode;
  }

  if((errorcode=readobject(objectnode, returned_cb, returned_o))==0) {
    if((errorcode=locateobject2(&path, returned_cb, returned_o))==0) {
      if(objectnode!=BE2L((*returned_o)->be_objectnode) && lockable(BE2L((*returned_o)->be_objectnode), SHARED_LOCK)==DOSFALSE) {
        errorcode=ERROR_OBJECT_IN_USE;
      }
    }
  }

  return(errorcode);
}



LONG locateobject(UBYTE *path, struct CacheBuffer **io_cb, struct fsObject **io_o) {
  UBYTE *p=path;

  return(locateobject2(&p, io_cb, io_o));
}



LONG locateobject2(UBYTE **io_path, struct CacheBuffer **io_cb, struct fsObject **io_o) {
  UBYTE *path=*io_path;
  LONG errorcode=0;

  /* Path is a normal C string.  It may include a colon.  Everything up to the
     last colon found in the string (if any) will be ignored.

     This function parses the path passed in and locates the object it refers
     to relative to the passed in object.
     This function returns an AmigaDOS errorcode in case of a problem, or zero
     if everything went okay.

     ERROR_OBJECT_WRONG_TYPE will be returned if a file is accidentally
     referred to as a directory.

     ERROR_OBJECT_NOT_FOUND will be returned if we can't find the object. */

  path=stripcolon(path);

  _XDEBUG((DEBUG_LOCK,"locateobject: Locating object with path '%s' from ObjectNode %ld\n",path,BE2L((*io_o)->be_objectnode)));

  while(*path!=0) {

    /* We've got an Object which is the start of the path which still needs to
       be parsed.  If the path starts with a slash we get the parent block and
       remove the slash from the path.  If the path doesn't start with a slash
       we scan the directory for the Object which that part of the path
       indicates.  We continue this process until the entire path has been
       parsed. */

    if(*path=='/') {
      struct fsObjectContainer *oc=(*io_cb)->data;

      if(BE2L(oc->be_parent)==0) {
        /* We can't get the parent of the root! */

        _XDEBUG((DEBUG_LOCK,"locateobject: Can't get parent of the root\n"));

        errorcode=ERROR_OBJECT_NOT_FOUND;
        break;
      }

      if((errorcode=readobject(BE2L(oc->be_parent),io_cb,io_o))!=0) {
        break;
      }

      path++;
    }
    else {
      /* We've got ourselves an fsObject. */

       /*
      if(((*io_o)->bits & OTYPE_LINK)!=0) {
        errorcode=ERROR_IS_SOFT_LINK;
        break;
      }
      */

      if(((*io_o)->bits & OTYPE_DIR)==0) {
        _XDEBUG((DEBUG_LOCK,"locateobject: Not a directory\n"));

        errorcode=ERROR_OBJECT_WRONG_TYPE;
        break;
      }

      if((errorcode=scandir(io_cb, io_o, path))!=0) {
        break;
      }

      /* We found a piece of the path we were looking for.  The fsObject
         structure returned is the object we are looking for. */

      while(*path!=0) {
        if(*path++=='/') {
          break;
        }
      }
    }
  }

  *io_path=path;

  return(errorcode);
}



LONG createglobalhandle(struct ExtFileLock *efl) {
  struct ExtFileLock *lock;
  struct CacheBuffer *cb;
  struct fsObject *o;
  LONG errorcode;

  lock=globals->locklist;

  while(lock!=0) {
    if(lock->objectnode==efl->objectnode) {
      if(lock->gh!=0) {
        efl->gh=lock->gh;
        lock->gh->count++;

        // _DEBUG(("createglobalhandle: Returning existing gh structure\n"));

        return(0);
      }
    }
    lock=lock->next;
  }

  if((errorcode=readobject(efl->objectnode, &cb, &o))==0) {
    if((efl->gh=AllocMem(sizeof(struct GlobalHandle), MEMF_PUBLIC))!=0) {
      struct GlobalHandle *gh=efl->gh;

      // _DEBUG(("createglobalhandle: Allocated new gh structure\n"));

      gh->count=1;

      gh->objectnode=efl->objectnode;
      gh->size=BE2L(o->object.file.be_size);
      gh->protection=BE2L(o->be_protection);
      gh->data=BE2L(o->object.file.be_data);

      addtailm(&globals->globalhandles,&gh->node);

      return(0);
    }
    else {
      return(ERROR_NO_FREE_STORE);
    }
  }

  return(errorcode);
}



struct GlobalHandle *findglobalhandle(NODE objectnode) {
  struct GlobalHandle *gh;

  gh=(struct GlobalHandle *)globals->globalhandles.mlh_Head;

  while(gh->node.mln_Succ!=0) {
    if(gh->objectnode==objectnode) {
      return(gh);
    }

    gh=(struct GlobalHandle *)(gh->node.mln_Succ);
  }

  return(0);
}



void freeglobalhandle(struct GlobalHandle *gh) {
  gh->count--;
  if(gh->count==0) {
    removem(&gh->node);
    FreeMem(gh,sizeof(struct GlobalHandle));
  }
}



void updatelocksaftermove(BLCK source, BLCK dest, ULONG blocks) {
  struct ExtFileLock *lock=globals->locklist;
  struct GlobalHandle *gh;

  while(lock!=0) {
    if(lock->curextent >= source && lock->curextent < source+blocks) {
      lock->curextent=0;
      lock->extentoffset=0;
    }
    lock=lock->next;
  }

  /* The FirstDataBlock pointer in a GlobalHandle might need to be
     changed.  Because this is the first extent of a file, it cannot
     be merged with a previous extent.  This means that /source/ must
     equal the FirstDataBlock number exactly, and that /dest/ is
     always the new location. */

  gh=(struct GlobalHandle *)globals->globalhandles.mlh_Head;

  while(gh->node.mln_Succ!=0) {
    if(gh->data==source) {
      gh->data=dest;
    }
    gh=(struct GlobalHandle *)(gh->node.mln_Succ);
  }
}
