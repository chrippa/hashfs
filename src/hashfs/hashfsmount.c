
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#define FUSE_USE_VERSION 26
#include <fuse.h>

#include "hashfs.h"

GHashTable *mounts;

static void hashfs_mounts_add (const gchar *path, const gchar *schema);
static gchar * hashfs_mounts_get (const gchar *path);
static gboolean hashfs_mounts_exists (const gchar *path);
static void hashfs_mounts_init (void);
static void hashfs_mounts_destroy (void);

static void
test_query (gchar *querystr)
{
	hashfs_db_query_t *query;
	hashfs_db_result_t *result;

	query = hashfs_db_query_new(querystr);
//	result = hashfs_db_query_result(query);
	result = hashfs_db_query_group(query, "group");

	printf("num results: %d\n", hashfs_db_result_num(result));

	for (gint i = 0; i < hashfs_db_result_num(result); i++) {
		hashfs_db_entry_t *entry;

		entry = hashfs_db_result_get_entry(result, i);
		printf("result: %s\n", hashfs_db_entry_pkey(entry));

		hashfs_db_entry_destroy(entry);
	}


	hashfs_db_query_destroy(query);
	hashfs_db_result_destroy(result);
}

static void
free_entry_list (GList *entries)
{
	GList *item;

	g_return_if_fail(entries != NULL);

	for (item = g_list_first(entries); item; item = g_list_next(item)) {
		hashfs_db_entry_destroy(item->data);
	}

	g_list_free(entries);
}

static GHashTable *
parse_schema (gchar *schema)
{
	GRegex *regex;
	GMatchInfo *match;
	GError *error = NULL;
	GHashTable *hash;

	hash = g_hash_table_new(g_str_hash, g_str_equal);
	regex = g_regex_new("(\\w+)=\"(.*?)\"", 0, 0, &error);

	if (error) {
		HASHFS_DEBUG("Failed to create regex: %s", error->message);

		g_error_free(error);

		return NULL;
	}

	g_regex_match(regex, schema, 0, &match);
	while (g_match_info_matches(match)) {
		gchar *key = g_match_info_fetch(match, 1);
		gchar *val = g_match_info_fetch(match, 2);

		g_hash_table_insert(hash, key, val);

		g_match_info_next(match, NULL);
	}

	g_match_info_free(match);
	g_regex_unref(regex);

	return hash;
}

static hashfs_db_entry_t *
resolve_path (gchar *squery, gchar *groupby, gchar *sdisplay, gchar *path)
{
	hashfs_db_query_t *query;
	hashfs_db_result_t *result;
	hashfs_db_entry_t *entry;

	query = hashfs_db_query_new(squery);
	if (groupby != NULL)
		result = hashfs_db_query_group(query, groupby);
	else
		result = hashfs_db_query_result(query);
	entry = NULL;

	printf("resolve_path, q=%s, num_results=%d\n", squery, hashfs_db_result_num(result));

	for (gint i = 0; i < hashfs_db_result_num(result); i++) {
		gchar *display;

		entry = hashfs_db_result_get_entry(result, i);
		display = hashfs_db_entry_format(entry, sdisplay);

		if (g_strcmp0(path, display) == 0) {
			g_free(display);
			break;
		}

		g_free(display);
		hashfs_db_entry_destroy(entry);
		entry = NULL;
	}

	hashfs_db_query_destroy(query);
	hashfs_db_result_destroy(result);

	return entry;
}

static gboolean
eval_cb (const GMatchInfo *info, GString *res, gpointer data)
{
	gchar *var;
	gchar *n;
	gchar *key;
	gint nth;

	var = g_match_info_fetch(info, 1);
	n = g_match_info_fetch(info, 2);
	key = g_match_info_fetch(info, 3);
	nth = atoi(n);

	if (g_strcmp0(var, "prev") == 0) {
		GList *last = g_list_last(data);
		GList *item = g_list_nth_prev(last, nth - 1);

		if (item) {
			const gchar *val;
			hashfs_db_entry_t *entry = item->data;

			if (g_strcmp0(key, "pkey") == 0) {
				val = hashfs_db_entry_pkey(entry);
				g_string_append(res, val);
			} else {
				if (hashfs_db_entry_lookup(entry, key, &val))
					g_string_append(res, val);
			}
		}
	}

	g_free(var);
	g_free(n);
	g_free(key);

	return FALSE;
}

