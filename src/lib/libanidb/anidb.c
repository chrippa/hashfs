#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <netdb.h>
#include <malloc.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>

#include "anidb.h"
#include "util.h"

extern anidb_result_handler_t anidb_handlers[1024];
static anidb_result_t * anidb_session_cmd (anidb_session_t *session, char *cmd, ...);
static void gen_query_va (char *buf, va_list ap);
static void gen_query (char *buf, ...);

static void
gen_query_va (char *buf, va_list ap)
{
	int i = 0;

	for (char *str = va_arg(ap, char *); str; str = va_arg(ap, char *), i++) {
		strcat(buf, str);

		if ((i % 2) == 0)
			strcat(buf, "=");
		else
			strcat(buf, "&");
	}

	buf[strlen(buf) - 1] = '\0';
}

static void
gen_query (char *buf, ...)
{
	va_list ap, aq;

	va_start(ap, buf);
	va_copy(aq, ap);
	va_end(ap);

	gen_query_va(buf, aq);
}


static int
get_ms (void)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);

	return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static void
throttle_session (anidb_session_t *session)
{
	int elapsed, needtosleep;

	elapsed = get_ms() - session->time;
	needtosleep = ANIDB_THROTTLE_MS - elapsed;

	if (needtosleep > 0) {
		usleep(needtosleep * 1000);
	}

	session->time = get_ms();
}

static void
sock_send (anidb_session_t *session, char *msg, char *out)
{
	int n;

//	printf("send: '%s'\n", msg);

	send(session->socket, msg, strlen(msg) + 1, 0);
	n = recv(session->socket, out, 1000, 0);

	out[n-1] = '\0';

//	printf("recv: '%s'\n", out);
}

