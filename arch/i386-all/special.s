#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#    $Log$
#    Revision 1.3  1996/09/11 16:54:29  digulla
#    Always use __AROS_SLIB_ENTRY() to access shared external symbols, because
#    	some systems name an external symbol "x" as "_x" and others as "x".
#    	(The problem arises with assembler symbols which might differ)
#
#    Revision 1.2  1996/08/01 17:41:26	digulla
#    Added standard header for all files
#
#    Desc:
#    Lang:

	/* Never Called */
	.globl	_Exec_TrapHandler
_Exec_TrapHandler:
	ret
