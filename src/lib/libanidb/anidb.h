#ifndef _LIBANIDB_H
#define _LIBANIDB_H

#include <sys/time.h>


enum anidb_result_type
{
	ANIDB_RESULT_NULL,
	ANIDB_RESULT_NUMBER,
	ANIDB_RESULT_STRING,
	ANIDB_RESULT_DICT
};

struct anidb_session_St
{
	int socket;
	char *clientname;
	char *clientversion;
	char *key;

	int refcount;
};

struct anidb_dict_St
{
	char *key;
	char *value;

	struct anidb_dict_St *next;
	int refcount;
};

struct anidb_result_St
{
	struct anidb_error_St *error;
	int code;
	enum anidb_result_type type;

	union {
		int number;
		char *string;
		struct anidb_dict_St *dict;
	} value;

	int refcount;
};

struct anidb_result_handler_St
{
	int code;
	void (*func)(struct anidb_result_St *, char *);
};

/* Types */
typedef enum anidb_result_type anidb_result_type_t;
typedef struct anidb_session_St anidb_session_t;
typedef struct anidb_dict_St anidb_dict_t;
typedef struct anidb_result_St anidb_result_t;
typedef struct anidb_result_handler_St anidb_result_handler_t;
typedef struct anidb_error_St anidb_error_t;


/* Session */
anidb_session_t * anidb_session_new (char *name, char *version, int localport);

/* AUTH */
anidb_result_t * anidb_session_authenticate (anidb_session_t *session,
                                             char *username,
                                             char *password);

/* LOGOUT */
anidb_result_t * anidb_session_logout (anidb_session_t *session);

/* ANIME */
anidb_result_t * anidb_session_anime_name (anidb_session_t *session, char *name);
anidb_result_t * anidb_session_anime_id (anidb_session_t *session, int id);

/* ANIMEDESC */
anidb_result_t * anidb_session_animedesc (anidb_session_t *session, int id, int ep);

/* EPISODE */
anidb_result_t * anidb_session_episode_id (anidb_session_t *session, int id);
anidb_result_t * anidb_session_episode_name (anidb_session_t *session, char *name, int ep);
anidb_result_t * anidb_session_episode_aid (anidb_session_t *session, int id, int ep);

/* FILE */
anidb_result_t * anidb_session_file_id (anidb_session_t *session, int id);
anidb_result_t * anidb_session_file_ed2k (anidb_session_t *session, int64_t size, char *ed2k);

/* GROUP */
anidb_result_t * anidb_session_group_id (anidb_session_t *session, int id);
anidb_result_t * anidb_session_group_name (anidb_session_t *session, char *name);

/* GROUPSTATUS */
anidb_result_t * anidb_session_groupstatus (anidb_session_t *session, int id);

/* MYLIST */

/* MYLISTADD */
anidb_result_t * anidb_session_mylist_add_fid (anidb_session_t *session, int id);
anidb_result_t * anidb_session_mylist_add_ed2k (anidb_session_t *session, int size, char *ed2k);

/* MYLISTDEL */
anidb_result_t * anidb_session_mylist_del_id (anidb_session_t *session, int id);
anidb_result_t * anidb_session_mylist_del_fid (anidb_session_t *session, int id);
anidb_result_t * anidb_session_mylist_del_ed2k (anidb_session_t *session, int size, char *ed2k);

/* MYLISTSTATS */
anidb_result_t * anidb_session_mylist_stats (anidb_session_t *session);

/* VOTE */

/* RANDOM */

/* MYLISTEXPORT */

/* PING */
anidb_result_t * anidb_session_ping (anidb_session_t *session);

/* VERSION */
anidb_result_t * anidb_session_uptime (anidb_session_t *session);

/* UPTIME */
anidb_result_t * anidb_session_version (anidb_session_t *session);

/* ENCODING */

/* SENDMSG */

/* USER */


void anidb_session_set_key (anidb_session_t *session, char *key);
int anidb_session_is_logged_in (anidb_session_t *session);
void anidb_session_ref (anidb_session_t *session);
void anidb_session_unref (anidb_session_t *session);


/* Dict */
anidb_dict_t * anidb_dict_new (void);
anidb_dict_t * anidb_dict_next (anidb_dict_t *dict);
void anidb_dict_ref (anidb_dict_t *dict);
void anidb_dict_unref (anidb_dict_t *dict);

void anidb_dict_set (anidb_dict_t *dict, char *key, char *value);
int anidb_dict_get (anidb_dict_t *dict, char **out);


/* Result */
anidb_result_t * anidb_result_new (int code);
void anidb_result_ref (anidb_result_t *result);
void anidb_result_unref (anidb_result_t *result);

void anidb_result_set_str (anidb_result_t *result, char *string);
void anidb_result_set_int (anidb_result_t *result, int number);
void anidb_result_dict_set (anidb_result_t *result, char *key, char *value);

anidb_result_type_t anidb_result_get_type (anidb_result_t *result);
int anidb_result_get_code (anidb_result_t *result);
int anidb_result_get_str (anidb_result_t *result, char **out);
int anidb_result_get_int (anidb_result_t *result, int *out);
int anidb_result_dict_get (anidb_result_t *result, char *key, char **out);
anidb_dict_t * anidb_result_get_dict (anidb_result_t *result);


#define ANIDB_SERVER_HOST "api.anidb.net"
#define ANIDB_SERVER_PORT 9000
#define ANIDB_PROTO_VERSION "3"

/* Positive responses 2XX */

#define ANIDB_LOGIN_ACCEPTED 200
#define ANIDB_LOGIN_ACCEPTED_NEW_VER 201
#define ANIDB_LOGGED_OUT 203
#define ANIDB_RESOURCE 205
#define ANIDB_STATS	206
#define ANIDB_TOP 207
#define ANIDB_UPTIME 208
#define ANIDB_ENCRYPTION_ENABLED 209

