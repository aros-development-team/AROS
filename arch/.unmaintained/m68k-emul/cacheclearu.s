#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#    $Log$
#    Revision 1.1  1996/12/05 15:30:58  aros
#    Patches by Geert Uytterhoeven integrated
#
#    Revision 1.6  1996/10/24 15:51:09  aros
#    Use the official AROS macros over the __AROS versions.
#
#    Revision 1.5  1996/09/11 16:54:25  digulla
#    Always use __AROS_SLIB_ENTRY() to access shared external symbols, because
#    	some systems name an external symbol "x" as "_x" and others as "x".
#    	(The problem arises with assembler symbols which might differ)
#
#    Revision 1.4  1996/08/23 16:49:20	digulla
#    With some systems, .align 16 aligns to 64K instead of 16bytes. Therefore
#	I replaced it with .balign which does what we want.
#
#    Revision 1.3  1996/08/13 14:03:17	digulla
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
#	AROS_LH0(void, CacheClearU,
#
#   LOCATION
#	struct ExecBase *, SysBase, 106, Exec)
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

# This really should use the Linux/m68k `cacheflush' system call!

	.text
	.balign 16
	.globl	_Exec_CacheClearU
	.type	_Exec_CacheClearU,@function
_Exec_CacheClearU:
	/* Dummy */
	rts