static gchar *
prepare_query (gchar *query, GList *entries)
{
	GRegex *regex;
	GMatchInfo *match;
	GError *error = NULL;
	gchar *rval;

	regex = g_regex_new("\\$(\\w+)\\[(\\d+)\\]\\.(\\w+)", 0, 0, &error);

	if (error) {
		HASHFS_DEBUG("Failed to create regex: %s", error->message);

		g_error_free(error);

		return NULL;
	}

	rval = g_regex_replace_eval(regex, query, -1, 0, 0, eval_cb, entries, NULL);

	g_regex_unref(regex);

	return rval;
}

static GList *
resolve_paths (const gchar *path)
{
	gchar **sschema, **spath;
	gchar *schema;
	GList *entries;

	spath = g_strsplit(path, "/", 0);
	schema = hashfs_mounts_get(spath[1]);

	sschema = g_strsplit(schema, "/", 0);
	entries = NULL;

	for (gint i = 2; i < g_strv_length(spath); i++) {
		GHashTable *hash = parse_schema(sschema[i]);
		gchar *q = g_hash_table_lookup(hash, "q");
		gchar *d = g_hash_table_lookup(hash, "d");
		gchar *g = g_hash_table_lookup(hash, "g");
		gchar *query = prepare_query(q, entries);
		hashfs_db_entry_t *entry = resolve_path(query, g, d, spath[i]);

		if (entry) {
			entries = g_list_append(entries, entry);
		} else {
			if (entries)
				free_entry_list(entries);

			entries = NULL;
		}

		g_free(q); g_free(d); g_free(query);
		g_hash_table_unref(hash);
	}

	g_strfreev(sschema);
	g_strfreev(spath);

	return entries;
}

static void
hashfs_mounts_add (const gchar *path, const gchar *schema)
{
	g_hash_table_insert(mounts, g_strdup(path), g_strdup_printf("/%s/%s", path, schema));
}

static gchar *
hashfs_mounts_get (const gchar *path)
{
	return g_hash_table_lookup(mounts, path);
}

static gboolean
hashfs_mounts_exists (const gchar *path)
{
	gchar *val;

	val = hashfs_mounts_get(path);

	if (val)
		return TRUE;

	return FALSE;
}

static void
hashfs_mounts_init (void)
{
	if (!mounts)
		mounts = g_hash_table_new(g_str_hash, g_str_equal);
}

static void
hashfs_mounts_destroy (void)
{
	if (mounts)
		g_hash_table_unref(mounts);
}

static gint
hashfs_fuse_listattrs (const gchar *path, struct stat *stats)
{
	GList *entries;
	gint rval;

	entries = resolve_paths(path);

	if (entries) {
		GList *item = g_list_last(entries);
		hashfs_db_entry_t *entry = item->data;
		const gchar *pkey;

		pkey = hashfs_db_entry_pkey(entry);

		if (g_strrstr(pkey, "set:")) {
			stats->st_mode = S_IFDIR | 0755;
			stats->st_nlink = 2;
		} else {
			struct stat realstats;
			const gchar *realpath;

			if (hashfs_db_entry_lookup(entry, "path", &realpath)) {
				stat(realpath, &realstats);

				stats->st_mode = S_IFREG | 0444;
				stats->st_nlink = 1;
				stats->st_size = realstats.st_size;
			}
		}

		rval = 0;

		free_entry_list(entries);
	} else {
		rval = -ENOENT;
	}


	return rval;
}

static gint
hashfs_fuse_getattr (const gchar *path, struct stat *stats)
{
	gint res;

	printf("getattr: %s\n", path);

	memset(stats, 0, sizeof(struct stat));
	res = 0;

	if (g_strcmp0(path, "/") == 0 || hashfs_mounts_exists(path + 1)) {
		stats->st_mode = S_IFDIR | 0555;
		stats->st_nlink = 2;
	} else {
		res = hashfs_fuse_listattrs(path, stats);
	}

	return res;
}

static void
hashfs_fuse_listdir (const gchar *squery, const gchar *groupby,
                     const gchar *format, gpointer buf,
                     fuse_fill_dir_t filler)
{
	hashfs_db_query_t *query;
	hashfs_db_result_t *result;

	query = hashfs_db_query_new(squery);

	if (groupby != NULL)
		result = hashfs_db_query_group(query, groupby);
	else
		result = hashfs_db_query_result(query);

	printf("listdir, num results: %d, q=%s, d=%s\n", hashfs_db_result_num(result), squery, format);
	for (gint i = 0; i < hashfs_db_result_num(result); i++) {
		hashfs_db_entry_t *entry;
		gchar *formatted;

		entry = hashfs_db_result_get_entry(result, i);
		formatted = hashfs_db_entry_format(entry, format);

		filler(buf, formatted, NULL, 0);

		g_free(formatted);
		hashfs_db_entry_destroy(entry);
	}

	hashfs_db_query_destroy(query);
	hashfs_db_result_destroy(result);
}

