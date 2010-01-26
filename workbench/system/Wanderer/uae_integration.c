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
#define DEBUG 0
#include <aros/debug.h>
#endif

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
	D(bug("[Wanderer] %s: J-UAE port found\n", __PRETTY_FUNCTION__, filename));
	return TRUE;
    }

    D(bug("[Wanderer] %s: J-UAE port *not* found\n", __PRETTY_FUNCTION__, filename));
    return FALSE;
}

/**************************************************************************
 * is_68k(filename with path)
 *
 * test, if the supplied file is an AmigaOS 68k binary
 *
 * (Amiga 68k executables start with 0x00 0x00 0x03 0xf3)
 **************************************************************************/
BOOL is_68k(STRPTR filename) 
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
		D(bug("[Wanderer] %s: Amiga 68k executable found: %s\n", __PRETTY_FUNCTION__, filename));
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
static ULONG tag_list_nr(struct TagItem *tstate) 
{
    ULONG count;

    count=0;
    while(NextTagItem(&tstate)) 
    {
     	count++;
    }

    D(bug("[Wanderer] %s: TagList has %d items.\n", __PRETTY_FUNCTION__, count));
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
struct TagItem *tag_list_clone(void *mempool, struct TagItem *in) 
{

    ULONG   nr;
    ULONG   t;
    struct TagItem *result;
    char   *name_from_lock;
    struct TagItem *tag;

    nr=tag_list_nr(in);

    if(!nr) 
    {
     	return NULL;
    }

    /* don't ask me, if all that stuff can work on 64-Bit AROS, sorry */

    /* local only, no pool required */
    name_from_lock=(char *) AllocVec(LOCK_NAME_BUFFER_SIZE, MEMF_CLEAR); 
    if(!name_from_lock) 
    {
     	D(bug("[Wanderer] %s: ERROR: AllocVec failed!\n", __PRETTY_FUNCTION__));
      	return NULL;
    }

    result=(struct TagItem *) AllocVecPooled(mempool, (nr+1) * sizeof(struct TagItem));
    if(!result) 
    {
     	D(bug("[Wanderer] %s: ERROR: AllocVecPooled failed!\n", __PRETTY_FUNCTION__));
      	FreeVec(name_from_lock);
      return NULL;
    }

    t=0;
    while( (tag=NextTagItem(&in)) ) 
    {
	switch(tag->ti_Tag) {

	  case WBOPENA_ArgLock:
	      NameFromLock((BPTR) tag->ti_Data, name_from_lock, LOCK_NAME_BUFFER_SIZE);
	      UnLock((BPTR) tag->ti_Data); /* do we have to UnLock it? */
	      result[t].ti_Tag =WBOPENA_ArgLock;
	      result[t].ti_Data=(STACKIPTR) str_dup_vec(mempool, name_from_lock);
	      D(bug("[Wanderer] %s: WBOPENA_ArgLock: %s\n", __PRETTY_FUNCTION__, name_from_lock));
	      t++;
	  break;

	  case WBOPENA_ArgName:
	      result[t].ti_Tag =WBOPENA_ArgName;
	      result[t].ti_Data=(STACKIPTR) str_dup_vec(mempool, (char *) tag->ti_Data);
	      D(bug("[Wanderer] %s: WBOPENA_ArgName: %s\n", __PRETTY_FUNCTION__, tag->ti_Data));
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
 * forward_to_uae(struct TagItem *argsTagList, struct IconList_Entry *ent)
 *
 * Sends a message to the J-UAE port in order to launch the
 * according m68k amigaOS executeable.
 *
 * Errorhandling is completely up to J-UAE!
 * J-UAE has to free all message resources!
 **************************************************************************/
void forward_to_uae(struct TagItem *argsTagList, struct IconList_Entry *ent) 
{

    struct MsgPort              *port;
    struct JUAE_Launch_Message  *msg;
    void                        *pool;

    D(bug("[Wanderer] %s: j_uae_running and is_68k!\n", __PRETTY_FUNCTION__));

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
	    msg->tags   =tag_list_clone(pool, argsTagList);
	    msg->ln_Name=str_dup_vec(pool, ent->ile_IconEntry->ie_IconNode.ln_Name);

	    /* now ensure, that the port stays alive */
	    Forbid();
	    port=get_j_uae_port();
	    if(port) 
	    {
		D(bug("[Wanderer] %s: PutMsg %lx to J-UAE\n", __PRETTY_FUNCTION__, msg));
		PutMsg(port, msg); /* one way */
	    }
	    else {
		/* argl. someone closed the port inbetween !? */
		D(bug("[Wanderer] %s: ERROR: someone closed the port inbetween !? \n", __PRETTY_FUNCTION__));
		DeletePort(pool);
	    }
	    Permit();
	}
    }
}
