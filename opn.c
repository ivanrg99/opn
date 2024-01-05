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
} State;

typedef struct {
    char *mime_type;
    char *program;
} Entry;

typedef struct {
    size_t len;
    size_t cap;
    Entry *data;
} Entries;

void
print_usage(void)
{
	printf("Usage: opn [OPTIONS] [FILE]\n"
	       "Open FILE with user's preferred application\n\n"
	       "-d <application>\tOpens FILE with <application> and sets "
	       "<application> as the preferred application for opening FILEs\n"
	       "-b\t\t\tLaunches the preferred application in detached mode (background)\n"
	       "-h\t\t\tShow this message\n");
}


/*
 * Caller must free returned string
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

	/* 2 for the / delimiters and 1 for the null terminator */
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


State
parse_arguments(int argc, char *argv[])
{
	if (argc == 1) {
		print_usage();
		exit(1);
	}

	State args = {0};

	int c;
	while ((c = getopt(argc, argv, "bhd:")) != -1) {
		switch (c) {
			case 'b':
				args.run_in_background = true;
				break;
			case 'd':
				args.set_default = true;
				args.program_name = optarg;
				break;
			case 'h':
				print_usage();
				exit(0);
			case '?':
				if (optopt == 'd') {
					fprintf(stderr,
						"Option -%c needs to be followed by an application name.\n",
						optopt);
					exit(1);
				} else if (isprint(optopt)) {
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
		strlen(dir) + strlen(DEFAULT_CONFIG_FILE_NAME) + 1 + 1;
	char *config_file = calloc(config_file_size, sizeof(char));
	if (config_file == NULL) {
		fprintf(stderr, "ERROR: Could not allocate enough memory\n");
		free(dir);
		return -1;
	}
	snprintf(config_file, config_file_size, "%s/%s", dir,
		 DEFAULT_CONFIG_FILE_NAME);

	/* Truncate config file */
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
		strlen(dir) + strlen(DEFAULT_CONFIG_FILE_NAME) + 1 + 1;
	char *config_file = calloc(config_file_size, sizeof(char));
	if (config_file == NULL) {
		fprintf(stderr, "ERROR: Could not allocate enough memory\n");
		free(dir);
		return NULL;
	}
	snprintf(config_file, config_file_size, "%s/%s", dir,
		 DEFAULT_CONFIG_FILE_NAME);

	/* Open config file */
	FILE *f = fopen(config_file, "r+");
	/* If it can't open, create it */
	if (f == NULL) {
		f = fopen(config_file, "w+");
	}

	/* Error check */
	if (f == NULL) {
		fprintf(stderr,
			"ERROR: Could not read/create %s at directory %s\n",
			DEFAULT_CONFIG_FILE_NAME, dir);
	}

	free(config_file);
	free(dir);
	return f;
}

/*
 * Caller must free returned string
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

Entries
entries_alloc(size_t capacity)
{
	Entries d = {0};
	d.cap = capacity;
	d.data = malloc(sizeof(Entry) * capacity);
	if (d.data == NULL) {
		fprintf(stderr, "Could not allocate enough memory\n");
	}
	return d;
}

void
entries_free(Entries *d)
{
	d->cap = 0;
	d->len = 0;
	free(d->data);
}

void
entries_push(Entries *d, Entry e)
{
	if (d->len == d->cap) {
		d->cap *= 2;
		Entry *cpy = d->data;
		d->data = realloc(d->data, d->cap * sizeof(Entry));
		if (d->data == NULL) {
			fprintf(stderr,
				"Failed to reallocate config entries dynamic data\n");
			free(cpy);
		}
	}
	d->data[d->len] = e;
	d->len++;
}

/*
 * The caller is responsible for freeing the string that is returned
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
			DEFAULT_CONFIG_FILE_NAME);
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
str_trim(char *str)
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

/*
 * Caller must call entries_free on returned Entries
 */
Entries
get_all_config_entries(char *file_contents)
{
	Entries entries = entries_alloc(30);
	if (file_contents == NULL || strlen(file_contents) == 0) {
		return entries;
	}

	char *line_savepoint = NULL;

	char *line = strtok_r(file_contents, "\n", &line_savepoint);
	while (line != NULL) {
		Entry entry;
		entry.mime_type = strtok(line, "=");
		entry.program = strtok(NULL, "=");
		line = strtok_r(NULL, "\n", &line_savepoint);
		if (entry.mime_type == NULL || entry.program == NULL) {
			continue;
		}

		entry.mime_type = str_trim(entry.mime_type);
		entry.program = str_trim(entry.program);

		entries_push(&entries, entry);

	}
	return entries;
}

void
resave_config_file(FILE *f, Entries entries)
{
	rewind(f);
	char line[LINE_SIZE];
	int total_size = 0;
	for (size_t i = 0; i < entries.len; ++i) {
		Entry e = entries.data[i];
		total_size += snprintf(line, LINE_SIZE, "%s=%s\n", e.mime_type,
				       e.program);
		fputs(line, f);
	}
	fflush(f);
	truncate_config_file(total_size);
}

/*
 * Caller must free returned string
 */
char *
find_program_in_config(FILE *f, char *type, State args)
{
	char *file_contents = load_entire_file(f);
	Entries entries = get_all_config_entries(file_contents);
	char *program = NULL;
	for (size_t i = 0; i < entries.len; ++i) {
		Entry *e = &entries.data[i];
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
			Entry e = {
				.program = args.program_name,
				.mime_type = type,
			};
			entries_push(&entries, e);
			program = strdup(args.program_name);
		} else {
			fprintf(stderr,
				"No application in config found for mimetype: %s\n"
				"You may rerun this command with -d <application> "
				"to open the file with <application> and set "
				"it as the default opener in the config\n", type);
		}
	}

	// We need to re-save the file with the changes
	if (args.set_default) {
		resave_config_file(f, entries);
	}

	free(file_contents);
	entries_free(&entries);
	return program;
}

int
main(int argc, char *argv[])
{
	State state = parse_arguments(argc, argv);

	FILE *f = get_config_file();
	if (f == NULL) {
		exit(1);
	}

	char *type = get_file_mime_type(state.file_path);
	if (type == NULL) {
		fclose(f);
		free(type);
		exit(1);
	}

	char *program = find_program_in_config(f, type, state);
	if (program == NULL) {
		exit(1);
	}

	if (state.run_in_background) {
		int pid = getpid();
		fork();
		if (getpid() == pid) {
			goto cleanup;
		}
	}

	if (execlp(program, program, state.file_path, NULL) < 0) {
		perror("Error running default program");
	}

	cleanup:
	free(program);
	fclose(f);
	free(type);
	return 0;
}
