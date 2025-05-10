#include "genmodule.h"

void generate_argtype_name_part(FILE *out, int argtype, int consecutive_args)
{
    if (argtype == TYPE_DOUBLE) {
        fprintf(out, "DOUBLE%d", consecutive_args);
    } else if (argtype == TYPE_QUAD) {
        fprintf(out, "QUAD%d", consecutive_args);
    } else {
        // This will look odd if normal args are last. Don't put them last!
        fprintf(out, "%d", consecutive_args);
    }
}
