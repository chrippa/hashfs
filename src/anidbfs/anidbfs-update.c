#include <stdio.h>
#include <stdlib.h>

#include <anidb.h>

void
dump_anime (anidb_session_t *session, char *name)
{
	anidb_result_t *res;
	anidb_dict_t *dict;
	char *aid;

	res = anidb_session_anime_by_name(session, "Neon Genesis Evangelion");


	if (anidb_result_get_code(res) == ANIDB_ANIME) {
		printf("Anime:\n");

		for (dict = res->value.dict; dict; dict = dict->next) {
			printf("%s: %s\n", dict->key, dict->value);
		}
	} else {
		printf("Fail\n");
	}
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

	if (anidb_result_get_code(res) == ANIDB_LOGIN_ACCEPTED) {
		if (anidb_result_get_str(res, &key)) {
			anidb_session_set_key(session, key);
		}

		dump_anime(session, "Neon Genesis Evangelion");

		anidb_session_logout(session);
	}

	anidb_result_unref(res);
	anidb_session_unref(session);

	return 0;
}
