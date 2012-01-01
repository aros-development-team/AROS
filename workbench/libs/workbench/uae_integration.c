/*
    Copyright  2010, The AROS Development Team. All rights reserved.
    $Id$
*/

/**************************************************************************
 * uae_integration.c
 *
 * This file contains helper functions to detect, if J-UAE is running and
 * to detect, if an executeable is a m68k amigaOS file.
 *
 * It also gives wanderer.c a function, to forward a Message to
 * the m68k amigaOS launchd via J-UAE, so that the correct
 * executeable is started with all wanderer arguments.
 *
 **************************************************************************/

#include <proto/utility.h>
#include <utility/tagitem.h>
#include <workbench/workbench.h>

#ifdef __AROS__
//#define DEBUG 1
#include <aros/debug.h>
#endif

#include "workbench_intern.h"
#include "uae_integration.h"

/**************************************************************************
 * get_j_uae_port
 *
 * return the port, Janus UAE uses to launch applications 
 **************************************************************************/
static struct MsgPort *get_j_uae_port(void) 
{
    return FindPort((STRPTR) J_UAE_PORT);
}

/**************************************************************************
 * j_uae_running
 *
 * test if Janus UAE is running
 **************************************************************************/
BOOL j_uae_running(void)
{
    if( get_j_uae_port() != NULL)
    {
	D(bug("[WBLIB] %s: J-UAE port found\n", __PRETTY_FUNCTION__));
	return TRUE;
    }

    D(bug("[WBLIB] %s: J-UAE port \"%s\" *not* found\n", __PRETTY_FUNCTION__, J_UAE_PORT));
    return FALSE;
}

/**************************************************************************
 * is_68k(filename with path)
 *
 * test, if the supplied file is an AmigaOS 68k binary
 *
 * (Amiga 68k executables start with 0x00 0x00 0x03 0xf3)
 **************************************************************************/
BOOL is_68k(STRPTR filename, APTR WorkbenchBase)
{
    BPTR  fh;
    UBYTE begin[4];

    fh=Open(filename, MODE_OLDFILE);

    if(fh) 
    {
    	if(Read(fh, begin, 4) == 4) 
	{
    	    if( (begin[0] == 0x00) && (begin[1] == 0x00) && (begin[2] == 0x03) && (begin[3] == 0xf3) ) 
	    {
		D(bug("[WBLIB] %s: Amiga 68k executable found: %s\n", __PRETTY_FUNCTION__, filename));
	      	Close(fh);
	       	return TRUE;
    	    }
   	}
  	Close(fh);
    }
    return FALSE;
}

/**************************************************************************
 * tag_list_nr(struct TagItem *tstate)
 *
 * count number of TagItems
 **************************************************************************/
static ULONG tag_list_nr(const struct TagItem *tstate, APTR WorkbenchBase)
{
    ULONG count;

    count=0;
    while(NextTagItem((struct TagItem **)&tstate)) 
    {
     	count++;
    }

    D(bug("[WBLIB] %s: TagList has %d items.\n", __PRETTY_FUNCTION__, count));
    return count;
}

/**************************************************************************
 * str_dup_vec(char *in) 
 *
 * Allocate a buffer for string in with AllocVecPooled
 **************************************************************************/
static STRPTR str_dup_vec(void *mempool, char *in) 
{
    char *result;

    if(!in) 
    {
     	return 0;
    }

    result=AllocVecPooled(mempool, strlen(in)+1);
    strcpy(result, in);
  
    return (STRPTR) result;
}

#define LOCK_NAME_BUFFER_SIZE 2000

/**************************************************************************
 * tag_list_clone(struct TagItem *in)
 *
 * clone the TagList in and also clone all strings, in points to
 **************************************************************************/
struct TagItem *tag_list_clone(void *mempool, const struct TagItem *in, APTR WorkbenchBase) 
{

    ULONG   nr;
    ULONG   t;
    struct TagItem *result;
    char   *name_from_lock;
    struct TagItem *tag;

    nr=tag_list_nr(in, WorkbenchBase);

