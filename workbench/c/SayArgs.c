/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */

#include "SayArgs.h"

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

static int EqualNoCase(const char *left, const char *right)
{
    while (*left != 0 && *right != 0)
    {
        if (tolower((unsigned char)*left) != tolower((unsigned char)*right))
            return 0;
        ++left;
        ++right;
    }
    return *left == *right;
}

static int EqualNoCaseN(const char *left, const char *right, size_t length)
{
    while (length-- != 0)
    {
        if (*left == 0 || *right == 0 ||
            tolower((unsigned char)*left) !=
                tolower((unsigned char)*right))
            return 0;
        ++left;
        ++right;
    }
    return 1;
}

static void SetError(char *error, size_t error_size, const char *format,
                     const char *value)
{
    if (error != NULL && error_size != 0)
        snprintf(error, error_size, format, value);
}

int SayTokenize(const char *text, char **argv, size_t argv_capacity,
                char *storage, size_t storage_size, int *argc_out)
{
    size_t at = 0;
    size_t used = 0;
    size_t count = 1;

    if (argv == NULL || argv_capacity == 0 || storage == NULL ||
        storage_size == 0 || argc_out == NULL)
        return 0;
    if (text == NULL)
        text = "";
    argv[0] = "Say";
    while (text[at] != 0)
    {
        while (text[at] == ' ' || text[at] == '\t')
            ++at;
        if (text[at] == 0)
            break;
        if (count >= argv_capacity || count >= (size_t)INT_MAX)
            return 0;
        argv[count++] = storage + used;
        if (text[at] == '"')
        {
            ++at;
            for (;;)
            {
                unsigned char value = (unsigned char)text[at++];

                if (value == 0)
                    return 0;
                if (value == '"')
                    break;
                if (value == '*')
                {
                    value = (unsigned char)text[at++];
                    if (value == 0)
                        return 0;
                    if (value == 'n' || value == 'N')
                        value = '\n';
                    else if (value == 'e' || value == 'E')
                        value = 0x1b;
                }
                if (used + 1 >= storage_size)
                    return 0;
                storage[used++] = (char)value;
            }
        }
        else
        {
            while (text[at] != 0 && text[at] != ' ' && text[at] != '\t')
            {
                if (used + 1 >= storage_size)
                    return 0;
                storage[used++] = text[at++];
            }
        }
        if (used >= storage_size)
            return 0;
        storage[used++] = 0;
    }
    *argc_out = (int)count;
    return 1;
}

static int ParseNumber(const char *text, unsigned long *value)
{
    unsigned long number = 0;

    if (text == NULL || *text == 0)
        return 0;
    while (*text != 0)
    {
        unsigned digit;

        if (*text < '0' || *text > '9')
            return 0;
        digit = (unsigned)(*text++ - '0');
        if (number > (ULONG_MAX - digit) / 10)
            return 0;
        number = number * 10 + digit;
    }
    *value = number;
    return 1;
}

static const char *LongValue(const char *argument, const char *name)
{
    size_t length = strlen(name);

    if (strlen(argument) <= length || argument[length] != '=')
        return NULL;
    if (!EqualNoCaseN(argument, name, length))
        return NULL;
    return argument + length + 1;
}

static const char *OptionValue(int argc, char **argv, int *at,
                               const char *attached, const char *name,
                               char *error, size_t error_size)
{
    if (attached != NULL)
    {
        if (*attached == 0)
        {
            SetError(error, error_size, "%s requires a value", name);
            return NULL;
        }
        return attached;
    }
    if (*at + 1 >= argc)
    {
        SetError(error, error_size, "%s requires a value", name);
        return NULL;
    }
    ++*at;
    return argv[*at];
}

static int SetNumber(int argc, char **argv, int *at, const char *attached,
                     const char *name, unsigned long *destination,
                     char *error, size_t error_size)
{
    const char *value = OptionValue(argc, argv, at, attached, name,
                                    error, error_size);

    if (value == NULL)
        return 0;
    if (!ParseNumber(value, destination))
    {
        SetError(error, error_size, "%s requires an unsigned integer", name);
        return 0;
    }
    return 1;
}

