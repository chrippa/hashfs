#ifndef _HASHFS_H
#define _HASHFS_H

#include <stdio.h>
#include <stdlib.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <gmodule.h>

#include <tcutil.h>
#include <tctdb.h>

struct hashfs_backend_St;
struct hashfs_backend_desc_St;
struct hashfs_db_St;
struct hashfs_db_entry_St;
struct hashfs_db_result_St;
struct hashfs_db_query_St;
struct hashfs_file_St;
struct hashfs_set_St;

typedef struct hashfs_backend_St hashfs_backend_t;
typedef struct hashfs_backend_desc_St hashfs_backend_desc_t;
typedef struct hashfs_db_St hashfs_db_t;
typedef struct hashfs_db_entry_St hashfs_db_entry_t;
typedef struct hashfs_db_result_St hashfs_db_result_t;
typedef struct hashfs_db_query_St hashfs_db_query_t;
typedef struct hashfs_file_St hashfs_file_t;
typedef struct hashfs_set_St hashfs_set_t;

struct hashfs_backend_St {
	gpointer data;
	GList *globs;
	GModule *module;

	struct {
		gboolean (*init)(hashfs_backend_t *);
		void (*file)(hashfs_backend_t *, hashfs_file_t *);
		void (*destroy)(hashfs_backend_t *);
	} funcs;

	hashfs_backend_desc_t *desc;
};

struct hashfs_backend_desc_St {
	const gchar *shortname;
	const gchar *name;
	const gchar *description;
	void (*setup_func)(hashfs_backend_t *);
};

struct hashfs_db_St {
	TCTDB *tdb;
	gchar *path;
	gint flags;
};

struct hashfs_db_entry_St {
	TCMAP *data;
	gchar *pkey;
};

struct hashfs_db_result_St {
	TCLIST *list;
};

struct hashfs_db_query_St {
	TDBQRY *query;
};


struct hashfs_file_St {
	gchar *filename;
	gint64 size;
	GList *sets;
	hashfs_backend_t *backend;
	hashfs_db_entry_t *entry;

	/* Hashes */
	gchar *ed2k;
	gchar *md5;
};

struct hashfs_set_St {
	gchar name[256];
	hashfs_db_entry_t *entry;
};

/* Config */
GKeyFile * hashfs_config_keyfile (void);
void hashfs_config_init (void);
void hashfs_config_destroy (void);
gboolean hashfs_config_property_exists (const gchar *group, const gchar *key);
void hashfs_config_property_lookup (const gchar *group, const gchar *key, gchar **out);
void hashfs_config_property_set (const gchar *group, const gchar *key, const gchar *value);


/* Database */
gboolean hashfs_db_init (gboolean readonly);
void hashfs_db_destroy (void);
gchar * hashfs_db_error (void);

gboolean hashfs_db_tran_abort (void);
gboolean hashfs_db_tran_begin (void);
gboolean hashfs_db_tran_commit (void);


/* Database entry */
hashfs_db_entry_t * hashfs_db_entry_new (const gchar *prefix, const gchar *id, const gchar *source, const gchar *type);
hashfs_db_entry_t * hashfs_db_entry_new_from_key (const gchar *key);
gboolean hashfs_db_entry_lookup (hashfs_db_entry_t *entry, const gchar *key, const gchar **out);
gchar * hashfs_db_entry_format (hashfs_db_entry_t *entry, const gchar *format);
const gchar * hashfs_db_entry_pkey (hashfs_db_entry_t *entry);
void hashfs_db_entry_set (hashfs_db_entry_t *entry, const gchar *key, const gchar *value);
gboolean hashfs_db_entry_put (hashfs_db_entry_t *entry);
void hashfs_db_entry_destroy (hashfs_db_entry_t *entry);


/* Database query */
hashfs_db_query_t * hashfs_db_query_new (const gchar *querystr);
hashfs_db_result_t * hashfs_db_query_result (hashfs_db_query_t *query);
hashfs_db_result_t * hashfs_db_query_group (hashfs_db_query_t *query, const gchar *groupby);
void hashfs_db_query_set_limit (hashfs_db_query_t *query, gint limit, gint skip);
void hashfs_db_query_set_order (hashfs_db_query_t *query, gchar *key, gint mode);
void hashfs_db_query_destroy (hashfs_db_query_t *query);


