struct rt_node
{
    struct Node     node;
    struct Resident *module;
};

ULONG **krnRomTagScanner(struct ExecBase *SysBase, UWORD *ranges[]);
