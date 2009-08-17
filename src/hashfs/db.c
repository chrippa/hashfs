#include <glib.h>
#include <string.h>
#include <stdio.h>


#include "hashfs.h"


static hashfs_db_t *db;

gboolean
hashfs_db_open (void)
{
	gchar *path;
	gboolean rval;

	g_return_val_if_fail(db == NULL, FALSE);

	db = g_new0(hashfs_db_t, 1);
	db->tdb = tctdbnew();

	path = g_build_filename(g_get_user_config_dir(), "hashfs", "metadata.tct", NULL);

	if (!tctdbopen(db->tdb, path, TDBOWRITER | TDBOCREAT)) {
		HASHFS_DEBUG("Unable to open DB: %s", hashfs_db_error());

		rval = FALSE;
	} else {
		HASHFS_DEBUG("Successfully opened DB");

		rval = TRUE;
	}

	g_free(path);

	return rval;
}

void
hashfs_db_close (void)
{
	g_return_if_fail(db != NULL);

	if (!tctdbclose(db->tdb)) {
		HASHFS_DEBUG("Unable to close DB: %s", hashfs_db_error());
	} else {
		HASHFS_DEBUG("Successfully closed DB");
	}

	tctdbdel(db->tdb);

	free(db);
}

gboolean
hashfs_db_tran_begin (void)
{
	return (gboolean) tctdbtranbegin(db->tdb);
}

gboolean
hashfs_db_tran_commit (void)
{
	return (gboolean) tctdbtrancommit(db->tdb);
}

gboolean
hashfs_db_tran_abort (void)
{
	return (gboolean) tctdbtranabort(db->tdb);
}

gchar *
hashfs_db_error (void)
{
	gint ecode = tctdbecode(db->tdb);

	return (gchar *) tctdberrmsg(ecode);
}

hashfs_db_entry_t *
hashfs_db_entry_new (const gchar *prefix, const gchar *id,
                     const gchar *source, const gchar *type)
{
	hashfs_db_entry_t *entry;
	gchar *pkey, *md5;

	md5 = hashfs_md5_str(id);

	if (!g_strcmp0(prefix, "set")) {
		pkey = g_strdup_printf("%s:%s:%s:%s", prefix, source, type, md5);
	} else {
		pkey = g_strdup_printf("%s:%s", prefix, md5);
	}

	entry = g_new0(hashfs_db_entry_t, 1);
	entry->pkey = pkey;
	entry->data = tcmapnew();

	HASHFS_DEBUG("Created entry with pkey: %s", pkey);

	g_free(md5);

	return entry;
}

void
hashfs_db_entry_set (hashfs_db_entry_t *entry, const gchar *key,
                     const gchar *value)
{
	g_return_if_fail(entry != NULL);
	g_return_if_fail(entry->data != NULL);

	tcmapput2(entry->data, key, value);
}

gboolean
hashfs_db_entry_get (hashfs_db_entry_t *entry, const gchar *key,
                     const gchar **out)
{

}

gboolean
hashfs_db_entry_put (hashfs_db_entry_t *entry)
{
	g_return_val_if_fail(entry != NULL, FALSE);
	g_return_val_if_fail(entry->pkey != NULL, FALSE);
	g_return_val_if_fail(entry->data != NULL, FALSE);

	return (gboolean) tctdbputcat(db->tdb, entry->pkey, strlen(entry->pkey),
	                              entry->data);
}

void
hashfs_db_entry_destroy (hashfs_db_entry_t *entry)
{
	g_return_if_fail (entry != NULL);

	if (entry->data)
		tcmapdel(entry->data);

	if (entry->pkey)
		g_free(entry->pkey);

	g_free(entry);
}
