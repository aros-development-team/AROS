 This file contains various notes about Windows-hosted port of AROS.

 1. COMPILING

 This port currently can only be compiled under Windows OS. In order to do this
you need:

a) Working Cygwin environment (Mingw's MSYS is expected to work too but it's not tested).
b) Netpbm package of course.
c) AROS-targetted crosscompiler. Cygwin version of it can be found on AROS Archives:
   http://archives.aros-exec.org/index.php?function=browse&cat=development/cross
   I compile it using gcc version 3.3.1, however i beleive more recent versions will
   work too.
d) Mingw32 libraries package for Cygwin.

 That's all. Execute "./configure --target=mingw32-i386", than make.

 2. RUNNING

 In order to run AROS open a command line, go to root AROS directory ("AROS"), and run
"boot\AROSBootstrap.exe". This port behaves like UNIX-hosted, it uses emul.handler, which
makes your current directory to be root of your SYS:. Default kernel file expected by the
bootstrap is "boot\kernel". You can explicitly specify another kernel to boot at bootstrap's
command line.
 Currently bootstrap allocates a fixed 100MB chunk of memory for use by AROS. In future you'll
be able to change this, i'm planning also managed memory support (like on Linux).

 3. HACKING

 Here i'll describe how AROS should interact with host OS (Windows in our case). This is very tricky,
however it all ends up in two simple rules when you get the whole picture.

 3.1. Two simple rules.

 1. If you need to call some WinAPI functions, enclose the call (or several calls if you do them in a batch)
in Forbid()/Permit() pair. Otherwise you may get random sudden aborts (AROS just quits without any message).
 2. Do not call any operations that would block (I/O to serial/parallel ports for example). From the Windows'
point of view the whole AROS is one thread. If you block it, the whole AROS gets non-responsive until it's
unblocked. No task scheduling will occur at all.
 
 3.2. Performing wait operations in Windows.

 Often it's needed to wait for some event on Windows side. A good example of this is handling keyboard and mouse.
You have to sit and wait until some message arrives. Another example could be serial port I/O (which can take rather
long time). Such things are implemented using virtual hardware (VH). A VH unit is represented by asynchronous Windows
thread that you create. You can use anything in order to talk to this thread. The simplest and most efficient way
to do it is having some structure in memory where you put the data and then signal to your VH thread to start working
somehow (for example by triggering an event or posting a message to its queue). While your thread works, AROS works too.
When your VH thread finishes its job, it should call KrnCauseIRQ() function from kernel_native.dll. This causes an IRQ
on AROS side and AROS can read back the data from your structure.
 As you can see, this works exactly in the same way as real hardware.
 Note that thread switching in Windows seems to be rather slow, so avoid using VH threads if possible (for example you
can use overlapped I/O with serial port, where Windows itself will play a role of VH thread, because in this case you
may directly specify an event to signal when the I/O completes). However corresponsind part in kernel_native.dll is not
designed yet (you can't get pointer to IRQ event object), but this will change as soon as someone needs it.
 Look at wingdi.hidd source code for a good example of VH implementation.

 3.3.In-depth description of AROS task scheduler implementation.

 As you can see, Windows-hosted AROS acts very much like AROS on real hardware. In fact it's little simple virtual machine.
There are two Windows threads. One thread (let's call it "main thread") runs all AROS processes. In order to perform task
switching there is a second Windows thread, representing a "CPU" (let's call it VCPU - virtual CPU). It sits in a loop
waiting for several objects (these objects are emulated hardware IRQs). The first of them is a periodic 50Hz waitable timer,
it's the heart of the VM. Its IRQ is responsible for generating VBLANK interrupt in exec and counting quantums (this is also
done in exec.library on AROS side). when any IRQ happens, VCPU stops main thread and remembers its CPU context. It's a supervisor
mode, from this thread AROS interrupt handlers are called. After all processing it looks if task switching should be done.
If yes, VCPU substitutes the context of main thread with the context of next scheduled AROS process. Then it resumes main thread.
 As a result, single Windowsthread runs all AROS processes in turn (actually this thread represents user mode of virtual CPU).
 This is why it's necessary to enclose WinAPI calls in Forbid()/Permit() pair. Windows probably attempts to protect itself from
hackers this way and keeps an eye on thread's context (mostly stack) during execution of a system call. If task switch occurs
while main thread is inside a sycall (it's actually paused by Windows in this state), context swap happens. The syscall notices
that the stack changes and silently aborts the whole process. As a result, AROS just silently quits without any error messages.
 Task switcher can be also run manually by RaiseException() WinAPI call. This happens inside exec's Cause(), and is responsible
for causing SoftInts.
 All other objects which VCPU waits on are simple triggerable events. They are triggered by KrnCauseIRQ() and are used by various
drivers in order to communicate with VH threads. Currently their allocation is static, but dynamic allocation is really needed and
will be implemented in future.
 For more details see kernel.resource, i hope the code is clear enough.

 26.02.2009, Pavel Fedin <sonic.amiga@gmail.com>