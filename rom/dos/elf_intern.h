#if (__WORDSIZE == 64)
#define WANT_CLASS ELFCLASS64
#else
#define WANT_CLASS ELFCLASS32
#endif

#if AROS_BIG_ENDIAN
#define WANT_BYTE_ORDER ELFDATA2MSB
#else
#define WANT_BYTE_ORDER ELFDATA2LSB
#endif

struct hunk
{
    ULONG size;
    BPTR  next;
    char  data[0];
} __attribute__((packed));

#define BPTR2HUNK(bptr) ((struct hunk *)((void *)bptr - offsetof(struct hunk, next)))
#define HUNK2BPTR(hunk) MKBADDR(&hunk->next)

#undef MyRead
#undef MyAlloc
#undef MyFree

#define MyRead(file, buf, size)      \
    AROS_CALL3                       \
    (                                \
        LONG, funcarray[0],          \
        AROS_LCA(BPTR,   file, D1),  \
        AROS_LCA(void *, buf,  D2),  \
        AROS_LCA(LONG,   size, D3),  \
        struct DosLibrary *, DOSBase \
    )


#define MyAlloc(size, flags)        \
    AROS_CALL2                      \
    (                               \
        void *, funcarray[1],       \
        AROS_LCA(ULONG, size,  D0), \
        AROS_LCA(ULONG, flags, D1), \
        struct ExecBase *, SysBase  \
    )				    


#define MyFree(addr, size)          \
    AROS_CALL2NR                    \
    (                               \
        void, funcarray[2],         \
        AROS_LCA(void *, addr, A1), \
        AROS_LCA(ULONG,  size, D0), \
        struct ExecBase *, SysBase  \
    )

int read_block(BPTR file, ULONG offset, APTR buffer, ULONG size, SIPTR *funcarray, struct DosLibrary *DOSBase);
void *load_block(BPTR file, ULONG offset, ULONG size, SIPTR *funcarray, struct DosLibrary *DOSBase);
int load_first_header(BPTR file, struct elfheader *eh, SIPTR *funcarray, ULONG *shnum, ULONG *shstrndx, struct DosLibrary *DOSBase);
int relocate(struct elfheader *eh, struct sheader *sh, ULONG shrel_idx, struct sheader *symtab_shndx, struct DosLibrary *DOSBase);
void register_elf(BPTR file, BPTR hunks, struct elfheader *eh, struct sheader *sh, struct DosLibrary *DOSBase);
