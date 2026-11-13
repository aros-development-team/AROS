#include <proto/acpica.h>

struct Parser
{
    const char *signature;
    const char *name;
    void (*parser)(const ACPI_TABLE_HEADER *table, void (*cb)(const char *));
};

extern const struct Parser ParserTable[];

#define BUFFER_SIZE 256

extern char buf[BUFFER_SIZE];

typedef void (*out_func)(const char *);

void unknown_parser(const ACPI_TABLE_HEADER *table, void (*cb)(const char *));

const struct Parser *FindParser(const char *signature);

