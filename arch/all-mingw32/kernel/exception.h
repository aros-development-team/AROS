struct EXCEPTION_REGISTRATION
{
    struct EXCEPTION_REGISTRATION *prev;
    void *handler;
};

#ifdef __i386__

#define ADD_EXCEPTION_FRAME(f)			     \
    asm volatile ("movl %%fs:0, %0" : "=r" (f.prev));\
    asm volatile ("movl %0, %%fs:0" : : "r" (&f))

#define REMOVE_EXCEPTION_FRAME(f) \
    asm volatile ("movl %0, %%fs:0" : : "r" (f.prev))

#else
#error Unsupported CPU
#endif

#define BEGIN_EXCEPTION(x)		    \
{					    \
    struct EXCEPTION_REGISTRATION _ex_frame;\
					    \
    _ex_frame.handler = x;		    \
    ADD_EXCEPTION_FRAME(_ex_frame)

#define END_EXCEPTION()		      \
    REMOVE_EXCEPTION_FRAME(_ex_frame);\
}
