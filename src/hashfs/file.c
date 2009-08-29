#include <glib.h>

#include "hashfs.h"

hashfs_file_t *
hashfs_file_new (const gchar *filename, hashfs_backend_t *backend)
{
	hashfs_file_t *file;
	struct stat info;

	stat(filename, &info);

	file = g_new0(hashfs_file_t, 1);
	file->backend = backend;
	file->entry = hashfs_db_entry_new("file", filename, NULL, NULL);
	file->filename = g_strdup(filename);
	file->size = (gint64) info.st_size;

	file->ed2k = NULL;
	file->md5 = NULL;

	hashfs_db_tran_begin();
	hashfs_db_entry_set(file->entry, "path", filename);

	return file;
}

gboolean
hashfs_file_prop_lookup (hashfs_file_t *file, const gchar *key,
                         const gchar **out)
{
	HASHFS_DEBUG("File (%s) looking up property: %s", hashfs_basename(file->filename), key);

	return hashfs_db_entry_lookup(file->entry, key, out);
}

void
hashfs_file_prop_set (hashfs_file_t *file, const gchar *key,
                      const gchar *value)
{
	HASHFS_DEBUG("File (%s) setting property: %s = %s", hashfs_basename(file->filename), key, value);

	hashfs_db_entry_set(file->entry, key, value);
}

hashfs_set_t *
hashfs_file_add_to_set (hashfs_file_t *file, const gchar *name,
                        const gchar *type)
{
	hashfs_set_t *set;
	const gchar *source;

	if (file->backend)
		source = file->backend->desc->shortname;
	else
		source = "unknown";

	set = hashfs_set_new(name, source, type);

	hashfs_db_entry_set(file->entry, type, set->entry->pkey);

	file->sets = g_list_append(file->sets, set);

	return set;
}

void
hashfs_file_destroy (hashfs_file_t *file)
{
	HASHFS_DEBUG("File (%s) destroying", hashfs_basename(file->filename));

	hashfs_db_tran_commit();

	if (file->filename)
		g_free(file->filename);

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

	if (file->entry) {
		hashfs_db_entry_put(file->entry);
		hashfs_db_entry_destroy(file->entry);
	}


	g_free(file);
}
