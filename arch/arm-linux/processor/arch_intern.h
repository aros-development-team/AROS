#define BUFFER_SIZE 256

struct LinuxArmProcessor
{
    ULONG	Implementer;
    ULONG	Arch;
    ULONG	Part;
    ULONG	Version;
    ULONG	Features;
    const char *Model;
};

/* Some of feature flags */
#define FF_VFP	 0x0001
#define FF_VFPv3 0x0002
#define FF_NEON	 0x0004

struct LibCInterface
{
    void *(*fopen)(const char *name, const char *mode);
    int   (*fclose)(void *file);
    char *(*fgets)(char *s, int n, void *stream);
};
