#include <glib.h>
#include <glib/gprintf.h>


#include "hashfs.h"

hashfs_set_t *
hashfs_set_new (const gchar *name, const gchar *source, const gchar *type)
{
	hashfs_set_t *set;

	set = g_new0(hashfs_set_t, 1);
	set->entry = hashfs_db_entry_new("set", name, source, type);
	g_strlcpy(set->name, name, sizeof(set->name));

	HASHFS_DEBUG("Creating new set: %s", name);

	return set;
}

gboolean
hashfs_set_prop_lookup (hashfs_set_t *set, const gchar *key,
                        const gchar **out)
{
	HASHFS_DEBUG("Set (%s) looking up property: %s", set->name, key);

	return hashfs_db_entry_lookup(set->entry, key, out);
}

void
hashfs_set_prop_set (hashfs_set_t *set, const gchar *key,
                     const gchar *value)
{
	HASHFS_DEBUG("Set (%s) setting property: %s = %s", set->name, key, value);

	hashfs_db_entry_set(set->entry, key, value);
}

void
hashfs_set_destroy (hashfs_set_t *set)
{
	HASHFS_DEBUG("Set (%s) destroying", set->name);

	if (set->entry) {
		hashfs_db_entry_put(set->entry);
		hashfs_db_entry_destroy(set->entry);
	}

	g_free(set);
}
