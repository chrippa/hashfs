#include <glib.h>

#include "hashfs.h"

static GKeyFile *config;
static gchar * hashfs_config_build_path (void);


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
hashfs_config_init (void)
{
	GError *error = NULL;
	gchar *configfile;

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

	g_free(configfile);
}

void
hashfs_config_destroy (void)
{
	gchar *configfile, *data;
	FILE *file;

	configfile = hashfs_config_build_path();
	data = g_key_file_to_data(config, NULL, NULL);

	file = g_fopen(configfile, "w");
	fputs(data, file);

	fclose(file);

	g_free(configfile);
	g_free(data);

	g_key_file_free(config);
}

static gchar *
hashfs_config_build_path (void)
{
	gchar *configdir, *configfile;
	const gchar *userconfigdir;

	userconfigdir = g_get_user_config_dir();
	configdir = g_build_filename(userconfigdir, "hashfs", NULL);
	configfile = g_build_filename(configdir, "hashfs.conf", NULL);

	if (g_file_test(configdir, G_FILE_TEST_EXISTS)) {
		if (!g_file_test(configdir, G_FILE_TEST_IS_DIR))
			HASHFS_ERROR("Config directory already exists but is not a directory (%s)", configdir);
	} else {
		if (g_mkdir_with_parents(configdir, 0755) < 0) {
			HASHFS_ERROR("Unable to create config directory (%s)", configdir);
		}
	}

	g_free(configdir);

	return configfile;
}
