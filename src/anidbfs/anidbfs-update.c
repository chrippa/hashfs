#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define __USE_LARGEFILE64
#define BLOCKSIZE	(9500*1024)

#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

#include <openssl/md4.h>
#include <anidb.h>

#include "util.h"

static void anidbfs_hash_file (anidb_session_t *session, char *path);
static void anidbfs_hash_dir (anidb_session_t *session, char *path);
static int anidbfs_dir_list (const char *name, const struct stat *status, int flag);

static unsigned long get_file_size (int fd);
static char * ed2k_hash (char *filename);
static void dump_result (anidb_result_t *result);
static const char hexdigits[16] = "0123456789abcdef";


static unsigned long
get_file_size (int fd)
{
	struct stat64 info;

	fstat64(fd, &info);

	return (unsigned long) info.st_size;
}

static char *
ed2k_hash (char *filename)
{
	unsigned long size;
	int fd, b, j, blocks;
	unsigned char *parthashes, *ed2k_hash;
	char *ed2k_hash_str;

	if ((fd = open(filename, O_RDONLY)) < 0) {
		ANIDBFS_DEBUG("Failed to open file (%s)", filename);
		return NULL;
	}

	size = get_file_size(fd);

	if (size <= 0) {
		ANIDBFS_DEBUG("Error getting filesize (%s)", filename);
		return NULL;
	}

	blocks = size / BLOCKSIZE;

	if (size % BLOCKSIZE > 0)
		blocks++;

	ed2k_hash = (unsigned char *) malloc(16);
	ed2k_hash_str = (char *) malloc(33);
	parthashes = (unsigned char *) malloc(blocks * 16);

	if ((!parthashes) || (!ed2k_hash) || (!ed2k_hash_str) || (!filename)) {
		ANIDBFS_DEBUG("Failed to allocate memory (%s)", filename);
		return NULL;
	}

	for (b = 0; b < blocks; b++) {
		MD4_CTX context;
		int len, start;
		void *map;

		len = BLOCKSIZE;
		if (b == blocks - 1)
			len = size % BLOCKSIZE;

		start = b * BLOCKSIZE;
		map = mmap(NULL, len, PROT_READ, MAP_SHARED, fd, b * BLOCKSIZE);

		if (map == NULL) {
			ANIDBFS_DEBUG("mmap failed (%s)", filename);
		}

		MD4_Init(&context);
		MD4_Update(&context, map, len);
		MD4_Final(parthashes + (b * 16), &context);

		munmap(map, len);

/*		int percent = (int) (((float) (b+1) / (float) blocks) * 100);
		ANIDBFS_DEBUG("Hashing %s %d%%", filename, percent);*/
	}

	close(fd);

	if (blocks > 1) {
		MD4_CTX context;

		MD4_Init(&context);
		MD4_Update(&context, parthashes, 16 * b);
		MD4_Final(ed2k_hash, &context);
	} else {
		memcpy(ed2k_hash, parthashes, 16);
	}

	memset(ed2k_hash_str, 0x00, 33 * sizeof(char));
	for (j = 0; j < 16; j++) {
		ed2k_hash_str[(j<<1)] = hexdigits[(((ed2k_hash[j]) & 0xf0) >> 4)];
		ed2k_hash_str[(j<<1)+1] = hexdigits[(((ed2k_hash[j]) & 0x0f))];
	}

	free(ed2k_hash);
	free(parthashes);

	return ed2k_hash_str;
}

static void
dump_result (anidb_result_t *result)
{
	int n;
	char *str;
	anidb_dict_t *dict;

	switch (anidb_result_get_type(result)) {
		case ANIDB_RESULT_NULL:
			ANIDBFS_DEBUG("<result %p null>", result);
			break;

		case ANIDB_RESULT_STRING:
			anidb_result_get_str(result, &str);
			ANIDBFS_DEBUG("<result %p (string) \"%s\">", result, str);

			break;

		case ANIDB_RESULT_NUMBER:
			anidb_result_get_int(result, &n);
			ANIDBFS_DEBUG("<result %p (int) \"%d\">", result, n);

			break;

		case ANIDB_RESULT_DICT:
			ANIDBFS_DEBUG("<result %p (dict)", result);

			for (dict = anidb_result_get_dict(result); dict; dict = anidb_dict_next(dict)) {
				ANIDBFS_DEBUG("  %s = \"%s\"", dict->key, dict->value);

			}

			ANIDBFS_DEBUG(">\n");

			break;
	}
}

static void
anidbfs_hash_file (anidb_session_t *session, char *path)
{
	char *hash;

	ANIDBFS_LOG("Hashing file: %s", anidbfs_basename(path));

	hash = ed2k_hash(path);

	ANIDBFS_DEBUG("Hash: %s", hash)

	free(hash);
}

static void
anidbfs_hash_dir (anidb_session_t *session, char *path)
{
	DIR *dir;
	char fullpath[9999];
	struct stat64 info;
	struct dirent *entry;

	ANIDBFS_LOG("Hashing dir: %s", path);

	dir = opendir(path);

	if (dir) {
		while (entry = readdir(dir)) {
			if (entry->d_name && entry->d_name[0] != '.') {
				sprintf(fullpath, "%s%s%s", path,
				        (path[strlen(path)-1]=='/') ? "" : "/",
				        entry->d_name);

				stat64(fullpath, &info);

				if (S_ISDIR(info.st_mode)) {
					anidbfs_hash_dir(session, fullpath);
				} else if (S_ISREG(info.st_mode) || S_ISLNK(info.st_mode))  {
					anidbfs_hash_file(session, fullpath);
				}
			}
		}

		closedir(dir);
	}

}

int
main (int argc, char *argv[])
{
	anidb_session_t *session;
	anidb_result_t *res;
	char *key;

	if (argc < 4) {
		printf("Usage: %s <username> <password> <path>\n", argv[0]);
		exit(0);
	}

	session = anidb_session_new("anidbfs", "1");

	ANIDBFS_DEBUG("Connecting to AniDB");
	res = anidb_session_authenticate(session, argv[1], argv[2]);
	dump_result(res);

	if (anidb_result_get_code(res) == ANIDB_LOGIN_ACCEPTED) {
		if (anidb_result_get_str(res, &key)) {
			anidb_session_set_key(session, key);
		}

		anidbfs_hash_dir(session, argv[3]);

		ANIDBFS_DEBUG("Logging out");
		anidb_session_logout(session);
	}

	anidb_result_unref(res);
	anidb_session_unref(session);

	return 1;
}


