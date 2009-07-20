
#include <sys/stat.h>
#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gmodule.h>


#include "hashfs.h"

typedef void (*hashfs_cmd_func) (gint argc, gchar **argv);

typedef struct hashfs_cmd_St {
	gchar *name;
	hashfs_cmd_func func;
	gchar *description;
} hashfs_cmd_t;


static void hashfs_hash_file (hashfs_backend_t *backend, gchar *path);
static void hashfs_hash_dir (hashfs_backend_t  *backend, gchar *path);

static void hashfs_cmd (hashfs_cmd_t *cmds, gchar *cmd, gint argv, gchar **args);
static void hashfs_cmd_config (gint argc, gchar **argv);
static void hashfs_cmd_help (gint argc, gchar **argv);
static void hashfs_cmd_update (gint argc, gchar **argv);

static
hashfs_cmd_t main_cmds[] = {
	{ "config", hashfs_cmd_config, "Manipulate configuration" },
	{ "help",   hashfs_cmd_help,   "Show available commands and description" },
	{ "update", hashfs_cmd_update, "Scan directory and add metadata" },

	{ NULL, NULL, NULL},
};

static
hashfs_cmd_t config_cmds[] = {

	{ NULL, NULL, NULL},
};


static void
hashfs_hash_file (hashfs_backend_t *backend, gchar *path)
{
	hashfs_file_t *file;

	HASHFS_LOG("Hashing file: %s", hashfs_basename(path));

	file = hashfs_file_new(path);

	hashfs_backend_file(backend, file);

	g_free(file);
}

static void
hashfs_hash_dir (hashfs_backend_t *backend, gchar *path)
{
	GDir *dir;
	GError *error;
	const gchar *filename;
	gchar *fullpath;

	HASHFS_LOG("Hashing dir: %s", path);

	error = NULL;
	dir = g_dir_open(path, 0, &error);

	if (error) {
		HASHFS_DEBUG("Failed to open directory (%s): %s", path, error->message);
		g_error_free(error);

		return;
	}

	while ((filename = g_dir_read_name(dir))) {
		if (!g_strcmp0(filename, ".") || !g_strcmp0(filename, ".."))
			continue;

		fullpath = g_build_filename(path, filename, NULL);

		if (g_file_test(fullpath, G_FILE_TEST_IS_REGULAR)) {
			hashfs_hash_file(backend, fullpath);
		} else if (g_file_test(fullpath, G_FILE_TEST_IS_DIR)) {
			hashfs_hash_dir(backend, fullpath);
		}

		g_free(fullpath);
	}

	g_dir_close(dir);
}

static void
hashfs_cmd (hashfs_cmd_t *cmds, gchar *cmd, gint argv, gchar **args)
{
	for (gint i = 0; cmds[i].name; i++) {
		if (!g_strcmp0(cmd, cmds[i].name)) {
			cmds[i].func(argv - 2, args + 2);
		}
	}
}

static void
hashfs_cmd_config (gint argc, gchar **argv)
{
	GKeyFile *config;

	config = hashfs_config_keyfile();

	if (argc == 0) {
		gint numgroups;
		gchar **groups;

		groups = g_key_file_get_groups(config, &numgroups);

		for (gint i = 0; i < numgroups; i++) {
			gint numkeys;
			gchar **keys;

			keys = g_key_file_get_keys(config, groups[i], &numkeys, NULL);

			for (gint j = 0; j < numkeys; j++) {
				printf("%s.%s = %s\n", groups[i], keys[j], g_key_file_get_string(config,
				       groups[i], keys[j], NULL));
			}
		}
	} else if (argc == 1) {
		gchar *val;
		gchar **split;

		split = g_strsplit(argv[0], ".", 2);

		if (g_strv_length(split) < 2) {
			HASHFS_ERROR("Invalid key format");
		} else {
			hashfs_config_property_lookup(split[0], split[1], &val);
			printf("%s\n", val);
		}
	} else if (argc == 2) {
		gchar **split;

		split = g_strsplit(argv[0], ".", 2);

		if (g_strv_length(split) < 2) {
			HASHFS_ERROR("Invalid key format");
		} else {
			hashfs_config_property_set(split[0], split[1], argv[1]);
			printf("Config value %s.%s set to %s\n", split[0], split[1], argv[1]);
		}
	}
}

static void
hashfs_cmd_help (gint argc, gchar **argv)
{
	printf("Available commands:\n");

	for (gint i = 0; main_cmds[i].name; i++) {
		printf("  %-15s %s\n", main_cmds[i].name, main_cmds[i].description);
	}
}

static void
hashfs_cmd_update (gint argc, gchar **argv)
{
	hashfs_backend_t *backend;

	if (argc == 0) {
	} else if (argc == 2) {
		backend = hashfs_backends_lookup(argv[0]);

		if (backend) {
			hashfs_backend_init(backend);
			hashfs_hash_dir(backend, argv[1]);
		}
	}
}

gint
main (gint argc, gchar **argv)
{
	hashfs_backend_t *backend;

	hashfs_config_load();

	if (g_module_supported()) {
		hashfs_backends_load(g_build_filename(g_get_user_config_dir(), "hashfs/backends", NULL));
		hashfs_backends_load("/usr/local/lib/hashfs");
		hashfs_backends_load("./_build_/default/src/backends/anidb/");
	} else {
		HASHFS_LOG("This platform does not support loading modules");

		return 0;
	}

	if (argc > 1)
		hashfs_cmd(main_cmds, argv[1], argc, argv);
	else
		hashfs_cmd(main_cmds, "help", argc, argv);


	hashfs_config_save();
	hashfs_backends_destroy();

	return 0;
}
