/*
 * This file is used as startup code by ELF wrapper.
 * It's purpose is only to define _start symbol which is
 * the default entry point. This is needed in order to make
 * the compiler passing link tests in configure scripts.
 * It's not used by AROS code in any way.
 */

int _start(void)
{
    return -1;
}