#define ANIDB_MYLIST_ENTRY_ADDED 210
#define ANIDB_MYLIST_ENTRY_DELETED 211

#define ANIDB_ADDED_FILE 214
#define ANIDB_ADDED_STREAM 215

#define ANIDB_ENCODING_CHANGED 219

#define ANIDB_FILE 220
#define ANIDB_MYLIST 221
#define ANIDB_MYLIST_STATS 222

#define ANIDB_ANIME 230
#define ANIDB_ANIME_BEST_MATCH 231
#define ANIDB_RANDOMANIME 232
#define ANIDB_ANIME_DESCRIPTION 233

#define ANIDB_EPISODE 240
#define ANIDB_PRODUCER 245
#define ANIDB_GROUP 250

#define ANIDB_BUDDY_LIST 253
#define ANIDB_BUDDY_STATE 254
#define ANIDB_BUDDY_ADDED 255
#define ANIDB_BUDDY_DELETED 256
#define ANIDB_BUDDY_ACCEPTED 257
#define ANIDB_BUDDY_DENIED 258

#define ANIDB_VOTED 260
#define ANIDB_VOTE_FOUND 261
#define ANIDB_VOTE_UPDATED 262
#define ANIDB_VOTE_REVOKED 263

#define ANIDB_NOTIFICATION_ENABLED 270
#define ANIDB_NOTIFICATION_NOTIFY 271
#define ANIDB_NOTIFICATION_MESSAGE 272
#define ANIDB_NOTIFICATION_BUDDY 273
#define ANIDB_NOTIFICATION_SHUTDOWN 274
#define ANIDB_PUSHACK_CONFIRMED 280
#define ANIDB_NOTIFYACK_SUCCESSFUL_M 281
#define ANIDB_NOTIFYACK_SUCCESSFUL_N 282
#define ANIDB_NOTIFICATION 290
#define ANIDB_NOTIFYLIST 291
#define ANIDB_NOTIFYGET_MESSAGE 292
#define ANIDB_NOTIFYGET_NOTIFY 293

#define ANIDB_SENDMSG_SUCCESSFUL 294
#define ANIDB_USER 295


/* Affirmative/Negative responses 3XX */

#define ANIDB_PONG 300
#define ANIDB_AUTHPONG 301
#define ANIDB_NO_SUCH_RESOURCE 305
#define ANIDB_API_PASSWORD_NOT_DEFINED 309

#define ANIDB_FILE_ALREADY_IN_MYLIST 310
#define ANIDB_MYLIST_ENTRY_EDITED 311
#define ANIDB_MULTIPLE_MYLIST_ENTRIES 312

#define ANIDB_SIZE_HASH_EXISTS 314
#define ANIDB_INVALID_DATA 315
#define ANIDB_STREAMNOID_USED 316

#define ANIDB_NO_SUCH_FILE 320
#define ANIDB_NO_SUCH_ENTRY 321
#define ANIDB_MULTIPLE_FILES_FOUND 322

#define ANIDB_NO_SUCH_ANIME 330
#define ANIDB_NO_SUCH_ANIME_DESCRIPTION 333
#define ANIDB_NO_SUCH_EPISODE 340
#define ANIDB_NO_SUCH_PRODUCER 345
#define ANIDB_NO_SUCH_GROUP 350

#define ANIDB_BUDDY_ALREADY_ADDED 355
#define ANIDB_NO_SUCH_BUDDY 356
#define ANIDB_BUDDY_ALREADY_ACCEPTED 357
#define ANIDB_BUDDY_ALREADY_DENIED 358

#define ANIDB_NO_SUCH_VOTE 360
#define ANIDB_INVALID_VOTE_TYPE 361
#define ANIDB_INVALID_VOTE_VALUE 362
#define ANIDB_PERMVOTE_NOT_ALLOWED 363
#define ANIDB_ALREADY_PERMVOTED 364

#define ANIDB_NOTIFICATION_DISABLED 370
#define ANIDB_NO_SUCH_PACKET_PENDING 380
#define ANIDB_NO_SUCH_ENTRY_M 381
#define ANIDB_NO_SUCH_ENTRY_N 382

#define ANIDB_NO_SUCH_MESSAGE 392
#define ANIDB_NO_SUCH_NOTIFY 393
#define ANIDB_NO_SUCH_USER 394


/* Negative responses 4XX */

#define ANIDB_NOT_LOGGED_IN 403

#define ANIDB_NO_SUCH_MYLIST_FILE 410
#define ANIDB_NO_SUCH_MYLIST_ENTRY 411


/* Client side failure 5XX */

#define ANIDB_LOGIN_FAILED 500
#define ANIDB_LOGIN_FIRST 501
#define ANIDB_ACCESS_DENIED 502
#define ANIDB_CLIENT_VERSION_OUTDATED 503
#define ANIDB_CLIENT_BANNED 504
#define ANIDB_ILLEGAL_INPUT_OR_ACCESS_DENIED 505
#define ANIDB_INVALID_SESSION 506
#define ANIDB_NO_SUCH_ENCRYPTION_TYPE 509
#define ANIDB_ENCODING_NOT_SUPPORTED 519

#define ANIDB_BANNED 555
#define ANIDB_UNKNOWN_COMMAND 598


/* Server side failure 6XX */

#define ANIDB_INTERNAL_SERVER_ERROR 600
#define ANIDB_ANIDB_OUT_OF_SERVICE 601
#define ANIDB_SERVER_BUSY 602
#define ANIDB_API_VIOLATION 666


#endif
