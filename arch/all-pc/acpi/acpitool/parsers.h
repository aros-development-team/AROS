struct Parser
{
    unsigned int signature;
    char *name;
    void (*parser)();
};

#define BUFFER_SIZE 256

extern char buf[BUFFER_SIZE];

typedef void (*out_func)(const char *);

void unknown_parser(struct ACPI_TABLE_DEF_HEADER *table, void (*cb)(const char *));

const struct Parser *FindParser(unsigned int signature);
