#
#   Copyright © 2010-2011, The AROS Development Team. All rights reserved.
#   $Id$
#
#   Desc: Exit from emulated interrupt with enabling, x86-64 version
#   Lang: English
#

# Theory of operation:
# x86-64 has red zone of 128 bytes below the rsp. This makes it impossible to use
# push/pop instructions here, unlike on i386, because doing this will destroy
# red zone data.
# Here we skip the red zone and use -128(%rsp) as our intermediate storage. However,
# there's an important problem with this. x86 is not ARM, and we can't do something like
# "addq $128, %rsp; jmpq *-128(rsp)" atomically. This mean, we can be preempted right before
# the final jump. This is dangerous, because next time we will get back here with our own
# address in 0(%rax), overwriting -128(rsp) with it and causing infinite loop.
# In order to work around this issue, interrupt thread checks rip value, and if rip is
# pointing at this code, interrupts are considered disabled.
# We still have to use stack for temporary storage because we first need to restore rax
# and only then jump to return address.

        .globl  core_LeaveInterrupt
        .globl  core_LeaveInt_End

core_LeaveInterrupt:
        movq    %rbx, -128(%rsp)    # Save rbx
        movq    0(%rax), %rbx       # Get real return address into rbx
        xchg    %rbx, -128(%rsp)    # Remember return address and restore rbx
	movq    8(%rax), %rax	    # Restore real rax contents
	movl    $1, Ints_Enabled    # Now enable interrupts
	jmpq    *-128(%rsp)         # And jump to the needed address
core_LeaveInt_End:
