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

typedef struct {
    bool set_default;
    bool run_in_background;
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

	/* 1 for the / delimiters and 1 for the null terminator */
	n = n + 2 + 1;
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
	while ((c = getopt(argc, argv, "bd:")) != -1) {
		switch (c) {
			case 'd':
				args.set_default = true;
				args.program_name = optarg;
				break;
			case 'b':
				args.run_in_background = true;
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
		fprintf(stderr, "ERROR: Could not find file %s\n",
			args.file_path);
		exit(1);
	}

	return args;
}

int
truncate_config_file(int size)
{
	/* Get config directory */
	char *dir = get_configdir();
	int err = mkdir(dir, S_IRUSR | S_IWUSR | S_IXUSR);
	if (err == -1 && errno != EEXIST) {
		fprintf(stderr,
			"ERROR: Could not create initial config folder\n");
		return -1;
	}

	/* Get config file path */
	/* 1 for null terminator and 1 for slash */
	size_t config_file_size =
		strlen(dir) + strlen(CONFIG_FILE_NAME) + 1 + 1;
	char *config_file = calloc(config_file_size, sizeof(char));
	if (config_file == NULL) {
		fprintf(stderr, "ERROR: Could not allocate enough memory\n");
		free(dir);
		return -1;
	}
	snprintf(config_file, config_file_size, "%s/%s", dir, CONFIG_FILE_NAME);

	/* Truncate config file */
	printf("Truncating %s\n", config_file);
	int res = truncate(config_file, (long) size);

	free(config_file);
	free(dir);
	return res;
}

FILE *
get_config_file(void)
{
	/* Get config directory */
	char *dir = get_configdir();
	int err = mkdir(dir, S_IRUSR | S_IWUSR | S_IXUSR);
	if (err == -1 && errno != EEXIST) {
		fprintf(stderr,
			"ERROR: Could not create initial config folder\n");
		return NULL;
	}

	/* Get config file path */
	/* 1 for null terminator and 1 for slash */
	size_t config_file_size =
		strlen(dir) + strlen(CONFIG_FILE_NAME) + 1 + 1;
	char *config_file = calloc(config_file_size, sizeof(char));
	if (config_file == NULL) {
		fprintf(stderr, "ERROR: Could not allocate enough memory\n");
		free(dir);
		return NULL;
	}
	snprintf(config_file, config_file_size, "%s/%s", dir, CONFIG_FILE_NAME);

	/* Open config file */
	FILE *f = fopen(config_file, "r+");
	if (f == NULL) {
		fprintf(stderr, "ERROR: Could not open %s at directory %s\n",
			CONFIG_FILE_NAME, dir);
		free(dir);
		free(config_file);
		return NULL;
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
		fprintf(stderr, "Cannot load libmagic database - %s\n",
			magic_error(magic_cookie));
		magic_close(magic_cookie);
		return NULL;
	}

	// Get MIME type of the file
	const char *mime_type = magic_file(magic_cookie, file_path);
	if (mime_type == NULL) {
		fprintf(stderr, "Cannot determine file type: %s\n",
			magic_error(magic_cookie));
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
			fprintf(stderr,
				"Failed to reallocate config entries dynamic array\n");
			free(cpy);
		}
	}
	d->array[d->len] = e;
	d->len++;
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
		fprintf(stderr, "Could not allocate enough memory to read %s\n",
			CONFIG_FILE_NAME);
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
	KVEntry_DA entries = KVEntry_DA_New(1);

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
resave_config_file(FILE *f, KVEntry_DA entries)
{
	//DELETEME
	for (size_t i = 0; i < entries.len; ++i) {
		KVEntry *e = &entries.array[i];
		printf("ENTRY: %s=%s\n", e->mime_type, e->program);
	}
	rewind(f);
	char line[LINE_SIZE];
	int total_size = 0;
	for (size_t i = 0; i < entries.len; ++i) {
		printf("SIZE: %d\n", total_size);
		KVEntry e = entries.array[i];
		total_size += snprintf(line, LINE_SIZE, "%s=%s\n", e.mime_type,
				       e.program);
		printf("SIZE: %d\n", total_size);
		printf("WRITING: %s\n", line);
		int res = fputs(line, f);
		printf("FPUTS RES: %d\n", res);
		perror(NULL);
	}
	fflush(f);
	truncate_config_file(total_size);
}

/**
 *
 * @param f
 * @param type
 * @param args
 * @return FREE THIS
 */
char *
find_program_in_config(FILE *f, char *type, Args args)
{
	char *file_contents = load_entire_file(f);
	KVEntry_DA entries = get_all_config_entries(file_contents);
	char *program = NULL;
	for (size_t i = 0; i < entries.len; ++i) {
		KVEntry *e = &entries.array[i];
		if (strcmp(e->mime_type, type) == 0) {
			if (args.set_default) {
				e->program = args.program_name;
			}
			program = strdup(e->program);
			break;
		}
	}

	if (program == NULL) {
		if (args.set_default) {
			KVEntry e = {
				.program = args.program_name,
				.mime_type = type,
			};
			KVEntry_DA_Push(&entries, e);
			program = strdup(args.program_name);
		} else {
			fprintf(stderr,
				"No program specified to open things of type blabla"
				"you can do so by running the same command with -d blabla\n");
		}
	}
	//DELETEME
	for (size_t i = 0; i < entries.len; ++i) {
		KVEntry *e = &entries.array[i];
		printf("ENTRY: %s=%s\n", e->mime_type, e->program);
	}

	// We need to resave the file with the changes
	if (args.set_default) {
		resave_config_file(f, entries);
	}

	free(file_contents);
	KVEntry_DA_Free(&entries);
	return program;
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
	if (f == NULL) {
		exit(1);
	}

	char *type = get_file_mime_type(args.file_path);
	if (type == NULL) {
		fclose(f);
		free(type);
		exit(1);
	}

	char *program = find_program_in_config(f, type, args);
	if (program == NULL) {
		exit(1);
	}

	if (args.run_in_background) {
		int pid = getpid();
		fork();
		if (getpid() == pid) {
			goto cleanup;
		}
	}

	if (execlp(program, program, args.file_path, NULL) < 0) {
		perror("Error running default program");
	}

	cleanup:
	free(program);
	fclose(f);
	free(type);
	return 0;
}
