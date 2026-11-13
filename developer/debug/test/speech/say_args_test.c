/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */

#include "SayArgs.h"

#include <stdio.h>
#include <string.h>

static int Parse(int argc, char **argv, struct SayArguments *arguments,
                 const char **words)
{
    char error[128];

    SayInitArguments(arguments, words, 16);
    return SayParseArguments(argc, argv, arguments, error, sizeof(error));
}

static int CheckClassic(void)
{
    char *argv[] = { "Say", "-fs150", "-p", "120", "hello", "world" };
    const char *words[16];
    struct SayArguments arguments;

    if (!Parse(6, argv, &arguments, words))
        return 0;
    return arguments.sex == 1 && arguments.have_rate &&
           arguments.rate == 150 && arguments.have_pitch &&
           arguments.pitch == 120 && arguments.word_count == 2 &&
           strcmp(words[0], "hello") == 0 && strcmp(words[1], "world") == 0;
}

static int CheckSeparatedNumber(void)
{
    char *argv[] = { "Say", "-s", "150", "hello" };
    const char *words[16];
    struct SayArguments arguments;

    if (!Parse(4, argv, &arguments, words))
        return 0;
    return arguments.rate == 150 && arguments.word_count == 1 &&
           strcmp(words[0], "hello") == 0;
}

static int CheckLongOptions(void)
{
    char *argv[] = { "Say", "--sample-rate=11025", "--volume", "32",
                     "--phoneme-input", "--wav=test.wav", "HEHLOW" };
    const char *words[16];
    struct SayArguments arguments;

    if (!Parse(7, argv, &arguments, words))
        return 0;
    return arguments.sample_frequency == 11025 && arguments.volume == 32 &&
           arguments.phoneme_input &&
           strcmp(arguments.wav_path, "test.wav") == 0 &&
           arguments.word_count == 1;
}

static int CheckEndOfOptions(void)
{
    char *argv[] = { "Say", "--", "-s150" };
    const char *words[16];
    struct SayArguments arguments;

    if (!Parse(3, argv, &arguments, words))
        return 0;
    return !arguments.have_rate && arguments.word_count == 1 &&
           strcmp(words[0], "-s150") == 0;
}

static int CheckFailures(void)
{
    char *bad_number[] = { "Say", "-s", "fast", "hello" };
    char *mixed_input[] = { "Say", "-x", "text", "hello" };
    char *unknown[] = { "Say", "--engine", "new", "hello" };
    char *empty_value[] = { "Say", "--rate=", "150" };
    const char *words[16];
    struct SayArguments arguments;

    return !Parse(4, bad_number, &arguments, words) &&
           !Parse(4, mixed_input, &arguments, words) &&
           !Parse(4, unknown, &arguments, words) &&
           !Parse(3, empty_value, &arguments, words);
}

static int CheckTokenize(void)
{
    char *argv[40];
    char storage[128];
    char text[40];
    int argc;
    size_t length;

    if (!SayTokenize("please type what you want me to say", argv, 40,
                     storage, sizeof(storage), &argc) || argc != 9 ||
        strcmp(argv[8], "say") != 0)
        return 0;
    if (!SayTokenize("ab\tc =x --rate=150", argv, 40, storage,
                     sizeof(storage), &argc) || argc != 5 ||
        strcmp(argv[1], "ab") != 0 || strcmp(argv[2], "c") != 0 ||
        strcmp(argv[3], "=x") != 0 || strcmp(argv[4], "--rate=150") != 0)
        return 0;
    if (!SayTokenize("\"hello world\" \"\" \"a**b*\"c*n*e\"", argv, 40,
                     storage, sizeof(storage), &argc) || argc != 4 ||
        strcmp(argv[1], "hello world") != 0 || argv[2][0] != 0 ||
        strcmp(argv[3], "a*b\"c\n\033") != 0)
        return 0;
    if (!SayTokenize("\"a\"b", argv, 40, storage, sizeof(storage),
                     &argc) || argc != 3 || strcmp(argv[1], "a") != 0 ||
        strcmp(argv[2], "b") != 0)
        return 0;
    if (SayTokenize("\"unterminated", argv, 40, storage, sizeof(storage),
                    &argc) ||
        SayTokenize("\"dangling*", argv, 40, storage, sizeof(storage),
                    &argc) ||
        SayTokenize("ab", argv, 1, storage, sizeof(storage), &argc) ||
        SayTokenize("ab", argv, 40, storage, 2, &argc))
        return 0;
    if (!SayTokenize("ab", argv, 2, storage, 3, &argc) || argc != 2 ||
        !SayTokenize(NULL, argv, 2, storage, 1, &argc) || argc != 1)
        return 0;
    for (length = 1; length < sizeof(text); ++length)
    {
        memset(text, 'x', length);
        text[length] = 0;
        if (!SayTokenize(text, argv, 2, storage, sizeof(storage), &argc) ||
            argc != 2 || strlen(argv[1]) != length)
            return 0;
    }
    return 1;
}

int main(void)
{
    if (!CheckClassic() || !CheckSeparatedNumber() || !CheckLongOptions() ||
        !CheckEndOfOptions() || !CheckFailures() || !CheckTokenize())
    {
        fputs("Say argument tests failed\n", stderr);
        return 1;
    }
    puts("Say argument tests passed");
    return 0;
}
