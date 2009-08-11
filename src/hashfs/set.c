#include <glib.h>
#include <glib/gprintf.h>


#include "hashfs.h"

hashfs_set_t *
hashfs_set_new (gchar *name)
{
	hashfs_set_t *set;

	set = g_new0(hashfs_set_t, 1);
	g_strlcpy(set->name, name, sizeof(set->name));

	HASHFS_DEBUG("Creating new set: %s", name);

	// TODO: Create new entry in DB if necessary.

	return set;
}

void
hashfs_set_add_file (hashfs_set_t *set, hashfs_file_t *file)
{
	HASHFS_DEBUG("Set (%s) adding file: %s", set->name, file->filename);

	// TODO: Add fileid to set in DB.
}

gint
hashfs_set_prop_lookup (hashfs_set_t *set, const gchar *key,
                        gchar **out)
{
	HASHFS_DEBUG("Set (%s) looking up property: %s", set->name, key);

	// TODO: Actually look up property from DB.

	return 0;
}

void
hashfs_set_prop_set (hashfs_set_t *set, const gchar *key,
                     gchar *value)
{
	HASHFS_DEBUG("Set (%s) setting property: %s = %s", set->name, key, value);

	// TODO: Set property in DB.
}

void
hashfs_set_destroy (hashfs_set_t *set)
{
	HASHFS_DEBUG("Set (%s) destroying", set->name);

	g_free(set);
}
