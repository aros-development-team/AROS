/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

	Desc: Generates .ctbl files from unicode data.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#define CHARSET_TABLE_SIZE 256
#define CTBL_MAGIC "CTBL"
#define CTBL_VERSION 1

#define FLAG_UPPER  0x0001
#define FLAG_LOWER  0x0002
#define FLAG_ALPHA  0x0004
#define FLAG_DIGIT  0x0008
#define FLAG_SPACE  0x0010
#define FLAG_PRINT  0x0020

struct CharsetTables {
    uint16_t classification[CHARSET_TABLE_SIZE];
    uint8_t to_upper[CHARSET_TABLE_SIZE];
    uint8_t to_lower[CHARSET_TABLE_SIZE];
};

static uint16_t classification_flags_from_category(const char *cat) {
    uint16_t flags = 0;

    if (strcmp(cat, "Lu") == 0) {
        flags |= FLAG_UPPER | FLAG_ALPHA;
    } else if (strcmp(cat, "Ll") == 0) {
        flags |= FLAG_LOWER | FLAG_ALPHA;
    } else if (strcmp(cat, "Nd") == 0) {
        flags |= FLAG_DIGIT;
    } else if (strcmp(cat, "Zs") == 0) {
        flags |= FLAG_SPACE;
    } else if (cat[0] == 'L') {
        flags |= FLAG_ALPHA;
    }

    if (cat[0] != 'C') {
        flags |= FLAG_PRINT;
    }

    return flags;
}

int parse_unicode_data_line(char *line,
    uint16_t *classification, uint8_t *to_upper, uint8_t *to_lower) {

    char *fields[15] = {0};
    int field_count = 0;

    char *p = line;
    while (field_count < 15 && p) {
        fields[field_count++] = p;
        char *semi = strchr(p, ';');
        if (!semi) break;
        *semi = '\0';
        p = semi + 1;
    }

    if (field_count < 3) return -1;

    unsigned int cp = 0;
    if (sscanf(fields[0], "%x", &cp) != 1) return -1;
    if (cp >= CHARSET_TABLE_SIZE) return 0;

    const char *category = fields[2];

    classification[cp] = classification_flags_from_category(category);

    if (field_count > 12 && fields[12][0] != '\0') {
        unsigned int up;
        if (sscanf(fields[12], "%x", &up) == 1 && up < 256) {
            to_upper[cp] = (uint8_t)up;
        } else {
            to_upper[cp] = (uint8_t)cp;
        }
    } else {
        to_upper[cp] = (uint8_t)cp;
    }

    if (field_count > 13 && fields[13][0] != '\0') {
        unsigned int low;
        if (sscanf(fields[13], "%x", &low) == 1 && low < 256) {
            to_lower[cp] = (uint8_t)low;
        } else {
            to_lower[cp] = (uint8_t)cp;
        }
    } else {
        to_lower[cp] = (uint8_t)cp;
    }

    return 0;
}

int write_ctbl_file(const char *path, struct CharsetTables *tables) {
    FILE *f = fopen(path, "wb");
    if (!f) {
        perror("fopen");
        return 1;
    }

    fwrite(CTBL_MAGIC, 1, 4, f);
    uint16_t version = CTBL_VERSION;
    uint16_t flags = 0;
    fwrite(&version, sizeof(version), 1, f);
    fwrite(&flags, sizeof(flags), 1, f);
    fwrite(tables->classification, sizeof(uint16_t), CHARSET_TABLE_SIZE, f);
    fwrite(tables->to_upper, sizeof(uint8_t), CHARSET_TABLE_SIZE, f);
    fwrite(tables->to_lower, sizeof(uint8_t), CHARSET_TABLE_SIZE, f);

    fclose(f);
    return 0;
}