    if(!nr) 
    {
     	return NULL;
    }

    /* don't ask me, if all that stuff can work on 64-Bit AROS, sorry */

    /* local only, no pool required */
    name_from_lock=(char *) AllocVec(LOCK_NAME_BUFFER_SIZE, MEMF_CLEAR); 
    if(!name_from_lock) 
    {
     	D(bug("[WBLIB] %s: ERROR: AllocVec failed!\n", __PRETTY_FUNCTION__));
      	return NULL;
    }

    result=(struct TagItem *) AllocVecPooled(mempool, (nr+1) * sizeof(struct TagItem));
    if(!result) 
    {
     	D(bug("[WBLIB] %s: ERROR: AllocVecPooled failed!\n", __PRETTY_FUNCTION__));
      	FreeVec(name_from_lock);
      return NULL;
    }

    t=0;
    while( (tag=NextTagItem((struct TagItem **)&in)) ) 
    {
	switch(tag->ti_Tag) {

	  case WBOPENA_ArgLock:
	      NameFromLock((BPTR) tag->ti_Data, name_from_lock, LOCK_NAME_BUFFER_SIZE);
	      UnLock((BPTR) tag->ti_Data); /* do we have to UnLock it? */
	      result[t].ti_Tag =WBOPENA_ArgLock;
	      result[t].ti_Data=(STACKIPTR) str_dup_vec(mempool, name_from_lock);
	      D(bug("[WBLIB] %s: WBOPENA_ArgLock: %s\n", __PRETTY_FUNCTION__, name_from_lock));
	      t++;
	  break;

	  case WBOPENA_ArgName:
	      result[t].ti_Tag =WBOPENA_ArgName;
	      result[t].ti_Data=(STACKIPTR) str_dup_vec(mempool, (char *) tag->ti_Data);
	      D(bug("[WBLIB] %s: WBOPENA_ArgName: %s\n", __PRETTY_FUNCTION__, tag->ti_Data));
	      t++;
	  break;
      }
    }

    /* terminate. not necessary, but feels better. */
    result[t].ti_Tag =TAG_DONE;
    result[t].ti_Data=TAG_DONE;

    FreeVec(name_from_lock);

    return result;
}

/**************************************************************************
 * forward_to_uae(struct TagItem *argsTagList, char *name)
 *
 * Sends a message to the J-UAE port in order to launch the
 * according m68k amigaOS executeable.
 *
 * Errorhandling is completely up to J-UAE!
 * J-UAE has to free all message resources!
 **************************************************************************/
void forward_to_uae(struct TagItem *argsTagList, char *name, APTR WorkbenchBase)
{

    struct MsgPort              *port;
    struct JUAE_Launch_Message  *msg;
    void                        *pool;

    D(bug("[WBLIB] %s: j_uae_running and is_68k!\n", __PRETTY_FUNCTION__));

    /* We allocate everything in a pool, so we can free it easily afterwards 
     * J-UAE has to free everything
     * */
    pool=CreatePool(MEMF_CLEAR|MEMF_PUBLIC, 2048, 1024);
    if(pool) 
    {
	msg=AllocVecPooled(pool, sizeof(struct JUAE_Launch_Message));
	msg->mempool=pool;
	if(msg) 
	{
	    msg->tags   =tag_list_clone(pool, argsTagList, WorkbenchBase);
	    msg->ln_Name=str_dup_vec(pool, name);

	    /* now ensure, that the port stays alive */
	    Forbid();
	    port=get_j_uae_port();
	    if(port) 
	    {
		D(bug("[WBLIB] %s: PutMsg %lx to J-UAE\n", __PRETTY_FUNCTION__, msg));
		PutMsg(port, (struct Message *) msg); /* one way */
	    }
	    else {
		/* argl. someone closed the port inbetween !? */
		D(bug("[WBLIB] %s: ERROR: someone closed the port inbetween !? \n", __PRETTY_FUNCTION__));
		DeletePool(pool);
	    }
	    Permit();
	}
    }
}
