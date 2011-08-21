#define STACK_SIZE 8192

struct BootData
{
    APTR bd_BootMem;
};

extern struct BootData *__BootData;

void core_kick(struct TagItem *bootMsg, void *target);
void intr_init(void);
