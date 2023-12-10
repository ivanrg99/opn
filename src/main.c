#include <magic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdbool.h>
#include <ctype.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>

#include "config.h"


/* Returns the appropriate config folder path as a malloced null terminated string
 * If it can't generate the string it will exit the program with an error
 * The caller is responsible for freeing the string that is returned
 */
char *
get_configdir(void)
{
        const char *HOME = getenv("HOME");
        if (HOME == NULL) {
                fprintf(stderr, "ERROR: $HOME is not defined!\n");
                return NULL;
        }

        const char *XDG_CONFIG_HOME = getenv("XDG_CONFIG_HOME");

        if (XDG_CONFIG_HOME == NULL) {
                XDG_CONFIG_HOME = DEFAULT_CONFIG_FOLDER;
        }

        size_t n = strlen(HOME) + strlen(XDG_CONFIG_HOME) +
                   strlen(DEFAULT_SETTINGS_FOLDER);
        n = n + 2 + 1; /* 1 for the / delimiters and 1 for the null terminator */
        char *configfile = calloc(n, sizeof(char));
        if (configfile == NULL) {
                fprintf(stderr,
                        "ERROR: Could not allocate enough memory for config folder name\n");
                exit(1);
        }

        snprintf(configfile, n,
                 "%s/%s/%s", HOME, XDG_CONFIG_HOME, DEFAULT_SETTINGS_FOLDER);

        return configfile;
}


Args
parse_arguments(int argc, char *argv[])
{
        if (argc == 1) {
                printf("Print help\n");
                exit(1);
        }

        Args args = {0};

        int c;
        while ((c = getopt(argc, argv, "d:")) != -1) {
                switch (c) {
                        case 'd':
                                args.set_default = true;
                                args.program_name = optarg;
                                break;
                        case '?':
                                if (optopt == 'd') {
                                        fprintf(stderr,
                                                "Option -%c requires the name of a program as an argument.\n",
                                                optopt);
                                        exit(1);
                                } else if (isprint(optopt)) {
                                        fprintf(stderr,
                                                "Unknown option `-%c'.\n",
                                                optopt);
                                        exit(1);
                                } else {
                                        fprintf(stderr,
                                                "Unknown option character `\\x%d'.\n",
                                                optopt);
                                        exit(1);
                                }
                        default:
                                abort();
                }
        }

        args.file_path = argv[optind];
        if (args.file_path == NULL) {
                fprintf(stderr, "ERROR: Missing file to open\n");
                exit(1);
        }
        if (access(args.file_path, F_OK) != 0) {
                fprintf(stderr, "ERROR: Could not find file %s\n", args.file_path);
                exit(1);
        }

        return args;
}


FILE *
get_config_file(void)
{
        /* Get config directory */
        char *dir = get_configdir();
        int err = mkdir(dir, S_IRUSR | S_IWUSR | S_IXUSR);
        if (err == -1 && errno != EEXIST) {
                fprintf(stderr, "ERROR: Could not create initial config folder\n");
                exit(1);
        }

        /* Get config file path */
        /* 1 for null terminator and 1 for slash */
        size_t config_file_size = strlen(dir) + strlen(CONFIG_FILE_NAME) + 1 + 1;
        char *config_file = calloc(config_file_size, sizeof(char));
        if (config_file == NULL) {
                fprintf(stderr, "ERROR: Could not allocate enough memory\n");
                exit(1);
        }
        snprintf(config_file, config_file_size, "%s/%s", dir, CONFIG_FILE_NAME);

        /* Open config file */
        FILE *f = fopen(config_file, "a+");
        if (f == NULL) {
                fprintf(stderr, "ERROR: Could not open mimeapps.list at directory %s\n", dir);
                exit(1);
        }

        free(config_file);
        free(dir);
        return f;
}

/* Returns the mime type of the file as a malloced null terminated string
 * Or NULL if error, as well as printing the error
 * The caller is responsible for freeing the string that is returned
 */
