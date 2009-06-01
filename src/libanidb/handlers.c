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
	/* pass */
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

static void
handler_animedesc (anidb_result_t *result, char *data)
{
	char *desc;

	desc = data + 14;

	DICT_PART_ADD(result, desc, 0, "part");
	DICT_PART_ADD(result, desc, 1, "max_parts");
	DICT_PART_ADD(result, desc, 2, "description");
}

static void
handler_episode (anidb_result_t *result, char *data)
{
	char *episode;

	episode = data + 12;

	DICT_PART_ADD(result, episode, 0, "eid");
	DICT_PART_ADD(result, episode, 1, "aid");
	DICT_PART_ADD(result, episode, 2, "length");
	DICT_PART_ADD(result, episode, 3, "rating");
	DICT_PART_ADD(result, episode, 4, "votes");
	DICT_PART_ADD(result, episode, 5, "epno");
	DICT_PART_ADD(result, episode, 6, "eng");
	DICT_PART_ADD(result, episode, 7, "romaji");
	DICT_PART_ADD(result, episode, 8, "kanji");
	DICT_PART_ADD(result, episode, 9, "aired");
}

static void
handler_file (anidb_result_t *result, char *data)
{
	char *file;

	file = data + 9;

	DICT_PART_ADD(result, file,  0, "fid");
	DICT_PART_ADD(result, file,  1, "aid");
	DICT_PART_ADD(result, file,  2, "eid");
	DICT_PART_ADD(result, file,  3, "gid");
	DICT_PART_ADD(result, file,  4, "lid");
	DICT_PART_ADD(result, file,  5, "state");
	DICT_PART_ADD(result, file,  6, "size");
	DICT_PART_ADD(result, file,  7, "ed2k");
	DICT_PART_ADD(result, file,  8, "md5");
	DICT_PART_ADD(result, file,  9, "sha1");
	DICT_PART_ADD(result, file, 10, "crc32");
	DICT_PART_ADD(result, file, 11, "dub");
	DICT_PART_ADD(result, file, 12, "sub");
	DICT_PART_ADD(result, file, 13, "quality");
	DICT_PART_ADD(result, file, 14, "source");
	DICT_PART_ADD(result, file, 15, "audio");
	DICT_PART_ADD(result, file, 16, "video");
	DICT_PART_ADD(result, file, 17, "resolution");
	DICT_PART_ADD(result, file, 18, "ext");
	DICT_PART_ADD(result, file, 19, "duration");

	DICT_PART_ADD(result, file, 20, "group_name");
	DICT_PART_ADD(result, file, 21, "group_short");

	DICT_PART_ADD(result, file, 22, "ep_number");
	DICT_PART_ADD(result, file, 23, "ep_eng");
	DICT_PART_ADD(result, file, 24, "ep_romaji");
	DICT_PART_ADD(result, file, 25, "ep_kanji");

	DICT_PART_ADD(result, file, 26, "anime_totalep");
	DICT_PART_ADD(result, file, 27, "anime_lastep");
	DICT_PART_ADD(result, file, 28, "anime_year");
	DICT_PART_ADD(result, file, 29, "anime_type");
	DICT_PART_ADD(result, file, 30, "anime_romaji");
	DICT_PART_ADD(result, file, 31, "anime_kanji");
	DICT_PART_ADD(result, file, 32, "anime_eng");
	DICT_PART_ADD(result, file, 33, "anime_categories");
}

static void
handler_group (anidb_result_t *result, char *data)
{
	char *group;

	group = data + 10;

	DICT_PART_ADD(result, group,  0, "gid");
	DICT_PART_ADD(result, group,  1, "rating");
	DICT_PART_ADD(result, group,  2, "votes");
	DICT_PART_ADD(result, group,  3, "acount");
	DICT_PART_ADD(result, group,  4, "fcount");
	DICT_PART_ADD(result, group,  5, "name");
	DICT_PART_ADD(result, group,  6, "short");
	DICT_PART_ADD(result, group,  7, "irc_channel");
	DICT_PART_ADD(result, group,  8, "irc_server");
	DICT_PART_ADD(result, group,  9, "url");
}

anidb_result_handler_t anidb_handlers[] = {
	/* AUTH */
	{ ANIDB_LOGIN_ACCEPTED,    handler_auth_accepted },
	{ ANIDB_LOGGED_OUT,        handler_auth_logout },

	/* ANIME */
	{ ANIDB_ANIME,             handler_anime },

	/* ANIMEDESC */
	{ ANIDB_ANIME_DESCRIPTION, handler_animedesc },

	/* EPISODE */
	{ ANIDB_EPISODE,           handler_episode },

	/* FILE */
	{ ANIDB_FILE,              handler_file },

	/* GROUP */
	{ ANIDB_GROUP,             handler_group },
	/* GROUPSTATUS */
	/* MYLIST */
	/* MYLISTADD */
	/* MYLISTDEL */
	/* MYLISTSTATS */
	/* VOTE */
	/* RANDOM */
	/* PING */
	/* UPTIME */
	/* ENCODING */
	/* SENDMSG */
	/* USER */
};
