#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <netdb.h>
#include <malloc.h>
#include <string.h>
#include <sys/socket.h>

#include "util.h"
#include "anidb.h"

static char *
get_part(char *src, int delim, int idx)
{
	int i = 0, j = 0;
	char buf[1024];

	while (*src != '\0') {
		if (i == idx && *src != delim) {
			buf[j] = *src;
			j++;
		}

		if (*src == delim)
			i++;

		src++;
	}

	buf[j++] = '\0';

	return strdup(buf);
}

#define DICT_PART_ADD(result, data, num, name) \
	anidb_result_dict_set((result), (name), get_part((data), '|', (num)))


static void
handler_auth_accepted (anidb_result_t *result, char *data)
{
	char buf[10];

	sscanf(data, "200 %s", &buf);

	anidb_result_set_str(result, buf);
}


static void
handler_auth_logout (anidb_result_t *result, char *data)
{
	anidb_result_dict_set(result, "test", "hej");
	anidb_result_dict_set(result, "test1", "hej");
	anidb_result_dict_set(result, "test2", "hej");
	anidb_result_dict_set(result, "test3", "hej");
}


static void
handler_anime (anidb_result_t *result, char *data)
{
	char *anime;


	anime = data + 10;

	DICT_PART_ADD(result, anime,  0, "aid");
	DICT_PART_ADD(result, anime,  1, "eps");
	DICT_PART_ADD(result, anime,  2, "ep_count");
	DICT_PART_ADD(result, anime,  3, "special_cnt");
	DICT_PART_ADD(result, anime,  4, "rating");
	DICT_PART_ADD(result, anime,  5, "votes");
	DICT_PART_ADD(result, anime,  6, "tmprating");
	DICT_PART_ADD(result, anime,  7, "tmpvotes");
	DICT_PART_ADD(result, anime,  8, "rating_average");
	DICT_PART_ADD(result, anime,  9, "reviews");
	DICT_PART_ADD(result, anime, 10, "year");
	DICT_PART_ADD(result, anime, 11, "type");
	DICT_PART_ADD(result, anime, 12, "romaji");
	DICT_PART_ADD(result, anime, 13, "kanji");
	DICT_PART_ADD(result, anime, 14, "english");
	DICT_PART_ADD(result, anime, 15, "other");
	DICT_PART_ADD(result, anime, 16, "short_names");
	DICT_PART_ADD(result, anime, 17, "synonyms");
	DICT_PART_ADD(result, anime, 18, "category_list");
}

anidb_result_handler_t anidb_handlers[] = {
	/* AUTH */
	{ ANIDB_LOGIN_ACCEPTED, handler_auth_accepted },
	{ ANIDB_LOGGED_OUT,     handler_auth_logout },

	/* ANIME */
	{ ANIDB_ANIME,          handler_anime },
};
