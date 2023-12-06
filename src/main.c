#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdbool.h>
#include <ctype.h>
#include <sys/stat.h>
#include <errno.h>

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
                exit(1);
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

int
main(int argc, char *argv[])
{
        if (argc == 1) {
                printf("Print help\n");
                exit(1);
        }

        char *dir = get_configdir();

        bool set_default = 0;
        int c;
        char *program_name = NULL;

        while ((c = getopt(argc, argv, "d:")) != -1) {
                printf("optarg: %s\n", optarg);
                printf("optopt: %c\n", optopt);
                switch (c) {
                        case 'd':
                                set_default = true;
                                program_name = optarg;
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
                                break;
                        default:
                                abort();
                }
        }
        const char *file_name = argv[optind];
	if (file_name == NULL) {
                fprintf(stderr, "ERROR: Missing file to open\n");
                exit(1);
	}

        int err = mkdir(dir, S_IRUSR | S_IWUSR | S_IXUSR);
        if (err == -1 && errno != EEXIST) {
                fprintf(stderr, "ERROR: Could not create initial config folder\n");
                exit(1);
        }

        /* 1 for null terminator and 1 for slash */
        size_t config_file_size = strlen(dir) + strlen(CONFIG_FILE_NAME) + 1 + 1;
        char *config_file = calloc(config_file_size, sizeof(char));
        if (config_file == NULL) {
                fprintf(stderr, "ERROR: Could not allocate enough memory\n");
                exit(1);
        }
        snprintf(config_file, config_file_size, "%s/%s", dir, CONFIG_FILE_NAME);

        printf("Hello world!: %s\n", dir);
        printf("Is it default?: %d! | program name: %s\n", set_default, program_name);
        printf("File name: %s\n", file_name);
        printf("Config file path: %s\n", config_file);

        FILE *f = fopen(config_file, "a+");
        fclose(f);

        free(config_file);
        free(dir);
        return 0;
}