anidb_session_t *
anidb_session_new (char *name, char *version, int localport)
{
	anidb_session_t *session;
	struct protoent *protocol;
	struct hostent *host;
	struct sockaddr_in addr, local;
	int sock, rval;

	session = calloc(1, sizeof(anidb_session_t));

	protocol = getprotobyname("udp");

	if (protocol == 0) {
		ANIDB_ERROR("UDP sockets not available");
	}

	sock = socket(PF_INET, SOCK_DGRAM, protocol->p_proto);

	if (sock < 0) {
		ANIDB_ERROR("Unable to open socket");
	}

	host = gethostbyname(ANIDB_SERVER_HOST);
	if (host == NULL) {
	   ANIDB_ERROR("Could lookup hostname: %s", ANIDB_SERVER_HOST);
	}

	if (localport > 0) {
		local.sin_addr.s_addr = INADDR_ANY;
		local.sin_family = AF_INET;
		local.sin_port = htons(localport);

		rval = bind(sock, (struct sockaddr *) &local,
		            sizeof(struct sockaddr_in));

		if (rval < 0) {
			ANIDB_ERROR("Unable to bind local port 0.0.0.0:%d", localport);
		}
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(ANIDB_SERVER_PORT);
	memcpy((char *) &addr.sin_addr, (char *) host->h_addr_list[0],
	       host->h_length);


	rval = connect(sock, (struct sockaddr *) &addr,
	               sizeof(struct sockaddr_in));

	if (rval < 0) {
		ANIDB_ERROR("Could not connect to server");
	}

	session->socket = sock;
	strncpy(session->clientname, name, sizeof(session->clientname));
	strncpy(session->clientversion, version, sizeof(session->clientversion));

	anidb_session_ref(session);

	return session;
}

void
anidb_session_ref (anidb_session_t *session)
{
	session->refcount++;
}

void
anidb_session_unref (anidb_session_t *session)
{
	session->refcount--;

	if (session->refcount == 0) {
		close(session->socket);

		free(session);
	}
}

void
anidb_session_set_key (anidb_session_t *session, char *key)
{
	strncpy(session->key, key, sizeof(session->key));
}

int
anidb_session_is_logged_in (anidb_session_t *session)
{
	if (session->key[0])
		return 1;

	return 0;
}

static anidb_result_t *
anidb_session_cmd (anidb_session_t *session, char *cmd, ...)
{
	char out[1024];
	char in[1024];
	int code;
	anidb_result_t *res;
	va_list aq, ap;

	sprintf(out, "%s ", cmd);

	va_start(ap, cmd);
	va_copy(aq, ap);
	gen_query_va(out, aq);
	va_end(ap);

	throttle_session(session);

	sock_send(session, out, in);
	code = atoi(in);
	res = anidb_result_new(code);

	for (int i = 0; i < LENGTH(anidb_handlers); i++) {
		if (anidb_handlers[i].code == code) {
			anidb_handlers[i].func(res, in);
			break;
		}
	}

	return res;
}


/* AUTH */

anidb_result_t *
anidb_session_authenticate (anidb_session_t *session, char *username,
                            char *password)
{
	anidb_result_t *res;

	res = anidb_session_cmd(session, "AUTH",
	                        "user", username,
	                        "pass", password,
	                        "protover", ANIDB_PROTO_VERSION,
	                        "client", session->clientname,
	                        "clientver", session->clientversion,
	                        "nat", "1",
	                        "enc", "UTF8",
	                        NULL);

	return res;
}


/* ANIME */

anidb_result_t *
anidb_session_anime_name (anidb_session_t *session, char *name)
{
	anidb_result_t *res;

	res = anidb_session_cmd(session, "ANIME",
	                        "aname", name,
	                        "s", session->key,
	                        NULL);

	return res;
}

anidb_result_t *
anidb_session_anime_id (anidb_session_t *session, int id)
{
	anidb_result_t *res;
	char aid[10];

	sprintf(aid, "%d", id);

	res = anidb_session_cmd(session, "ANIME",
	                        "aid", aid,
	                        "s", session->key,
	                        NULL);

	return res;
}


/* ANIMEDESC */

anidb_result_t *
anidb_session_animedesc (anidb_session_t *session, int id, int n)
{
	anidb_result_t *res;
	char aid[10];
	char part[10];

	sprintf(aid, "%d", id);
	sprintf(part, "%d", n);

	res = anidb_session_cmd(session, "ANIMEDESC",
	                        "aid", aid,
	                        "part", part,
	                        "s", session->key,
	                        NULL);

	return res;
}


/* EPISODE */

anidb_result_t *
anidb_session_episode_id (anidb_session_t *session, int id)
{
	anidb_result_t *res;
	char eid[10];

	sprintf(eid, "%d", id);

	res = anidb_session_cmd(session, "EPISODE",
	                        "eid", eid,
	                        "s", session->key,
	                        NULL);

	return res;
}

anidb_result_t *
anidb_session_episode_name (anidb_session_t *session, char *name, int ep)
{
	anidb_result_t *res;
	char epno[10];

	sprintf(epno, "%d", ep);

	res = anidb_session_cmd(session, "EPISODE",
	                        "aname", name,
	                        "epno", epno,
	                        "s", session->key,
	                        NULL);

	return res;

}

anidb_result_t *
anidb_session_episode_aid (anidb_session_t *session, int id, int ep)
{
	anidb_result_t *res;
	char aid[10];
	char epno[10];

	sprintf(aid, "%d", id);
	sprintf(epno, "%d", ep);

	res = anidb_session_cmd(session, "EPISODE",
	                        "aid", aid,
	                        "epno", epno,
	                        "s", session->key,
	                        NULL);

	return res;

}


/* FILE */

anidb_result_t *
anidb_session_file_id (anidb_session_t *session, int id)
{
	anidb_result_t *res;
	char fid[10];

	sprintf(fid, "%d", id);

	res = anidb_session_cmd(session, "FILE",
	                        "fid", fid,
	                        "fcode", "123682590",
	                        "acode", "75435779",
	                        "s", session->key,
	                        NULL);

	return res;
}

anidb_result_t *
anidb_session_file_ed2k (anidb_session_t *session, int64_t size, char *ed2k)
{
	anidb_result_t *res;
	char siz[50];

	sprintf(siz, "%d", size);

	res = anidb_session_cmd(session, "FILE",
	                        "size", siz,
	                        "ed2k", ed2k,
	                        "fcode", "123682590",
	                        "acode", "75435779",
	                        "s", session->key,
	                        NULL);

	return res;
}


/* GROUP */

anidb_result_t *
anidb_session_group_id (anidb_session_t *session, int id)
{
	anidb_result_t *res;
	char gid[10];

	sprintf(gid, "%d", id);

	res = anidb_session_cmd(session, "GROUP",
	                        "gid", gid,
	                        "s", session->key,
	                        NULL);

	return res;
}

anidb_result_t *
anidb_session_group_name (anidb_session_t *session, char *name)
{
	anidb_result_t *res;

	res = anidb_session_cmd(session, "GROUP",
	                        "gname", name,
	                        "s", session->key,
	                        NULL);

	return res;
}


/* GROUPSTATUS */
anidb_result_t *
anidb_session_groupstatus (anidb_session_t *session, int id)
{

}

/* MYLIST */


/* MYLISTADD */

anidb_result_t *
anidb_session_mylist_add_fid (anidb_session_t *session, int id)
{
	anidb_result_t *res;
	char fid[10];

	sprintf(fid, "%d", id);

	res = anidb_session_cmd(session, "MYLISTADD",
	                        "fid", fid,
	                        "s", session->key,
	                        NULL);

	return res;
}

anidb_result_t *
anidb_session_mylist_add_ed2k (anidb_session_t *session, int size, char *ed2k)
{
	anidb_result_t *res;
	char siz[20];

	sprintf(siz, "%d", size);

	res = anidb_session_cmd(session, "MYLISTADD",
	                        "size", siz,
	                        "ed2k", ed2k,
	                        "s", session->key,
	                        NULL);

	return res;
}


/* MYLISTDEL */

anidb_result_t *
anidb_session_mylist_del_id (anidb_session_t *session, int id)
{
	anidb_result_t *res;
	char lid[10];

	sprintf(lid, "%d", id);

	res = anidb_session_cmd(session, "MYLISTDEL",
	                        "lid", lid,
	                        "s", session->key,
	                        NULL);

	return res;

}

anidb_result_t *
anidb_session_mylist_del_fid (anidb_session_t *session, int id)
{
	anidb_result_t *res;
	char fid[10];

	sprintf(fid, "%d", id);

	res = anidb_session_cmd(session, "MYLISTDEL",
	                        "fid", fid,
	                        "s", session->key,
	                        NULL);

	return res;

}

anidb_result_t *
anidb_session_mylist_del_ed2k (anidb_session_t *session, int size, char *ed2k)
{
	anidb_result_t *res;
	char siz[20];

	sprintf(siz, "%d", size);

	res = anidb_session_cmd(session, "MYLISTDEL",
	                        "size", siz,
	                        "ed2k", ed2k,
	                        "s", session->key,
	                        NULL);

	return res;
}


/* MYLISTSTATS */

anidb_result_t *
anidb_session_mylist_stats (anidb_session_t *session)
{

}


/* LOGOUT */

anidb_result_t *
anidb_session_logout (anidb_session_t *session)
{
	anidb_result_t *res;

	res = anidb_session_cmd(session, "LOGOUT",
	                        "s", session->key,
	                        NULL);

	return res;
}

anidb_dict_t *
anidb_dict_new (void)
{
	anidb_dict_t *dict;

	dict = calloc(1, sizeof(anidb_dict_t));
	dict->next = NULL;

	anidb_dict_ref(dict);

	return dict;
}


void
anidb_dict_ref (anidb_dict_t *dict)
{
	dict->refcount++;
}

void
anidb_dict_unref (anidb_dict_t *dict)
{
	dict->refcount--;

	if (dict->refcount == 0) {
		free(dict);
	}
}

void
anidb_dict_set (anidb_dict_t *dict, char *key, char *value)
{
	dict->key = key;
	dict->value = value;
}

int
anidb_dict_get (anidb_dict_t *dict, char **out)
{
	*out = dict->value;

	return 1;
}

anidb_dict_t *
anidb_dict_next (anidb_dict_t *dict)
{
	return dict->next;
}

anidb_result_t *
anidb_result_new (int code)
{
	anidb_result_t *res;

	res = calloc(1, sizeof(anidb_result_t));
	res->code = code;
	res->type = ANIDB_RESULT_NULL;

	anidb_result_ref(res);

	return res;
}

void
anidb_result_ref (anidb_result_t *result)
{
	result->refcount++;
}

void
anidb_result_unref (anidb_result_t *result)
{
	anidb_dict_t *dict, *next;

	result->refcount--;

	if (result->refcount == 0) {
		if (result->type == ANIDB_RESULT_DICT) {
			next = result->value.dict;

			for (dict = next; dict; dict = next) {
				next = dict->next;

				anidb_dict_unref(dict);
			}
		}

		free(result);
	}
}

void
anidb_result_set_str (anidb_result_t *result, char *string)
{
	result->type = ANIDB_RESULT_STRING;
	result->value.string = string;
}

void
anidb_result_set_int (anidb_result_t *result, int number)
{
	result->type = ANIDB_RESULT_NUMBER;
	result->value.number = number;
}

void
anidb_result_dict_set (anidb_result_t *result, char *key, char *value)
{
	anidb_dict_t *dict, *new;

	result->type = ANIDB_RESULT_DICT;

	if (!result->value.dict) {
		dict = anidb_dict_new();

		anidb_dict_set(dict, key, value);

		result->value.dict = dict;
	} else {
		for (dict = result->value.dict; dict; dict = dict->next) {
			if (!dict->next) {
				new = anidb_dict_new();

				anidb_dict_set(new, key, value);

				dict->next = new;

				break;
			}
		}
	}
}

anidb_result_type_t
anidb_result_get_type (anidb_result_t *result)
{
	return result->type;
}

int
anidb_result_get_code (anidb_result_t *result)
{
	return result->code;
}

int
anidb_result_get_str (anidb_result_t *result, char **out)
{
	if (result->type != ANIDB_RESULT_STRING)
		return 0;

	*out = result->value.string;

	return 1;
}

int
anidb_result_get_int (anidb_result_t *result, int *out)
{
	if (result->type != ANIDB_RESULT_NUMBER)
		return 0;

	*out = result->value.number;

	return 1;
}

int
anidb_result_dict_get (anidb_result_t *result, char *key, char **out)
{
	anidb_dict_t *dict;

	if (result->type != ANIDB_RESULT_DICT)
		return 0;

	for (dict = result->value.dict; dict; dict = dict->next) {
		if (!strcmp(dict->key, key)) {
			return anidb_dict_get(dict, out);
		}
	}

	return 0;
}

anidb_dict_t *
anidb_result_get_dict (anidb_result_t *result)
{
	if (result->type != ANIDB_RESULT_DICT)
		return NULL;

	return result->value.dict;
}
