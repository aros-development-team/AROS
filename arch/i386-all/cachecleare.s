#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#    $Log$
#    Revision 1.3  1996/08/13 14:03:17  digulla
#    Added standard headers
#
#    Revision 1.2  1996/08/01 17:41:06	digulla
#    Added standard header for all files
#
#    Desc:
#    Lang:

#*****************************************************************************
#
#   NAME
#	__AROS_LH3(void, CacheClearE,
#
#   SYNOPSIS
#	__AROS_LHA(APTR,  address, A0),
#	__AROS_LHA(ULONG, length,  D0),
#	__AROS_LHA(ULONG, caches,  D1),
#
#   LOCATION
#	struct ExecBase *, SysBase, 107, Exec)
#
#   FUNCTION
#
#   INPUTS
#
#   RESULT
#
#   NOTES
#
#   EXAMPLE
#
#   BUGS
#
#   SEE ALSO
#
#   INTERNALS
#
#   HISTORY
#
#******************************************************************************
	.text
	.align	16
	.globl	Exec_CacheClearE
	.type	Exec_CacheClearE,@function
Exec_CacheClearE:
	/* Dummy */
	ret