static gint
hashfs_fuse_readdir (const gchar *path, gpointer buf, fuse_fill_dir_t filler,
                     off_t offset, struct fuse_file_info *fi)
{
	printf("readdir: %s\n", path);

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	if (g_strcmp0(path, "/") == 0) {
		GList *keys, *item;
		keys = g_hash_table_get_keys(mounts);

		for (item = g_list_first(keys); item; item = g_list_next(item)) {
			gchar *key = item->data;

			filler(buf, key, NULL, 0);
		}

		g_list_free(keys);
	} else {
		gchar **sschema, **spath;
		gchar *schema;
		GList *entries;

		spath = g_strsplit(path, "/", 0);
		schema = hashfs_mounts_get(spath[1]);

		sschema = g_strsplit(schema, "/", 0);
		entries = NULL;

		for (gint i = 1; i < g_strv_length(spath); i++) {
			GHashTable *hash = parse_schema(sschema[i+1]);
			gchar *q = g_hash_table_lookup(hash, "q");
			gchar *d = g_hash_table_lookup(hash, "d");
			gchar *g = g_hash_table_lookup(hash, "g");
			gchar *query = prepare_query(q, entries);

			if ((i + 1) == g_strv_length(spath)) {
				hashfs_fuse_listdir(query, g, d, buf, filler);
				break;
			}

			hashfs_db_entry_t *entry = resolve_path(query, g, d, spath[i+1]);

			if (entry)
				entries = g_list_append(entries, entry);


			g_free(q); g_free(d); g_free(query);
			g_hash_table_unref(hash);
		}

		g_strfreev(sschema);
		g_strfreev(spath);

		if (entries)
			free_entry_list(entries);
	}

	return 0;
}

static gint
hashfs_fuse_open (const gchar *path, struct fuse_file_info *fi)
{
	GList *entries, *item;
	gint rval = 0;
	const gchar *realpath;
	hashfs_db_entry_t *entry;

	printf("open: %s\n", path);

	entries = resolve_paths(path);

	if (!entries)
		return -ENOENT;

	if ((fi->flags & 3) != O_RDONLY)
		return -EACCES;

	item = g_list_last(entries);
	entry = item->data;

	if (hashfs_db_entry_lookup(entry, "path", &realpath)) {
		fi->fh = open(realpath, fi->flags);
	}

	free_entry_list(entries);

	return 0;
}

static gint
hashfs_fuse_read (const gchar *path, gchar *buf, size_t size, off_t offset,
                  struct fuse_file_info *fi)
{
	gint rval;

	printf("read %s %d %d\n", path, size, offset);

	lseek(fi->fh, offset, SEEK_SET);

	return read(fi->fh, buf, size);
}

static gint
hashfs_fuse_release (const gchar *path, struct fuse_file_info *fi)
{
	printf("close %s\n", path);

	return close(fi->fh);
}

static struct fuse_operations hashfs_fuse_operations = {
	.getattr = hashfs_fuse_getattr,
	.readdir = hashfs_fuse_readdir,
	.open = hashfs_fuse_open,
	.read = hashfs_fuse_read,
	.release = hashfs_fuse_release,
};

gint
main (gint argc, gchar **argv)
{
	gint rval;

	hashfs_config_init();
	hashfs_db_init(TRUE);

	hashfs_mounts_init();
	hashfs_mounts_add("by-name", "q=\"pkey.BeginsWith(set:anidb:anime)\", d=\"$romaji [$eps]\"/"
	                             "q=\"pkey.BeginsWith(file:), anime.Equals($prev[1].pkey)\", d=\"$anime_romaji - $ep_number [$group_name].$ext\"");
	hashfs_mounts_add("by-group", "q=\"pkey.BeginsWith(set:anidb:group)\", d=\"$name\")/"
	                              "q=\"pkey.BeginsWith(file:), group.Equals($prev[1].pkey)\", g=\"anime\", d=\"$romaji\")/"
	                              "q=\"pkey.BeginsWith(file:), group.Equals($prev[2].pkey), anime.Equals($prev[1].pkey)\", d=\"$anime_romaji - $ep_number [$group_name].$ext\"");

	rval = fuse_main(argc, argv, &hashfs_fuse_operations, NULL);


	hashfs_config_destroy();
	hashfs_db_destroy();
	hashfs_mounts_destroy();

	return rval;
}
