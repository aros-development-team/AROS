#define STACK_SIZE 8192

struct BootData
{
    APTR bd_BootMem;
};

void core_kick(struct TagItem *bootMsg, void *target);
