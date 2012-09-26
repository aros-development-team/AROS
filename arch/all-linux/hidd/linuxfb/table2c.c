/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>

int main(int argc, char *argv[])
{
    unsigned char buf[256];
    FILE *in = NULL, *out = NULL;
    int i;
    int tabsize;
    int ret = 1;
    
    if (argc < 2) {
        fprintf(stderr, "Too few args\n");
        goto exit;
    }
    
    in = fopen(argv[1], "r");
    if (NULL == in) {
        fprintf(stderr, "Could not open input file %s\n", argv[1]);
        goto exit;
    }
    

    if (1 != fread(buf, 256, 1, in)) {
        fprintf(stderr, "Could not read from input file\n");
        goto exit;
    }
    
    tabsize = 0;
    for (i = 0; i < 256; i ++) {
        if (buf[i] != 0xFF) {
            tabsize = i + 1;
        }
    }
    
    printf("#define DEF_TAB_SIZE %d\n", tabsize);
    printf("const UBYTE deftable[] = {\n");
    printf("\t  0x%02x", buf[0]);
    for (i = 1; i < tabsize; i ++) {
        printf(", 0x%02x", buf[i]);

        if ((i + 1) % 10 == 0) {
            printf("\n\t");
        }
    }
    printf("\n};\n");
    
    
    ret = 0;
    
exit:

/*    if (NULL != in)
    fclose(in);

    if (NULL != out)
    fclose(out);
*/    
    return ret;
}