/* Database result list */
gint hashfs_db_result_num (hashfs_db_result_t *result);
hashfs_db_entry_t * hashfs_db_result_get_entry (hashfs_db_result_t *result, gint index);
void hashfs_db_result_destroy (hashfs_db_result_t *result);


/* Set */
hashfs_set_t * hashfs_set_new (const gchar *name, const gchar *source, const gchar *type);
gboolean hashfs_set_prop_lookup (hashfs_set_t *file, const gchar *key, const gchar **out);
void hashfs_set_prop_set (hashfs_set_t *file, const gchar *key, const gchar *value);
void hashfs_set_destroy (hashfs_set_t *set);


/* File */
hashfs_file_t * hashfs_file_new (const gchar *filename, hashfs_backend_t *backend);
gboolean hashfs_file_hash_ed2k (hashfs_file_t *file, const gchar **out);
gboolean hashfs_file_hash_md5 (hashfs_file_t *file, const gchar **out);
gboolean hashfs_file_prop_lookup (hashfs_file_t *file, const gchar *key, const gchar **out);
void hashfs_file_prop_set (hashfs_file_t *file, const gchar *key, const gchar *value);
hashfs_set_t * hashfs_file_add_to_set (hashfs_file_t *file, const gchar *name, const gchar *type);
void hashfs_file_destroy (hashfs_file_t *file);


/* Backend manager */
hashfs_backend_t * hashfs_backends_lookup (const gchar *name);
hashfs_backend_t * hashfs_backends_get (gint idx);
gint hashfs_backends_count (void);
void hashfs_backends_load (const gchar *path);
void hashfs_backends_destroy (void);


/* Backend */
hashfs_backend_t * hashfs_backend_load (const gchar *path);
void hashfs_backend_init (hashfs_backend_t *backend);
void hashfs_backend_file (hashfs_backend_t *backend, hashfs_file_t *file);
void hashfs_backend_destroy (hashfs_backend_t *backend);
void hashfs_backend_glob_set (hashfs_backend_t *backend, ...);
gboolean hashfs_backend_glob_try (hashfs_backend_t *backend, const gchar *filename);
void hashfs_backend_config_register (hashfs_backend_t *backend, const gchar *key, const gchar *defaultval);
void hashfs_backend_config_lookup (hashfs_backend_t *backend, const gchar *key, gchar **out);


/* Utils */
gchar * hashfs_current_time (void);
gchar * hashfs_basename (const gchar *name);
gchar * hashfs_md5_str (const gchar *str);


#define HASHFS_BACKEND(shname, name, desc, setupfunc) \
	hashfs_backend_desc_t HASHFS_BACKEND_DESC = { \
		shname, \
		name, \
		desc, \
		(void (*)(hashfs_backend_t *))setupfunc, \
	};

#define HASHFS_LOG(fmt, ...) { \
	gchar *log_time = hashfs_current_time(); \
	printf("%s  LOG ", log_time); \
	printf(fmt"\n", ## __VA_ARGS__); \
	g_free(log_time); \
}

#define HASHFS_ERROR(fmt, ...) { \
	fprintf(stderr, "ERROR "fmt"\n", ## __VA_ARGS__); \
	exit(EXIT_FAILURE); \
}

#ifdef DEBUG
	#define HASHFS_DEBUG(fmt, ...) { \
		gchar *log_path = g_build_filename(g_get_user_config_dir(), "hashfs", "debug.log", NULL); \
		gchar *log_time = hashfs_current_time(); \
		FILE *fd = g_fopen(log_path, "at"); \
		fprintf(fd, "%s  DEBUG %s:%d: ", log_time, __FILE__, __LINE__); \
		fprintf(fd, fmt"\n", ## __VA_ARGS__); \
		fclose(fd); \
		g_free(log_path); \
		g_free(log_time); \
	}
#else
	#define HASHFS_DEBUG(...)
#endif


#endif
