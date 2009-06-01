#include <stdio.h>
#include <stdlib.h>

#include <anidb.h>

void
dump_result (anidb_result_t *result)
{
	int n;
	char *str;
	anidb_dict_t *dict;

	switch (anidb_result_get_type(result)) {
		case ANIDB_RESULT_NULL:
			printf("<result %p null>\n", result);
			break;

		case ANIDB_RESULT_STRING:
			anidb_result_get_str(result, &str);
			printf("<result %p (string) \"%s\">\n", result, str);

			break;

		case ANIDB_RESULT_NUMBER:
			anidb_result_get_int(result, &n);
			printf("<result %p (int) \"%d\">\n", result, n);

			break;

		case ANIDB_RESULT_DICT:
			printf("<result %p (dict)\n", result);

			for (dict = anidb_result_get_dict(result); dict; dict = anidb_dict_next(dict)) {
				printf("  %s = \"%s\"", dict->key, dict->value);

				if (anidb_dict_next(dict))
					printf(",\n");
			}

			printf(">\n");

			break;
	}
}


void
dump_anime (anidb_session_t *session, char *name)
{
	anidb_result_t *res;

	res = anidb_session_anime_name(session, name);

	if (anidb_result_get_code(res) == ANIDB_ANIME) {
		dump_result(res);
	}

	anidb_result_unref(res);
}

void
dump_animedesc (anidb_session_t *session, int id, int part)
{
	anidb_result_t *res;

	res = anidb_session_animedesc(session, id, part);

	if (anidb_result_get_code(res) == ANIDB_ANIME_DESCRIPTION) {
		dump_result(res);
	}

	anidb_result_unref(res);
}

void
dump_episode (anidb_session_t *session, int id)
{
	anidb_result_t *res;

	res = anidb_session_episode_id(session, id);

	if (anidb_result_get_code(res) == ANIDB_EPISODE) {
		dump_result(res);
	}

	anidb_result_unref(res);
}

void
dump_file (anidb_session_t *session, int id)
{
	anidb_result_t *res;

//	res = anidb_session_file_ed2k(session, 44255118, "fa8313487b58f37f72500e8c5afeb3bf");
	res = anidb_session_file_id(session, id);

	if (anidb_result_get_code(res) == ANIDB_FILE) {
		dump_result(res);
	}

	anidb_result_unref(res);
}


void
dump_group (anidb_session_t *session, char *name)
{
	anidb_result_t *res;

	res = anidb_session_group_name(session, name);

	if (anidb_result_get_code(res) == ANIDB_GROUP) {
		dump_result(res);
	}

	anidb_result_unref(res);
}


int
main (int argc, char *argv[])
{
	anidb_session_t *session;
	anidb_result_t *res;
	char *key;

	if (argc < 3) {
		printf("Usage: %s <username> <password>\n", argv[0]);
		exit(0);
	}

	session = anidb_session_new("anidbfs", "1");

	res = anidb_session_authenticate(session, argv[1], argv[2]);
	dump_result(res);

	if (anidb_result_get_code(res) == ANIDB_LOGIN_ACCEPTED) {
		if (anidb_result_get_str(res, &key)) {
			anidb_session_set_key(session, key);
		}

		dump_anime(session, "Neon Genesis Evangelion");
		dump_group(session, "Chrippa Crapsubs");
		dump_animedesc(session, 23, 0);
		dump_episode(session, 21346);
		dump_file(session, 43698);

		dump_result(anidb_session_logout(session));
	}

	anidb_result_unref(res);
	anidb_session_unref(session);

	return 0;
}
