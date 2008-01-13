register void * global_data asm("r29");

#define EXPORT_FUNC(x) \
        asm volatile (                  \
"       .globl " #x "\n"                \
#x ":\n"                                \
"       lwz     %%r11, %0(%%r29)\n"     \
"       lwz     %%r11, %1(%%r11)\n"     \
"       mtctr   %%r11\n"                \
"       bctr\n"                         \
        : : "i"(offsetof(gd_t, jt)), "i"(XF_ ## x * sizeof(void *)) : "r11");

