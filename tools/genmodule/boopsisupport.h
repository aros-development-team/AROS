#include "config.h"
#include "functionhead.h"

#include <stdio.h>

void writeboopsiincludes(FILE *out);
void writeclassinit(struct config *cfg, FILE *out, struct classinfo *);
void writeboopsidispatcher(struct config *cfg, FILE *out, struct classinfo *);
