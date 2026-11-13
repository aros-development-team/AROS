/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */

#ifndef SAYARGS_H
#define SAYARGS_H

#include <stddef.h>

struct SayArguments
{
    const char *input_path;
    const char *wav_path;
    unsigned long rate;
    unsigned long pitch;
    unsigned long sample_frequency;
    unsigned long volume;
    int have_rate;
    int have_pitch;
    int have_sample_frequency;
    int have_volume;
    int sex;
    int mode;
    int phoneme_input;
    int help;
    const char **words;
    size_t word_count;
    size_t word_capacity;
};

void SayInitArguments(struct SayArguments *arguments, const char **words,
                      size_t word_capacity);

int SayTokenize(const char *text, char **argv, size_t argv_capacity,
                char *storage, size_t storage_size, int *argc_out);

int SayParseArguments(int argc, char **argv, struct SayArguments *arguments,
                      char *error, size_t error_size);

#endif
