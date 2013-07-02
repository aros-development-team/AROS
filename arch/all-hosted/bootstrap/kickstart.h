#ifdef _WIN64
#define __aros __attribute__((sysv_abi))
#else
#define __aros
#endif

int kick(int __aros (*addr)(), struct TagItem *msg);

