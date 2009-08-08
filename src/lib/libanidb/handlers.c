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


static void get_part (char *buf, char *src, char *delim, int idx);

#define PARSE_KEYS(result, src, keys) { \
	char buf[1024]; \
	for (int i = 0; i < LENGTH((keys)); i++) { \
		get_part((buf), (src), "|", i); \
		anidb_result_dict_set((result), (keys[i]), (buf)); \
	} \
}

static void
get_part (char *buf, char *src, char *delim, int idx)
{
	char *ptr;
	int len;

	ptr = src;

	for (int n = 0; n <= idx; n++) {
		len = strstr(ptr, delim) - ptr;

		if (len < 0 || len > strlen(ptr))
			len = strlen(ptr);

		strncpy(buf, ptr, len);
		buf[len] = '\0';

		ptr += len + strlen(delim);
	}
}

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
	char *keys[] = { "aid", "eps", "ep_count", "special_cnt",
	                 "rating", "votes", "tmprating", "tmpvotes",
	                 "rating_avg", "reviews", "year", "type",
	                 "romaji", "kanji", "english", "other",
	                 "short_names", "synonyms", "category_list" };

	PARSE_KEYS(result, data + 10, keys);
}

static void
handler_animedesc (anidb_result_t *result, char *data)
{
	char *keys[] = { "part", "max_parts", "description" };

	PARSE_KEYS(result, data + 14, keys);
}

static void
handler_episode (anidb_result_t *result, char *data)
{
	char *keys[] = { "eid", "aid", "length", "rating",
	                 "votes", "epno", "eng", "romaji",
	                 "kanji", "aired" };

	PARSE_KEYS(result, data + 12, keys);
}

static void
handler_file (anidb_result_t *result, char *data)
{
	char *keys[] = { "fid", "aid", "eid", "gid",
	                 "lid", "state", "size", "ed2k",
	                 "md5", "sha1", "crc32", "dub",
	                 "sub", "quality", "source",
	                 "audio", "video", "resolution",
	                 "ext", "duration",

	                 "group_name", "group_short",

	                 "ep_number", "ep_eng", "ep_romaji",
	                 "ep_kanji",

	                 "anime_totalep", "anime_lastep",
	                 "anime_year", "anime_type",
	                 "anime_romaji", "anime_kanji",
	                 "anime_eng", "anime_categories" };

	PARSE_KEYS(result, data + 9, keys);
}

static void
handler_group (anidb_result_t *result, char *data)
{
	char *keys[] = { "gid", "rating", "votes", "acount",
	                 "fcount", "name", "short",
	                 "irc_channel", "irc_server", "url" };

	PARSE_KEYS(result, data + 10, keys);
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
