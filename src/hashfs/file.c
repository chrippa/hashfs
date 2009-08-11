#include <glib.h>

#include "hashfs.h"

hashfs_file_t *
hashfs_file_new (gchar *filename)
{
	hashfs_file_t *file;
	struct stat info;

	stat(filename, &info);

	file = g_new0(hashfs_file_t, 1);
	file->filename = filename;
	file->size = (gint64) info.st_size;
	file->ed2k = NULL;
	file->md5 = NULL;

	return file;
}

gint
hashfs_file_prop_lookup (hashfs_file_t *file, const gchar *key,
                         gchar **out)
{
	HASHFS_DEBUG("File (%s) looking up property: %s", hashfs_basename(file->filename), key);

	// TODO: Actually look up property from DB.

	return 0;
}

void
hashfs_file_prop_set (hashfs_file_t *file, const gchar *key,
                      gchar *value)
{
	HASHFS_DEBUG("File (%s) setting property: %s = %s", hashfs_basename(file->filename), key, value);

	// TODO: Set property in DB.
}

hashfs_set_t *
hashfs_file_add_to_set (hashfs_file_t *file, gchar *setname)
{
	hashfs_set_t *set;

	set = hashfs_set_new(setname);

	hashfs_set_add_file(set, file);

	file->sets = g_list_append(file->sets, set);

	return set;
}

void
hashfs_file_destroy (hashfs_file_t *file)
{
	if (file->ed2k)
		g_free(file->ed2k);

	if (file->md5)
		g_free(file->md5);

	if (file->sets) {
		GList *item;
		hashfs_set_t *set;

		for (item = g_list_first(file->sets); item; item = g_list_next(item)) {
			set = item->data;
			hashfs_set_destroy(set);
		}
		g_list_free(file->sets);
	}

	HASHFS_DEBUG("File (%s) destroying", hashfs_basename(file->filename));

	g_free(file);
}
