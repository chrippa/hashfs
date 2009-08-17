#include <stdio.h>

#include <hashfs.h>
#include <anidb.h>

static void hashfs_anidb_setup (hashfs_backend_t *backend);
static gboolean hashfs_anidb_init (hashfs_backend_t *backend);
static void hashfs_anidb_destroy (hashfs_backend_t *backend);
static void hashfs_anidb_handle_file (hashfs_backend_t *backend,
                                      hashfs_file_t *file);

static void dump_result (anidb_result_t *result);

HASHFS_BACKEND("anidb", "AniDB",
              "AniDB hashing backend",
               hashfs_anidb_setup);

typedef struct hashfs_anidb_data_St {
	anidb_session_t *session;
} hashfs_anidb_data_t;


static void
hashfs_anidb_setup (hashfs_backend_t *backend)
{
	backend->funcs.init = hashfs_anidb_init;
	backend->funcs.file = hashfs_anidb_handle_file;
	backend->funcs.destroy = hashfs_anidb_destroy;

	hashfs_backend_config_register(backend, "username", "");
	hashfs_backend_config_register(backend, "password", "");
	hashfs_backend_config_register(backend, "local_port", "0");
}

static gboolean
hashfs_anidb_init (hashfs_backend_t *backend)
{
	anidb_session_t *session;
	anidb_result_t *res;
	hashfs_anidb_data_t *data;
	gchar *username, *password, *port_s, *key;
	gint port;
	gboolean rval;

	HASHFS_DEBUG("Connecting to AniDB");

	hashfs_backend_config_lookup(backend, "username", &username);
	hashfs_backend_config_lookup(backend, "password", &password);
	hashfs_backend_config_lookup(backend, "local_port", &port_s);

	port = atoi(port_s);

	HASHFS_DEBUG("username=%s, password=%s, port=%d", username, password, port);

	session = anidb_session_new("anidbfuse", "1", port);
	data = g_new0(hashfs_anidb_data_t, 1);
	data->session = session;
	backend->data = data;

	res = anidb_session_authenticate(session, username, password);

	if (anidb_result_get_code(res) == ANIDB_LOGIN_ACCEPTED) {
		if (anidb_result_get_str(res, &key)) {
			anidb_session_set_key(session, key);

			HASHFS_DEBUG("Successfully logged in");

			rval = TRUE;
		} else {
			HASHFS_DEBUG("Failed to get session key");

			rval = FALSE;
		}
	} else {
		HASHFS_DEBUG("Unable to login to AniDB servers");

		rval = FALSE;
	}

	g_free(username);
	g_free(password);
	g_free(port_s);
	anidb_result_unref(res);

	return rval;
}

static void
hashfs_anidb_destroy (hashfs_backend_t *backend)
{
	hashfs_anidb_data_t *data;

	g_return_if_fail(backend);

	data = (hashfs_anidb_data_t *) backend->data;

	if (data) {
		if (anidb_session_is_logged_in(data->session)) {
			HASHFS_DEBUG("Logging out");

			anidb_result_unref(anidb_session_logout(data->session));
		}

		anidb_session_unref(data->session);

		g_free(data);
	}
}

#define ANIDB_SET_PROP(res, file, prop) { \
	gchar *tmp; \
	if (anidb_result_dict_get((res), (prop), &tmp)) \
		hashfs_file_prop_set((file), (prop), tmp); \
}

static void
hashfs_anidb_handle_file (hashfs_backend_t *backend, hashfs_file_t *file)
{
	hashfs_anidb_data_t *data;
	const gchar *hash;

	g_return_if_fail(backend);

	data = (hashfs_anidb_data_t *) backend->data;

	g_return_if_fail(data);

	if (anidb_session_is_logged_in(data->session)) {
		if (hashfs_file_hash_ed2k(file, &hash)) {
			hashfs_set_t *set;
			anidb_result_t *res;
			gchar *tmp;

			HASHFS_DEBUG("ed2k hash: %s", hash);

			res = anidb_session_file_ed2k(data->session, file->size, (char *) hash);

			ANIDB_SET_PROP(res, file, "fid");
			ANIDB_SET_PROP(res, file, "eid");
			ANIDB_SET_PROP(res, file, "state");
			ANIDB_SET_PROP(res, file, "size");
			ANIDB_SET_PROP(res, file, "ed2k");
			ANIDB_SET_PROP(res, file, "md5");
			ANIDB_SET_PROP(res, file, "sha1");
			ANIDB_SET_PROP(res, file, "crc32");
			ANIDB_SET_PROP(res, file, "dub");
			ANIDB_SET_PROP(res, file, "sub");
			ANIDB_SET_PROP(res, file, "quality");
			ANIDB_SET_PROP(res, file, "source");
			ANIDB_SET_PROP(res, file, "audio");
			ANIDB_SET_PROP(res, file, "video");
			ANIDB_SET_PROP(res, file, "resolution");
			ANIDB_SET_PROP(res, file, "ext");
			ANIDB_SET_PROP(res, file, "duration");
			ANIDB_SET_PROP(res, file, "ep_number");
			ANIDB_SET_PROP(res, file, "ep_eng");
			ANIDB_SET_PROP(res, file, "ep_romaji");
			ANIDB_SET_PROP(res, file, "ep_kanji");

			if (anidb_result_dict_get(res, "anime_romaji", &tmp)) {
				set = hashfs_file_add_to_set(file, tmp, "anime");

				hashfs_set_prop_set(set, "romaji", tmp);

				if (anidb_result_dict_get(res, "anime_totalep", &tmp))
					hashfs_set_prop_set(set, "totalep", tmp);

				if (anidb_result_dict_get(res, "anime_lastep", &tmp))
					hashfs_set_prop_set(set, "lastep", tmp);

				if (anidb_result_dict_get(res, "anime_year", &tmp))
					hashfs_set_prop_set(set, "year", tmp);

				if (anidb_result_dict_get(res, "anime_type", &tmp))
					hashfs_set_prop_set(set, "type", tmp);

				if (anidb_result_dict_get(res, "anime_kanji", &tmp))
					hashfs_set_prop_set(set, "kanji", tmp);

				if (anidb_result_dict_get(res, "anime_eng", &tmp))
					hashfs_set_prop_set(set, "eng", tmp);

				if (anidb_result_dict_get(res, "anime_categories", &tmp))
					hashfs_set_prop_set(set, "categories", tmp);

			}

			anidb_result_unref(res);
		}
	}
}

static void
dump_result (anidb_result_t *result)
{
	gint n;
	gchar *str;
	anidb_dict_t *dict;

	switch (anidb_result_get_type(result)) {
		case ANIDB_RESULT_NULL:
			HASHFS_DEBUG("<result %p null>", result);
			break;

		case ANIDB_RESULT_STRING:
			anidb_result_get_str(result, &str);
			HASHFS_DEBUG("<result %p (string) \"%s\">", result, str);

			break;

		case ANIDB_RESULT_NUMBER:
			anidb_result_get_int(result, &n);
			HASHFS_DEBUG("<result %p (int) \"%d\">", result, n);

			break;

		case ANIDB_RESULT_DICT:
			HASHFS_DEBUG("<result %p (dict)", result);

			for (dict = anidb_result_get_dict(result); dict; dict = anidb_dict_next(dict)) {
				HASHFS_DEBUG("  %s = \"%s\"", dict->key, dict->value);

			}

			HASHFS_DEBUG(">");

			break;
	}
}
