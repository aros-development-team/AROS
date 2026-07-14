/*
    Copyright (C) 1995-2014, The AROS Development Team. All rights reserved.
*/

/* In .text.moduleend so it links at the module tail: rt_EndSkip = &Alerthook_end
 * then marks the true module end for the romtag scanner's leap to the next module. */
__attribute__((section(".text.moduleend"))) void Alerthook_end(void) { }
