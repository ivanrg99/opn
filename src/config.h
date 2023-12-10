#ifndef OPN_CONFIG_H
#define OPN_CONFIG_H

#include <stdlib.h>

typedef struct {
    bool set_default;
    char *program_name;
    char *file_path;
} Args;

typedef struct {
    char *mime_type;
    char *program;
} KVEntry;

typedef struct {
    size_t len;
    size_t cap;
    KVEntry *array;
} KVEntry_DA;

static const char *DEFAULT_CONFIG_FOLDER = ".config";
static const char *DEFAULT_SETTINGS_FOLDER = "opn";
static const char *CONFIG_FILE_NAME = "mimeapps.list";

#define LINE_SIZE 1024

#endif
