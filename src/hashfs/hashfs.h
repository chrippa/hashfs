#ifndef _HASHFS_H
#define _HASHFS_H

#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <glib/gstdio.h>

struct hashfs_backend_St;
struct hashfs_backend_desc_St;
struct hashfs_file_St;

typedef struct hashfs_backend_St hashfs_backend_t;
typedef struct hashfs_backend_desc_St hashfs_backend_desc_t;
typedef struct hashfs_file_St hashfs_file_t;


struct hashfs_file_St {
	gchar *filename;
	gint64 size;
};

struct hashfs_backend_St {
	gpointer data;

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

gchar * hashfs_current_time (void);
gchar * hashfs_basename (gchar *name);


GKeyFile * hashfs_config_keyfile (void);
void hashfs_config_load (void);
void hashfs_config_save (void);
gboolean hashfs_config_property_exists (const gchar *group, const gchar *key);
void hashfs_config_property_lookup (const gchar *group, const gchar *key, gchar **out);
void hashfs_config_property_set (const gchar *group, const gchar *key, const gchar *value);

hashfs_file_t * hashfs_file_new (gchar *filename);
gint hashfs_file_hash_ed2k (hashfs_file_t *file, gchar **out);
gint hashfs_file_prop_lookup (hashfs_file_t *file, const gchar *key, gchar **out);
void hashfs_file_prop_set (hashfs_file_t *file, const gchar *key, gchar *value);

hashfs_backend_t * hashfs_backends_lookup (const gchar *name);
hashfs_backend_t * hashfs_backends_get (gint idx);
gint hashfs_backends_count (void);
void hashfs_backends_load (const gchar *path);
void hashfs_backends_destroy (void);

hashfs_backend_t * hashfs_backend_load (const gchar *path);
void hashfs_backend_init (hashfs_backend_t *backend);
void hashfs_backend_file (hashfs_backend_t *backend, hashfs_file_t *file);
void hashfs_backend_destroy (hashfs_backend_t *backend);

void hashfs_backend_config_register (hashfs_backend_t *backend, const gchar *key, gchar *defaultval);
void hashfs_backend_config_lookup (hashfs_backend_t *backend, const gchar *key, gchar **out);

#define HASHFS_BACKEND(shname, name, desc, setupfunc) \
	hashfs_backend_desc_t HASHFS_BACKEND_DESC = { \
		shname, \
		name, \
		desc, \
		(void (*)(hashfs_backend_t *))setupfunc, \
	};

#define HASHFS_LOG(fmt, ...) { \
	printf("%s  LOG ", hashfs_current_time()); \
	printf(fmt"\n", ## __VA_ARGS__); \
}

#define HASHFS_ERROR(fmt, ...) { \
	fprintf(stderr, "ERROR "fmt"\n", ## __VA_ARGS__); \
	exit(EXIT_FAILURE); \
}

#ifdef DEBUG
	#define HASHFS_DEBUG(fmt, ...) { \
		FILE *fd; \
		fd = g_fopen(g_build_filename(g_get_user_config_dir(), "hashfs", "debug.log", NULL), "at"); \
		fprintf(fd, "%s  DEBUG %s:%d: ", hashfs_current_time(), __FILE__, __LINE__); \
		fprintf(fd, fmt"\n", ## __VA_ARGS__); \
		fclose(fd); \
	}
#else
	#define HASHFS_DEBUG(...)
#endif


#endif