static int ParseLongOption(int argc, char **argv, int *at,
                           struct SayArguments *arguments,
                           char *error, size_t error_size)
{
    const char *option = argv[*at];
    const char *value = NULL;

    if (EqualNoCase(option, "--help"))
        arguments->help = 1;
    else if (EqualNoCase(option, "--male"))
        arguments->sex = 0;
    else if (EqualNoCase(option, "--female"))
        arguments->sex = 1;
    else if (EqualNoCase(option, "--natural"))
        arguments->mode = 0;
    else if (EqualNoCase(option, "--robotic"))
        arguments->mode = 1;
    else if (EqualNoCase(option, "--phoneme-input"))
        arguments->phoneme_input = 1;
    else if (EqualNoCase(option, "--rate") ||
             (value = LongValue(option, "--rate")) != NULL)
    {
        if (!SetNumber(argc, argv, at, value, "--rate", &arguments->rate,
                       error, error_size))
            return 0;
        arguments->have_rate = 1;
    }
    else if (EqualNoCase(option, "--pitch") ||
             (value = LongValue(option, "--pitch")) != NULL)
    {
        if (!SetNumber(argc, argv, at, value, "--pitch", &arguments->pitch,
                       error, error_size))
            return 0;
        arguments->have_pitch = 1;
    }
    else if (EqualNoCase(option, "--sample-rate") ||
             (value = LongValue(option, "--sample-rate")) != NULL)
    {
        if (!SetNumber(argc, argv, at, value, "--sample-rate",
                       &arguments->sample_frequency, error, error_size))
            return 0;
        arguments->have_sample_frequency = 1;
    }
    else if (EqualNoCase(option, "--volume") ||
             (value = LongValue(option, "--volume")) != NULL)
    {
        if (!SetNumber(argc, argv, at, value, "--volume", &arguments->volume,
                       error, error_size))
            return 0;
        arguments->have_volume = 1;
    }
    else if (EqualNoCase(option, "--input") ||
             (value = LongValue(option, "--input")) != NULL)
    {
        value = OptionValue(argc, argv, at, value, "--input",
                            error, error_size);
        if (value == NULL)
            return 0;
        arguments->input_path = value;
    }
    else if (EqualNoCase(option, "--wav") ||
             (value = LongValue(option, "--wav")) != NULL)
    {
        value = OptionValue(argc, argv, at, value, "--wav",
                            error, error_size);
        if (value == NULL)
            return 0;
        arguments->wav_path = value;
    }
    else
    {
        SetError(error, error_size, "unknown option %s", option);
        return 0;
    }
    return 1;
}

static int ParseShortOption(int argc, char **argv, int *at,
                            struct SayArguments *arguments,
                            char *error, size_t error_size)
{
    const char *option = argv[*at];
    const char *p = option + 1;

    while (*p != 0)
    {
        int letter = tolower((unsigned char)*p++);

        if (letter == 'm')
            arguments->sex = 0;
        else if (letter == 'f')
            arguments->sex = 1;
        else if (letter == 'n')
            arguments->mode = 0;
        else if (letter == 'r')
            arguments->mode = 1;
        else if (letter == 'h')
            arguments->help = 1;
        else if (letter == 's' || letter == 'p')
        {
            unsigned long *destination = letter == 's' ? &arguments->rate
                                                        : &arguments->pitch;
            const char *name = letter == 's' ? "-s" : "-p";

            if (!SetNumber(argc, argv, at, *p != 0 ? p : NULL,
                           name, destination,
                           error, error_size))
                return 0;
            if (letter == 's')
                arguments->have_rate = 1;
            else
                arguments->have_pitch = 1;
            return 1;
        }
        else if (letter == 'x')
        {
            const char *value = OptionValue(argc, argv, at,
                                            *p != 0 ? p : NULL, "-x",
                                            error, error_size);
            if (value == NULL)
                return 0;
            arguments->input_path = value;
            return 1;
        }
        else
        {
            SetError(error, error_size, "unknown option %s", option);
            return 0;
        }
    }
    return 1;
}

void SayInitArguments(struct SayArguments *arguments, const char **words,
                      size_t word_capacity)
{
    memset(arguments, 0, sizeof(*arguments));
    arguments->sex = -1;
    arguments->mode = -1;
    arguments->words = words;
    arguments->word_capacity = word_capacity;
}

int SayParseArguments(int argc, char **argv, struct SayArguments *arguments,
                      char *error, size_t error_size)
{
    int options = 1;
    int at;

    arguments->word_count = 0;
    if (error != NULL && error_size != 0)
        error[0] = 0;

    for (at = 1; at < argc; ++at)
    {
        const char *argument = argv[at];

        if (options && strcmp(argument, "--") == 0)
        {
            options = 0;
            continue;
        }
        if (options && (strcmp(argument, "?") == 0 ||
                        strcmp(argument, "-?") == 0))
        {
            arguments->help = 1;
            continue;
        }
        if (options && argument[0] == '-' && argument[1] == '-')
        {
            if (!ParseLongOption(argc, argv, &at, arguments,
                                 error, error_size))
                return 0;
            continue;
        }
        if (options && argument[0] == '-' && argument[1] != 0)
        {
            if (!ParseShortOption(argc, argv, &at, arguments,
                                  error, error_size))
                return 0;
            continue;
        }
        if (arguments->word_count == arguments->word_capacity)
        {
            SetError(error, error_size, "%s", "too many text arguments");
            return 0;
        }
        arguments->words[arguments->word_count++] = argument;
        options = 0;
    }
    if (arguments->input_path != NULL && arguments->word_count != 0)
    {
        SetError(error, error_size, "%s",
                 "-x/--input cannot be combined with command text");
        return 0;
    }
    return 1;
}
