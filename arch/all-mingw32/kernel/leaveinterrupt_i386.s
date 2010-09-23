        .globl  _core_LeaveInterrupt
        .type   _core_LeaveInterrupt,@function

# Note that we first get all the values from struct LeaveInterruptContext
# and only then enable interrupts. After enabling interrupts our structure
# can be reused by another process.
# We are already out of Windows exception and running on task's stack,
# so it's okay to play with stack here.

_core_LeaveInterrupt:
	pushl 0(%eax)		# Push real return address
	pushl 4(%eax)		# Push real eax contents
	movl $1, _Ints_Enabled	# Now enable interrupts
	popl %eax		# Restore eax and leave
	ret
