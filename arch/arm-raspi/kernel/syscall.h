#ifndef SYSCALL_H_
#define SYSCALL_H_

#define SC_CAUSE        0x000
#define SC_DISPATCH     0x001
#define SC_SWITCH       0x002
#define SC_SCHEDULE     0x003
#define SC_CLI          0x004
#define SC_STI          0x005
#define SC_SUPERSTATE   0x006
#define SC_ISSUPERSTATE 0x007
#define SC_INVALIDATED  0x008
#define SC_RTAS			0x009
#define SC_REBOOT		 0x100

#endif /*SYSCALL_H_*/
