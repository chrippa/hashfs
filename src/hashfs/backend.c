#include <sys/stat.h>
#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gmodule.h>

#include "hashfs.h"

static GList *backends;

hashfs_backend_t *
hashfs_backend_load (const gchar *path)
{
	GModule *module;
	gpointer sym;
	hashfs_backend_t *backend;
	hashfs_backend_desc_t *desc;

	HASHFS_DEBUG("Loading module (%s)", path);

	module = g_module_open(path, 0);

	if (!module) {
		HASHFS_DEBUG("Failed to load module: %s", g_module_error());

		return NULL;
	}

	if (!g_module_symbol(module, "HASHFS_BACKEND_DESC", (gpointer) &sym)) {
		HASHFS_DEBUG("Failed to lookup symbol: %s", g_module_error());

		g_module_close(module);

		return NULL;
	}


	desc = (hashfs_backend_desc_t *) sym;

	if (!desc) {
		HASHFS_DEBUG("HASHFS_BACKEND_DESC == NULL");

		g_module_close(module);

		return NULL;
	}

	if (hashfs_backends_lookup(desc->shortname)) {
		HASHFS_DEBUG("Already loaded backend %s", desc->shortname);

		g_module_close(module);

		return NULL;
	}

	backend = g_new0(hashfs_backend_t, 1);
	backend->desc = desc;

	desc->setup_func(backend);

	HASHFS_DEBUG("Backend successfully loaded: %s - %s - %s", desc->shortname,
	             desc->name, desc->description);

	return backend;
}

void
hashfs_backends_load (const gchar *path)
{
	GDir *dir;
	GError *error = NULL;
	const gchar *filename;
	gchar *pattern, *glob, *fullpath;
	hashfs_backend_t *backend;

	dir = g_dir_open(path, 0, &error);

	if (error) {
		HASHFS_DEBUG("Failed to open directory (%s): %s", path, error->message);
		g_error_free(error);

		return;
	}

	glob = g_module_build_path(path, "hashfs_*");
	pattern = g_path_get_basename(glob);

	HASHFS_DEBUG("Loooking for backends in: %s", glob);

	while ((filename = g_dir_read_name(dir))) {

		if (!g_pattern_match_simple(pattern, filename))
			continue;

		fullpath = g_build_filename(path, filename, NULL);

		if (!g_file_test(fullpath, G_FILE_TEST_IS_REGULAR)) {
			g_free(fullpath);
			continue;
		}

		backend = hashfs_backend_load(fullpath);

		if (backend) {
			backends = g_list_append(backends, backend);
		}
	}

	g_dir_close(dir);
}

hashfs_backend_t *
hashfs_backends_lookup (const gchar *name)
{
	hashfs_backend_t *backend;
	GList *item;

	for (item = g_list_first(backends); item; item = g_list_next(item)) {
		backend = item->data;

		if (!g_strcmp0(name, backend->desc->shortname))
			return backend;
	}

	return NULL;
}

void
hashfs_backends_destroy (void)
{
	hashfs_backend_t *backend;
	GList *item;

	for (item = g_list_first(backends); item; item = g_list_next(item)) {
		backend = item->data;

		hashfs_backend_destroy(backend);
	}

	g_list_free(backends);
}

hashfs_file_t *
hashfs_file_new (gchar *filename)
{
	hashfs_file_t *file;
	struct stat info;

	stat(filename, &info);

	file = g_new0(hashfs_file_t, 1);
	file->filename = filename;
	file->size = (gint64) info.st_size;

	return file;
}
void
hashfs_backend_init (hashfs_backend_t *backend)
{
	if (backend->funcs.init) {
		backend->funcs.init(backend);
	}
}

void
hashfs_backend_file (hashfs_backend_t *backend, hashfs_file_t *file)
{
	if (backend->funcs.file) {
		backend->funcs.file(backend, file);
	}
}

void
hashfs_backend_destroy (hashfs_backend_t *backend)
{
	if (backend->funcs.destroy) {
		backend->funcs.destroy(backend);
	}

	g_free(backend);
}

void
hashfs_backend_config_register (hashfs_backend_t *backend, const gchar *key,
                                gchar *defaultval)
{
	if (!hashfs_config_property_exists(backend->desc->shortname, key)) {
		hashfs_config_property_set(backend->desc->shortname, key, defaultval);
	}
}

void
hashfs_backend_config_lookup (hashfs_backend_t *backend, const gchar *key,
                              gchar **out)
{
	hashfs_config_property_lookup(backend->desc->shortname, key, out);
}
