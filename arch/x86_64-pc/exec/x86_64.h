#ifndef X86_64_H_
#define X86_64_H_

#define IN_USER_MODE \
    ({  short __val; \
        __asm__ __volatile__("mov %%cs,%0":"=r"(__val)); \
        (__val & 0x03); })

#endif /*X86_64_H_*/
