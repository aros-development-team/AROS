#define LINELENGTH 256

extern int verbose;

extern int parsendkoffsets (char *offfile, char *sdkdir, char *gendir, char *bindir);
extern void printBanner(FILE *structfile, char *comment);
extern  void makefullpath(char *path);