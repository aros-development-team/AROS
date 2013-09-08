/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <dos/dos.h>
#include <proto/dos.h>

#include <errno.h>
#include <sys/types.h>

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
	path - Pathname of the file
	mode - Bit mask created by ORing zero or more of the following
	       permission bit masks:

	       S_ISUID - set user id on execution
	       S_ISGID - set group id on execution
	       S_ISVTX - sticky bit (restricted deletion flag)
	       S_IRUSR - allow owner to read
	       S_IWUSR - allow owner to write
	       S_IXUSR - allow owner to execute/search directory
	       S_IRGRP - allow group to read
	       S_IWGRP - allow group to write
	       S_IXGRP - allow group to execute/search directory
	       S_IROTH - allow others to read
	       S_IWOTH - allow others to write
	       S_IXOTH - allow others to execute/search directory
    
    RESULT
	0 on success and -1 on error. If an error occurred, the global
	variable errno is set.
    
    NOTES

    EXAMPLE

    BUGS
	S_ISUID and S_ISGID are silently ignored.

    SEE ALSO
	fchmod()

    INTERNALS
	Permission bit masks are converted to their respective dos.library
	counterparts:

	S_ISVTX to FIBF_SCRIPT
	!S_IRUSR to FIBF_READ
	!S_IWUSR to FIBF_WRITE
	!S_IXUSR to FIBF_EXECUTE
	S_IRGRP to FIBF_GRP_READ
	S_IWGRP to FIBF_GRP_WRITE
	S_IXGRP to FIBF_GRP_EXECUTE
	S_IROTH to FIBF_OTR_READ
	S_IWOTH to FIBF_OTR_WRITE
	S_IXOTH to FIBF_OTR_EXECUTE

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
    	errno = __arosc_ioerr2errno(IoErr());
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
