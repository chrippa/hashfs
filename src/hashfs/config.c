
#include <sys/stat.h>
#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gmodule.h>

#include "hashfs.h"

static GKeyFile *config;
static gboolean config_loaded;
static const gchar * hashfs_config_build_path (void);


gboolean
hashfs_config_property_exists (const gchar *group, const gchar *key)
{
	if (g_key_file_has_group(config, group)) {
		return g_key_file_has_key(config, group, key, NULL);
	}

	return FALSE;
}

void
hashfs_config_property_lookup (const gchar *group, const gchar *key, gchar **out)
{
	*out = g_key_file_get_string(config, group, key, NULL);
}

void
hashfs_config_property_set (const gchar *group, const gchar *key, const gchar *value)
{
	g_key_file_set_string(config, group, key, value);
}

GKeyFile *
hashfs_config_keyfile (void)
{
	return config;
}

void
hashfs_config_load (void)
{
	GError *error = NULL;
	const gchar *configfile;

	config = g_key_file_new();
	configfile = hashfs_config_build_path();

	g_key_file_load_from_file(config, configfile, G_KEY_FILE_NONE, &error);

	if (error) {
		/* Don't error when file doesn't exist or is empty */
		if (error->code != 4 && error->code != 1) {
			HASHFS_ERROR("Unable to load config file (%s): %d %s", configfile, error->message);
		}

		g_error_free(error);
	}
}

void
hashfs_config_save (void)
{
	const gchar *configfile;
	FILE *file;

	configfile = hashfs_config_build_path();
	file = g_fopen(configfile, "w");
	fputs(g_key_file_to_data(config, NULL, NULL), file);

	fclose(file);
}

static const gchar *
hashfs_config_build_path (void)
{
	const gchar *configdir;
	const gchar *configfile;

	configdir = g_build_filename(g_get_user_config_dir(), "hashfs", NULL);
	configfile = g_build_filename(configdir, "hashfs.conf", NULL);

	if (g_file_test(configdir, G_FILE_TEST_EXISTS)) {
		if (!g_file_test(configdir, G_FILE_TEST_IS_DIR))
			HASHFS_ERROR("Config directory already exists but is not a directory (%s)", configdir);
	} else {
		if (g_mkdir_with_parents(configdir, 0755) < 0) {
			HASHFS_ERROR("Unable to create config directory (%s)", configdir);
		}
	}

	return configfile;
}
