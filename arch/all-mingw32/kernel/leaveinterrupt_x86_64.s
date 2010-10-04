        .globl  _core_LeaveInterrupt

_core_LeaveInterrupt:
	pushq 0(%rax)		# Push real return address
	pushq 8(%rax)		# Push real rax contents
	movl $1, _Ints_Enabled	# Now enable interrupts
	popq %rax		# Restore rax and leave
	ret
