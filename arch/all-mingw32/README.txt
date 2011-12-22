 This file contains various notes about Windows-hosted port of AROS.
 At the moment both x86 (i386 and x86_64) processors are supported. ARM support
 is in very early experimental state, and is likely to be postponed until Windows 8
 release. Windows CE lacks some capabilities necessary to run hosted AROS, and it is
 unclear if a workaround can be found.

 1. COMPILING

 In order to compile it natively under Windows OS you need:

a) Working Cygwin or MinGW + MSYS environment.
b) Netpbm package of course.
c) Native gcc v3 (for Cygwin). In gcc v4 -mno-cygwin option is no more supported. If you use MSYS
   you're free from this restriction and can use the latest gcc.
d) AROS-targetted crosscompiler. It can be found on AROS Archives:
   http://archives.aros-exec.org/index.php?function=browse&cat=development/cross
e) Mingw32 libraries package (only for Cygwin).
f) libiconv plus development files (only for MinGW - for building makecountry).

 That's all. Execute "./configure --target=mingw32-<your_cpu>", then "make".

 You can also crosscompile it under other OS. The only restriction (implied by configure script,
needs to be fixed): build system CPU should be different from i386. Otherwise configure suggests native build
and attempts to use host's gcc as a kernel compiler (one that builds a bootstrap and helper DLLs). Of course
it won't work. For cross-compiling you need the same toolset as for native build, plus Mingw32-targetted
crosscompiler. Currently cross-build is succesfully performed on Linux/x86-64 using native gcc v4.1.2 and
both crosscompilers v4.2.2 (built from vanilla source tree).

 2. RUNNING

 In order to run AROS open a command line, go to root AROS directory ("AROS"), and run
"boot\AROSBootstrap.exe". This port behaves like UNIX-hosted, it uses emul.handler, which
makes your current directory to be root of your SYS:.
 You can specify some options on the command line for bootstrap and AROS. Enter
"boot\AROSBootstrap.exe -h" to learn about them. --hostmem option currently does nothing.

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
thread that you create when you start up the driver (this is an equivalent of hardware discovery on a real machine).
You can use anything in order to talk to this thread. The simplest and most efficient way to do it is having some
structure in memory where you put the data and then signal to your VH thread to start working somehow (for example by
triggering an event or posting a message to its queue). While your thread works, AROS works too. When your VH thread
finishes its job, it should call KrnCauseIRQ() function from kernel_native.dll. This causes an IRQ on AROS side and
AROS can read back the data from your structure.
 As you can see, this works exactly in the same way as real hardware.
 Note that thread switching in Windows seems to be rather slow, so avoid using VH threads if possible (for example you
can use overlapped I/O with serial port, where Windows itself will play a role of VH thread, because in this case you
may directly specify an event to signal when the I/O completes).
 Look at wingdi.hidd source code for a good example of VH implementation.

 3.3. In-depth description of AROS task scheduler implementation.

 As you can see, Windows-hosted AROS acts very much like AROS on real hardware. In fact it's little simple virtual machine.
There are two Windows threads. One thread (let's call it "main thread") runs all AROS processes. In order to perform task
switching there is a second Windows thread, representing a "CPU" (let's call it VCPU - virtual CPU). It sits in a loop
waiting for several objects (these objects are emulated hardware IRQs). The first of them is a periodic 50Hz waitable timer,
it's the heart of the virtual machine. Its IRQ is responsible for generating VBLANK interrupt in exec and counting quantums
(this is also done in exec.library on AROS side). when any IRQ happens, VCPU stops main thread and remembers its CPU context.
It's a supervisor mode, from this thread AROS interrupt handlers are called. After all processing it looks if task switching
should be done. If yes, VCPU substitutes the context of main thread with the context of next scheduled AROS process. Then it
resumes main thread.
 As a result, single Windows thread runs all AROS processes in turn (actually this thread represents user mode of virtual CPU).
 This is why it's necessary to enclose WinAPI calls in Forbid()/Permit() pair. Windows probably attempts to protect itself from
hackers this way and keeps an eye on thread's context (mostly stack) during execution of a system call. If task switch occurs
while main thread is inside a syscall (it's actually paused by Windows in this state), context swap happens. The syscall notices
that the stack changes and silently aborts the whole process. As a result, AROS just silently quits without any error messages.
 Task switcher can be also run manually by RaiseException() WinAPI call, which has its reflection as krnSysCall() macro on
AROS side. This is used by KrnSwitch(), KrnSchedule(), KrnDispatch() and KrnCause() functions.
 All other objects which VCPU waits on are simple triggerable events. They are managed dynamically by drivers using a special
Windows-side API:

 KrnAllocIRQ()     - Allocate an IRQ for the use with the driver.
 KrnFreeIRQ()      - Free an allocated IRQ (should be used when the driver releases resources and exits).
 KrnCauseIRQ()     - Cause an IRQ on AROS side.
 KrnGetIRQObject() - Obtain a HANDLE of the IRQ object. For an IRQ allocated by KrnAllocIRQ() it will be a simple triggerable
                     event. You may use this handle for example for overlapped I/O operations.
 
  These are not AROS functions! They are legal to call only from within hardware emulation threads.
  
  For more details see kernel.resource, i hope the code is clear enough.

 20.09.2010, Pavel Fedin <pavel_fedin@mail.ru>