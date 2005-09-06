#include "config.h"
#include "functionhead.h"

#include <stdio.h>

void writeboopsiincludes(FILE *out);
void writeclassinit(FILE *out, struct classinfo *);
void writeboopsidispatcher(FILE *out, struct classinfo *);
