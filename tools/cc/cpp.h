#ifndef h_cpp_
#define h_cpp_

#define MAX_INCLUDE_DEPTH 64
#define MAX_EXPAND_DEPTH  32

struct Macro {
    char *name;
    char *value;
};

struct Cond {
    int parent_active;
    int condition;
    int seen_else;
};

static struct Macro *macros;
static unsigned long nmacros;
static char **incdirs;
static unsigned long nincdirs;
static const char *current_file;
static int errors;

int process_file(FILE *fp, const char *filename, int depth);

#endif
