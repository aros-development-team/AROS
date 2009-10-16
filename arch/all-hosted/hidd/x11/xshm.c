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

#include <aros/symbolsets.h>

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

#include "x11_hostlib.h"

#include <proto/hostlib.h>

static void *xext_handle = NULL;
static void *shm_handle = NULL;

static struct {
    Status (*XShmDetach) ( Display* , XShmSegmentInfo* );
    Status (*XShmPutImage) ( Display* , Drawable , GC , XImage* , int , int , int , int , unsigned int , unsigned int , Bool );
    Status (*XShmGetImage) ( Display* , Drawable , XImage* , int , int , unsigned long );
    XImage * (*XShmCreateImage) ( Display* , Visual* , unsigned int , int , char* , XShmSegmentInfo* , unsigned int , unsigned int );
    Bool (*XShmQueryVersion) ( Display* , int* , int* , Bool* );
    Status (*XShmAttach) ( Display* , XShmSegmentInfo* );
} xext_func;

static const char *xext_func_names[] = {
    "XShmDetach",
    "XShmPutImage",
    "XShmGetImage",
    "XShmCreateImage",
    "XShmQueryVersion",
    "XShmAttach"
};

#define XEXT_SOFILE "libXext.so.6"

#define XEXTCALL(func,...) (xext_func.func(__VA_ARGS__))

extern void *x11_hostlib_load_so(const char *, const char **, int, void **);

static int xext_hostlib_init(void *libbase) {
    D(bug("[x11] xext hostlib init\n"));

    if ((xext_handle = x11_hostlib_load_so(XEXT_SOFILE, xext_func_names, 6, (void **) &xext_func)) == NULL)
        return FALSE;

    return TRUE;
}

static int xext_hostlib_expunge(void *libbase) {
    D(bug("[x11] xext hostlib expunge\n"));

    if (xext_handle != NULL)
        HostLib_Close(xext_handle, NULL);

    return TRUE;
}

ADD2INITLIB(xext_hostlib_init, 1)
ADD2EXPUNGELIB(xext_hostlib_expunge, 1)


/****************************************************************************************/

void *init_shared_mem(Display *display)
{
    #warning "Also check if this is a local display"
    
    XShmSegmentInfo *shminfo;
    int     	     xshm_major, xshm_minor;
    Bool    	     xshm_pixmaps;
    
    if (XEXTCALL(XShmQueryVersion, display, &xshm_major, &xshm_minor, &xshm_pixmaps))
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
    	#if 1
	    key = IPC_PRIVATE;
	#else
	    key = CCALL(ftok, "./C", 'A');
	    if(key == -1)
	    {
		kprintf("Hmm, path \"./C\" doesn't seem to exist?\n");
		key = IPC_PRIVATE;
	    }
	    else
	    {
		kprintf("Using shared memory key %d\n", key);
	    }
    	#endif
	    memset(shminfo, 0, sizeof (*shminfo));
		
	    /* Allocate shared memory */
	    shminfo->shmid = CCALL(shmget, key, XSHM_MEMSIZE, IPC_CREAT|0777);
			
	    if (shminfo->shmid >= 0)
	    {
		/* Attach the mem to our process */
		shminfo->shmaddr = CCALL(shmat, shminfo->shmid, NULL, 0);
		if (NULL != shminfo->shmaddr)
		{
		    shminfo->readOnly = False;
		    if (XEXTCALL(XShmAttach, display, shminfo))
		    {
		    	return shminfo;
			
		    }
		    CCALL(shmdt, shminfo->shmaddr);
		    
		}
		CCALL(shmctl, shminfo->shmid, IPC_RMID, NULL);
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
    
    XEXTCALL(XShmDetach, display, shminfo);
    CCALL(shmdt, shminfo->shmaddr);
    CCALL(shmctl, shminfo->shmid, IPC_RMID, 0);
    
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
    
    image = XEXTCALL(XShmCreateImage, display, visual, depth, format, shminfo->shmaddr,
    	    	    	              shminfo, width, height);
    
    return image;
}
	
/****************************************************************************************/

void put_xshm_ximage(Display *display, Drawable d, GC gc, XImage *image,
    	    	     int xsrc,  int ysrc, int xdest, int ydest,
		     int width, int height, Bool send_event)
{
    XEXTCALL(XShmPutImage, display, d, gc, image, xsrc, ysrc, xdest, ydest,
    	    	           width, height, send_event);
    XCALL(XSync, display, False);
}

/****************************************************************************************/

int get_xshm_ximage(Display *display, Drawable d, XImage *image, int x, int y)
{
    XCALL(XSync, display, False);
    XEXTCALL(XShmGetImage, display, d, image, x, y, AllPlanes);
    
    return (int)XEXTCALL(XShmGetImage, display, d, image, x, y, AllPlanes);
}

/****************************************************************************************/

void destroy_xshm_ximage(XImage *image)
{
    XDestroyImage(image);
}

/****************************************************************************************/

#endif /* USE_XSHM */

/****************************************************************************************/
