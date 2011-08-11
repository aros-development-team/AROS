#define BUFFER_SIZE 1024

extern char *KernelArgs;
extern char *SystemVersion;
char buf[BUFFER_SIZE];

int bootstrap(int argc, char ** argv);
