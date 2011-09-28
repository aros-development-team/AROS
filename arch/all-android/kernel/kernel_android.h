#include <inttypes.h>

extern int alertPipe;

int SendAlert(uint32_t code, const char *text);
