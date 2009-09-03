#include <glib.h>
#include <string.h>
#include <stdio.h>

#include "hashfs.h"

#define HASHFS_QUERY_PATTERN "([A-Za-z\\:]+)\\.(\\w+)\\((.*?)\\)"
#define LENGTH(x) sizeof(x)/sizeof(x[0])

static hashfs_db_t *db;

typedef struct {
	gint op;
	gchar *name;
} hashfs_db_query_cond_t;

static hashfs_db_query_cond_t query_cond[] = {
	{ TDBQCSTREQ,         "Equals" },
	{ TDBQCSTRINC,        "Contains" },
	{ TDBQCSTRBW,         "BeginsWith" },
	{ TDBQCSTREQ,         "EndsWith" },
	{ TDBQCSTRAND,        "IncludeAll" },
	{ TDBQCSTROR,         "Include" },
	{ TDBQCSTRRX,         "Regexp" },
};

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
	HASHFS_DEBUG("Starting transsaction");

	return (gboolean) tctdbtranbegin(db->tdb);
}

gboolean
hashfs_db_tran_commit (void)
{
	HASHFS_DEBUG("Comitting transsaction");

	return (gboolean) tctdbtrancommit(db->tdb);
}

gboolean
hashfs_db_tran_abort (void)
{
	HASHFS_DEBUG("Aborting transsaction");

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
	gchar *pkey, *md5;

	md5 = hashfs_md5_str(id);

	if (!g_strcmp0(prefix, "set"))
		pkey = g_strdup_printf("%s:%s:%s:%s", prefix, source, type, md5);
	else
		pkey = g_strdup_printf("%s:%s", prefix, md5);

	g_free(md5);

	return hashfs_db_entry_new_from_key(pkey);
}

hashfs_db_entry_t *
hashfs_db_entry_new_from_key (const gchar *pkey)
{
	hashfs_db_entry_t *entry;
	TCMAP *curdata;

	entry = g_new0(hashfs_db_entry_t, 1);
	entry->pkey = g_strdup(pkey);

	curdata = tctdbget(db->tdb, pkey, strlen(pkey));

	if (curdata != NULL)
		entry->data = curdata;
	else
		entry->data = tcmapnew();

	HASHFS_DEBUG("Created entry with pkey: %s", pkey);

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
hashfs_db_entry_lookup (hashfs_db_entry_t *entry, const gchar *key,
                        const gchar **out)
{
	const gchar *val;

	val = tcmapget2(entry->data, key);

	if (val == NULL)
		return FALSE;

	*out = val;

	return TRUE;
}

static gboolean
eval_cb (const GMatchInfo *info, GString *res, gpointer data)
{
	const gchar *tmp;
	gchar *match;

	match = g_match_info_fetch(info, 1);

	if (hashfs_db_entry_lookup(data, match, &tmp))
		g_string_append(res, tmp);

	g_free(match);

	return FALSE;
}

gchar *
hashfs_db_entry_format (hashfs_db_entry_t *entry, const gchar *format)
{
	GRegex *regex;
	GMatchInfo *match;
	GError *error = NULL;
	gchar *rval;

	regex = g_regex_new("\\$(\\w+)", 0, 0, &error);

	if (error) {
		HASHFS_DEBUG("Failed to create regex: %s", error->message);

		g_error_free(error);

		return NULL;
	}

	rval = g_regex_replace_eval(regex, format, -1, 0, 0, eval_cb, entry, NULL);

	g_regex_unref(regex);

	return rval;
}

const gchar *
hashfs_db_entry_pkey (hashfs_db_entry_t *entry)
{
	return entry->pkey;
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

static gint
hashfs_db_query_op (gchar *name)
{
	for (gint i = 0; i < LENGTH(query_cond); i++) {
		if (g_strcmp0(name, query_cond[i].name) == 0)
			return query_cond[i].op;
	}

	return -1;
}


hashfs_db_query_t *
hashfs_db_query_new (const gchar *querystr)
{
	GRegex *regex;
	GMatchInfo *match;
	GError *error = NULL;
	hashfs_db_query_t *query;

	query = g_new0(hashfs_db_query_t, 1);
	query->query = tctdbqrynew(db->tdb);

	regex = g_regex_new(HASHFS_QUERY_PATTERN, 0, 0, &error);

	if (error) {
		HASHFS_DEBUG("Failed to create regex: %s", error->message);

		g_error_free(error);

		return NULL;
	}

	g_regex_match(regex, querystr, 0, &match);
	while (g_match_info_matches(match)) {
		gchar *key = g_match_info_fetch(match, 1);
		gchar *func = g_match_info_fetch(match, 2);
		gchar *val = g_match_info_fetch(match, 3);
		gint op;

		op = hashfs_db_query_op(func);

		HASHFS_DEBUG("Adding query condition: %s.%s(%s)", key, func, val);

		if (op >= 0) {
			if (!g_strcmp0(key, "pkey"))
				tctdbqryaddcond(query->query, "", op, val);
			else
				tctdbqryaddcond(query->query, key, op, val);
		}

		g_free(key); g_free(func); g_free(val);
		g_match_info_next(match, NULL);
	}

	g_match_info_free(match);
	g_regex_unref(regex);

	return query;
}

void
hashfs_db_query_set_limit (hashfs_db_query_t *query, gint limit, gint skip)
{
	tctdbqrysetlimit(query->query, limit, skip);
}

void
hashfs_db_query_set_order (hashfs_db_query_t *query, gchar *key, gint mode)
{
	tctdbqrysetorder(query->query, key, mode);
}

hashfs_db_result_t *
hashfs_db_query_result (hashfs_db_query_t *query)
{
	hashfs_db_result_t *result;

	result = g_new0(hashfs_db_result_t, 1);
	result->list = tctdbqrysearch(query->query);

	return result;
}

void
hashfs_db_query_destroy (hashfs_db_query_t *query)
{
	if (query->query)
		tctdbqrydel(query->query);

	g_free(query);
}

gint
hashfs_db_result_num (hashfs_db_result_t *result)
{
	return (gint) tclistnum(result->list);
}

hashfs_db_entry_t *
hashfs_db_result_get_entry (hashfs_db_result_t *result, gint index)
{
	const gchar *key;
	gint len;

	key = tclistval(result->list, index, &len);

	return hashfs_db_entry_new_from_key(key);
}

void
hashfs_db_result_destroy (hashfs_db_result_t *result)
{
	if (result->list)
		tclistdel(result->list);

	g_free(result);
}

