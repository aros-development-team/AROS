/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <dos/dos.h>
#include <proto/dos.h>

#include <errno.h>
#include <sys/types.h>

#include "__errno.h"
#include "__upath.h"

ULONG prot_u2a(mode_t protect);

/*****************************************************************************

    NAME */
#include <sys/stat.h>

	int chmod(

/*  SYNOPSIS */			
	const char *path, 
	mode_t mode)

/*  FUNCTION
	Change permission bits of a specified file.

    INPUTS
	path - path to the file
	mode - permission bits
    
    RESULT
    	Permission bits of the file given by path are changed accordingly
    	to given mode.
    
    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    if (!path) /*safety check */
    {
	errno = EFAULT;
	return -1;
    }

    path = __path_u2a(path);
    if (path == NULL)
        return -1;

    if (!SetProtection(path, prot_u2a(mode)))
    {
    	errno = IoErr2errno(IoErr());
	return -1;
    }

    return 0;
}


/* taken from emul_handler */
ULONG prot_u2a(mode_t protect)
{
    ULONG aprot = 0;

    /* The following three (AROS) flags are low-active! */
    if (!(protect & S_IRUSR))
	aprot |= FIBF_READ;
    if (!(protect & S_IWUSR))
	aprot |= FIBF_WRITE;
    if (!(protect & S_IXUSR))
	aprot |= FIBF_EXECUTE;

    /* The following flags are high-active again. */
    if ((protect & S_IRGRP))
	aprot |= FIBF_GRP_READ;
    if ((protect & S_IWGRP))
	aprot |= FIBF_GRP_WRITE;
    if ((protect & S_IXGRP))
	aprot |= FIBF_GRP_EXECUTE;
    if ((protect & S_IROTH))
	aprot |= FIBF_OTR_READ;
    if ((protect & S_IWOTH))
	aprot |= FIBF_OTR_WRITE;
    if ((protect & S_IXOTH))
	aprot |= FIBF_OTR_EXECUTE;

    if ((protect & S_ISVTX))
        aprot |= FIBF_SCRIPT;

    return aprot;
}
