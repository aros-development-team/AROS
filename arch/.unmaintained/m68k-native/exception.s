#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#    $Log$
#    Revision 1.3  1996/10/21 21:08:57  aros
#    Changed __AROS_LA to __AROS_LHA
#
#    Revision 1.2  1996/08/01 17:41:35  digulla
#    Added standard header for all files
#
#    Desc:
#    Lang:

#*****************************************************************************
#
#   NAME
#	__AROS_LH0(void, Exception,
#
#   LOCATION
#	struct ExecBase *, SysBase, 8, Exec)
#
#   FUNCTION
#	Exception handler. This function is called by the dispatcher if a
#	exception has to be delivered. It is called in Disable()d state
#	(atomically) so that all Signals are still unchanged.
#	TF_EXCEPT is still set and must be reset by this routine.
#
#   INPUTS
#
#   RESULT
#
#   NOTES
#	This function has a context on its own and doesn't need to preserve
#	any registers.
#
#	Internal exec function.
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

	Disable     =	-0x78
	Enable	    =	-0x7e
	ThisTask    =	0x114
	IDNestCnt   =	0x126
	tc_Flags    =	0xe
	tc_SigRecvd =	0x1a
	tc_SigExcept=	0x1e
	tc_ExceptData=	0x26
	tc_ExceptCode=	0x2a
	TB_EXCEPT   =	5

	.globl	_Exec_Exception
_Exec_Exception:
	# First clear task exception bit.
	movel	a6@(ThisTask),a2
	bclr	#TB_EXCEPT,a2@(tc_Flags)

	# If the exception is raised out of a Wait()
	# IDNestCnt may be almost everything.
	# Store nesting level and set it to a
	# defined value 1 beyond -1.
excusr: moveb	a6@(IDNestCnt),d2
	clrb	a6@(IDNestCnt)

exloop: # get signals causing the exception
	# (do nothing if there are none)
	movel	a2@(tc_SigExcept),d0
	andl	a2@(tc_SigRecvd),d0
	jeq	excend

	# disable the signals
	eorl	d0,a2@(tc_SigExcept)
	eorl	d0,a2@(tc_SigRecvd)

	# call the exception vector with interrupts enabled
	movel	a2@(tc_ExceptData),a1
	movel	a2@(tc_ExceptCode),a0
	jsr	a6@(Enable)
	jsr	a0@
	jsr	a6@(Disable)

	# reenable signals and look again
	orl	d0,a2@(tc_SigExcept)
	jra	exloop

	# restore state of Disable() and return
excend: moveb	d2,a6@(IDNestCnt)
	rts