int write_c_source_file(const char *path, const char *symbol_name, struct CharsetTables *tables) {
    FILE *f = fopen(path, "w");
    if (!f) {
        perror("fopen");
        return 1;
    }

    fprintf(f, "/*\n"
                "    Automatically generated classification and conversion tables for symbol: %s\n"
                "    Do not edit manually.\n"
                " */\n\n", symbol_name);

    fprintf(f, "#include <wctype.h>\n\n");
    fprintf(f, "#if __WCHAR_MAX__ > 255\n"
                "#define UNICODE_TABLE_SIZE 256\n"
                "#else\n"
                "#define UNICODE_TABLE_SIZE 128\n"
                "#endif\n\n");

    // to_upper table
    fprintf(f, "static const wchar_t unicode_l2u[UNICODE_TABLE_SIZE] = {\n");
    for (int i = 0; i < 127; ++i) {
        if (i % 8 == 0) fprintf(f, "    ");
        fprintf(f, "0x%02X, ", tables->to_upper[i]);
        if ((i + 1) % 8 == 0) fprintf(f, "\n");
    }
    fprintf(f,
        "\n#if __WCHAR_MAX__ < 256\n"
        "    0x%02X\n"
        "#else\n"
        "    0x%02X,\n", 
        tables->to_upper[127], tables->to_upper[127]);

    for (int i = 128; i < 256; ++i) {
        if ((i - 128) % 8 == 0) fprintf(f, "    ");
        fprintf(f, "0x%02X", tables->to_upper[i]);
        if (i != 255) fprintf(f, ", ");
        if ((i + 1) % 8 == 0 || i == 255) fprintf(f, "\n");
    }
    fprintf(f, "#endif\n};\n\n");

    // to_lower table
    fprintf(f, "static const wchar_t unicode_u2l[UNICODE_TABLE_SIZE] = {\n");
    for (int i = 0; i < 127; ++i) {
        if (i % 8 == 0) fprintf(f, "    ");
        fprintf(f, "0x%02X, ", tables->to_lower[i]);
        if ((i + 1) % 8 == 0) fprintf(f, "\n");
    }
    fprintf(f,
        "\n#if __WCHAR_MAX__ < 256\n"
        "    0x%02X\n"
        "#else\n"
        "    0x%02X,\n",
        tables->to_lower[127], tables->to_lower[127]);

    for (int i = 128; i < 256; ++i) {
        if ((i - 128) % 8 == 0) fprintf(f, "    ");
        fprintf(f, "0x%02X", tables->to_lower[i]);
        if (i != 255) fprintf(f, ", ");
        if ((i + 1) % 8 == 0 || i == 255) fprintf(f, "\n");
    }
    fprintf(f, "#endif\n};\n\n");

    // classification table
    fprintf(f, "static const wctype_t unicode_wctype[UNICODE_TABLE_SIZE] = {\n");
    for (int i = 0; i < 127; ++i) {
        if (i % 8 == 0) fprintf(f, "    ");
        fprintf(f, "0x%04X, ", tables->classification[i]);
        if ((i + 1) % 8 == 0) fprintf(f, "\n");
    }
    fprintf(f,
        "\n#if __WCHAR_MAX__ < 256\n"
        "    0x%04X\n"
        "#else\n"
        "    0x%04X,\n",
        tables->classification[127], tables->classification[127]);

    for (int i = 128; i < 256; ++i) {
        if ((i - 128) % 8 == 0) fprintf(f, "    ");
        fprintf(f, "0x%04X", tables->classification[i]);
        if (i != 255) fprintf(f, ", ");
        if ((i + 1) % 8 == 0 || i == 255) fprintf(f, "\n");
    }
    fprintf(f, "#endif\n};\n");

    fclose(f);
    return 0;
}

int main(int argc, char **argv) {
    int emit_c_output = 0, verbose = 0;
    if (argc == 5 && strcmp(argv[4], "--emit-c") == 0) {
        emit_c_output = 1;
    } else if (argc != 4) {
        fprintf(stderr, "Usage: %s <unicode-dir> <target-dir> <iso_locale> [--emit-c]\n", argv[0]);
        return 1;
    }

    const char *unicode_dir = argv[1];
    const char *target_dir = argv[2];
    const char *iso_locale = argv[3];

    // Build full path to UnicodeData.txt
    char unicode_data_path[512];
    snprintf(unicode_data_path, sizeof(unicode_data_path), "%s/UnicodeData.txt", unicode_dir);

    FILE *f = fopen(unicode_data_path, "r");
    if (!f) {
        perror("fopen UnicodeData.txt");
        return 1;
    }
    fclose(f);

    struct CharsetTables tables;
    for (int i = 0; i < CHARSET_TABLE_SIZE; i++) {
        tables.classification[i] = 0;
        tables.to_upper[i] = (uint8_t)i;
        tables.to_lower[i] = (uint8_t)i;
    }

    f = fopen(unicode_data_path, "r");
    if (!f) {
        perror("fopen UnicodeData.txt");
        return 1;
    }

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        size_t len = strlen(line);
        if (len && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[len - 1] = '\0';
            if (len > 1 && line[len - 2] == '\r') {
                line[len - 2] = '\0';
            }
        }
        parse_unicode_data_line(line, tables.classification, tables.to_upper, tables.to_lower);
    }
    fclose(f);

    // Sanitize locale for use as both filename and symbol
    char safe_locale[256];
    snprintf(safe_locale, sizeof(safe_locale), "%s", iso_locale);
    for (char *p = safe_locale; *p; ++p) {
        if (*p == '.' || *p == '/' || *p == '\\') *p = '_';
    }

    char out_path[512];
    if (emit_c_output) {
        snprintf(out_path, sizeof(out_path), "%s/%s.c", target_dir, safe_locale);
        if (write_c_source_file(out_path, safe_locale, &tables) != 0) {
            fprintf(stderr, "Failed to write output C file %s\n", out_path);
            return 1;
        }
        if (verbose)
            printf("Generated %s (C source) from %s\n", out_path, unicode_data_path);
    } else {
        snprintf(out_path, sizeof(out_path), "%s/%s.ctbl", target_dir, safe_locale);
        if (write_ctbl_file(out_path, &tables) != 0) {
            fprintf(stderr, "Failed to write output file %s\n", out_path);
            return 1;
        }
        if (verbose)
            printf("Generated %s (binary .ctbl) from %s\n", out_path, unicode_data_path);
    }

    return 0;
}