char *
get_file_mime_type(const char *file_path)
{
        // Create a magic object
        magic_t magic_cookie = magic_open(MAGIC_MIME_TYPE);
        if (magic_cookie == NULL) {
                fprintf(stderr, "Unable to initialize libmagic\n");
                return NULL;
        }

        // Load default magic database
        if (magic_load(magic_cookie, NULL) != 0) {
                fprintf(stderr, "Cannot load libmagic database - %s\n", magic_error(magic_cookie));
                magic_close(magic_cookie);
                return NULL;
        }

        // Get MIME type of the file
        const char *mime_type = magic_file(magic_cookie, file_path);
        if (mime_type == NULL) {
                fprintf(stderr, "Cannot determine file type: %s\n", magic_error(magic_cookie));
                magic_close(magic_cookie);
                return NULL;
        }

        char *res = calloc(strlen(mime_type) + 1, sizeof(char));
        memcpy(res, mime_type, strlen(mime_type));
        magic_close(magic_cookie);
        return res;
}

KVEntry_DA
KVEntry_DA_New(size_t capacity)
{
        KVEntry_DA d = {0};
        d.cap = capacity;
        d.array = malloc(sizeof(KVEntry) * capacity);
        if (d.array == NULL) {
                fprintf(stderr, "Could not allocate enough memory\n");
        }
        return d;
}

void
KVEntry_DA_Free(KVEntry_DA *d)
{
        d->cap = 0;
        d->len = 0;
        free(d->array);
}

void
KVEntry_DA_Push(KVEntry_DA *d, KVEntry e)
{
        if (d->len == d->cap) {
                d->cap *= 2;
                KVEntry *cpy = d->array;
                d->array = realloc(d->array, d->cap * sizeof(KVEntry));
                if (d->array == NULL) {
                        fprintf(stderr, "Failed to reallocate config entries dynamic array\n");
                        free(cpy);
                }
        }
        d->array[d->len] = e;
        d->len++;
}

KVEntry
KVEntry_DA_Pop(KVEntry_DA *d)
{
        d->len--;
        return d->array[d->len];
}


/* Returns a string with the whole contents of the file as a null terminated malloced string
 * Or NULL on error
 * The caller is responsible for freeing the string
 */
char *
load_entire_file(FILE *f)
{
        size_t size;
        fseek(f, 0, SEEK_END);
        size = (size_t) ftell(f);
        rewind(f);
        char *contents = malloc(size * sizeof(char) + 1);
        if (contents == NULL) {
                fprintf(stderr, "Could not allocate enough memory to read mimeapps.list\n");
                return NULL;
        }

        /* Could not read file */
        if (fread(contents, size, 1, f) != 1) {
                free(contents);
                return NULL;
        }

        contents[size] = 0;
        return contents;
}

char *
strtrim(char *str)
{
        char *beg = str;
        char *end = str + strlen(str) - 1;
        while (beg < end && isspace(*beg)) {
                beg++;
        }
        while (end > beg && isspace(*end)) {
                end--;
        }
        str = beg;
        *(end + 1) = 0;
        return str;
}

/**
 * Caller must call KVEntry_DA_Free on returned value
 * @return
 */
KVEntry_DA
get_all_config_entries(char *file_contents)
{
        KVEntry_DA entries = KVEntry_DA_New(40);

        char *line_savepoint = NULL;

        char *line = strtok_r(file_contents, "\n", &line_savepoint);
        while (line != NULL) {
                KVEntry entry;
                entry.mime_type = strtok(line, "=");
                entry.program = strtok(NULL, "=");
                line = strtok_r(NULL, "\n", &line_savepoint);
                if (entry.mime_type == NULL || entry.program == NULL) {
                        continue;
                }

                entry.mime_type = strtrim(entry.mime_type);
                entry.program = strtrim(entry.program);

                KVEntry_DA_Push(&entries, entry);

        }
        return entries;
}

void
find_program_in_config(FILE *f)
{
        char *file_contents = load_entire_file(f);
        KVEntry_DA entries = get_all_config_entries(file_contents);

        for (size_t i = 0; i < entries.len; i++) {
                KVEntry e = entries.array[i];
        }

        free(file_contents);
        KVEntry_DA_Free(&entries);
}

int
main(int argc, char *argv[])
{
        // get args
        // parse all args (use better error messages with suggestions)
        // get config file
        // parse config file
        // update config file if needed
        // run program with execve

        Args args = parse_arguments(argc, argv);

        FILE *f = get_config_file();

        char *type = get_file_mime_type(args.file_path);
        if (type == NULL) {
                goto cleanup;
        }

        find_program_in_config(f);

        printf("TYPE: %s\n", type);

        cleanup:
        fclose(f);
        free(type);
        return 0;
}
