/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

/* I have to put his in its own file because of include file
conflicts between AROS includes and system includes */


/* NOTE !!! All these functions need to be
  singlethreded by the LOCK_X11/UNLOCK_X11 macros form the outside
*/

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdlib.h>
#include <string.h>
#include "xshm.h"
#include <aros/debug.h>

/****************************************************************************************/

/* stegerg: maybe more safe, even if Unix malloc is used and not AROS malloc */
#define NO_MALLOC 1

/****************************************************************************************/

#if NO_MALLOC
#include <exec/memory.h>
#include <proto/exec.h>
#endif

/****************************************************************************************/

static void dummy_func()
{
	return;
}

/****************************************************************************************/

#if USE_XSHM

/****************************************************************************************/

#include <sys/shm.h>
#include <sys/ipc.h>
#include <X11/extensions/XShm.h>

/****************************************************************************************/

void *init_shared_mem(Display *display)
{
    #warning "Also check if this is a local display"
    
    XShmSegmentInfo *shminfo;
    int     	     xshm_major, xshm_minor;
    Bool    	     xshm_pixmaps;
    
    if (XShmQueryVersion(display, &xshm_major, &xshm_minor, &xshm_pixmaps))
    {
	#if NO_MALLOC
	shminfo = (XShmSegmentInfo *)AllocVec(sizeof(*shminfo), MEMF_PUBLIC);
	#else
	shminfo = (XShmSegmentInfo *)malloc(sizeof(*shminfo));
	#endif
	
	if (NULL != shminfo)
	{
	    key_t key;

	    /*
	     *	Try and get a key for us to use. The idea is to use a
	     *	filename that isn't likely to change all that often. This
	     *	is made somewhat easier since we must be run from the AROS
	     *	root directory (atm). So, I shall choose the path "C",
	     *	since the inode number isn't likely to change all that
	     *	often.
	     */
	    key = ftok("./C", 'A');
	    if(key == -1)
	    {
		kprintf("Hmm, path \"./C\" doesn't seem to exist?\n");
		key = IPC_PRIVATE;
	    }
	    else
	    {
		kprintf("Using shared memory key %d\n", key);
	    }

	    memset(shminfo, 0, sizeof (*shminfo));
		
	    /* Allocate shared memory */
	    shminfo->shmid = shmget(key,XSHM_MEMSIZE, IPC_CREAT|0777);
			
	    if (shminfo->shmid >= 0)
	    {
		/* Attach the mem to our process */
		shminfo->shmaddr = shmat(shminfo->shmid, NULL, 0);
		if (NULL != shminfo->shmaddr)
		{
		    shminfo->readOnly = False;
		    if (XShmAttach(display, shminfo))
		    {
		    	return shminfo;
			
		    }
		    shmdt(shminfo->shmaddr);
		    
		}
		shmctl(shminfo->shmid, IPC_RMID, NULL);
	    }
	#if NO_MALLOC
	    FreeVec(shminfo);
	#else	    
	    free(shminfo);
	#endif
	    
	}
    
    }	/* If has XShm extension */
    
    return NULL;
    
}

/****************************************************************************************/

void cleanup_shared_mem(Display *display, void *meminfo)
{
    XShmSegmentInfo *shminfo = (XShmSegmentInfo *)meminfo;
    
    if (NULL == meminfo)
    	return;
    
    XShmDetach(display, shminfo);
    shmdt(shminfo->shmaddr);
    shmctl(shminfo->shmid, IPC_RMID, 0);
    
#if NO_MALLOC
    FreeVec(shminfo);
#else
    free(shminfo);
#endif

}

/****************************************************************************************/

XImage *create_xshm_ximage(Display *display, Visual *visual, int depth, int format,
    	    	    	   int width, int height, void *xshminfo)	
{
    XShmSegmentInfo *shminfo;
    XImage  	    *image;
    
    shminfo = (XShmSegmentInfo *)xshminfo;
    
    image = XShmCreateImage(display, visual, depth, format, shminfo->shmaddr,
    	    	    	    shminfo, width, height);
    
    return image;
}
	
/****************************************************************************************/

void put_xshm_ximage(Display *display, Drawable d, GC gc, XImage *image,
    	    	     int xsrc,  int ysrc, int xdest, int ydest,
		     int width, int height, Bool send_event)
{
    XShmPutImage(display, d, gc, image, xsrc, ysrc, xdest, ydest,
    	    	 width, height, send_event);

    XSync(display, False);	

}

/****************************************************************************************/

void get_xshm_ximage(Display *display, Drawable d, XImage *image, int x, int y)
{
    XSync(display, False);
    XShmGetImage(display, d, image, x, y, AllPlanes);
}

/****************************************************************************************/

void destroy_xshm_ximage(XImage *image)
{
    XDestroyImage(image);
}

/****************************************************************************************/

#endif /* USE_XSHM */

/****************************************************************************************/
