extern struct ELFNode *FirstELF;

int AddKernelFile(char *name);
void FreeKernelList(void);

void *open_file(struct ELFNode *n);
void close_file(void *file);
int read_block(void *file, unsigned long offset, void *dest, unsigned long length);
void *load_block(void *file, unsigned long offset, unsigned long length);
void free_block(void *addr);
