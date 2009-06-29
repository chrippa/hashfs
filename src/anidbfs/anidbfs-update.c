#define _FILE_OFFSET_BITS 64
#define _LARGEFILE64_SOURCE
#define BLOCKSIZE (9500*1024)

#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <math.h>

#include <openssl/md4.h>
#include <anidb.h>

#include "util.h"

static void anidbfs_hash_file (anidb_session_t *session, char *path, struct stat *info);
static void anidbfs_hash_dir (anidb_session_t *session, char *path);

static char * ed2k_hash (char *filename);
static void dump_result (anidb_result_t *result);
static const char hexdigits[16] = "0123456789abcdef";

static char *
ed2k_hash (char *filename)
{
	int fd, blocks;
	off_t size;
	struct stat stat;
	char *hash_str;
	unsigned char *hashes, *hash;

	if ((fd = open(filename, O_RDONLY)) < 0) {
		ANIDBFS_DEBUG("Failed to open file (%s)", filename);

		return NULL;
	}

	if (fstat(fd, &stat) < 0) {
		ANIDBFS_DEBUG("Failed to stat file (%s)", filename);

		return NULL;
	}

	size = stat.st_size;

	blocks = (double) size / (double) BLOCKSIZE;

	if (fmod((double) size, (double) BLOCKSIZE) > 0)
		blocks++;

	hash = (unsigned char *) malloc(16);
	hash_str = (char *) malloc(33);
	hashes = (unsigned char *) malloc(blocks * 16);

	if (!hash || !hashes || !hash_str || !filename) {
		ANIDBFS_DEBUG("Failed to allocate memory (%s)", filename);

		return NULL;
	}

	for (int b = 0; b < blocks; b++) {
		MD4_CTX ctx;
		int len;
		void *data;
		off_t offset;

		len = BLOCKSIZE;
		if (b == blocks - 1) /* Last block is smaller than BLOCKSIZE */
			len = ceil(fmod((double) size, (double) BLOCKSIZE));

		offset = (off_t) ((double) b * (double) BLOCKSIZE);

		data = mmap(NULL, len, PROT_READ, MAP_SHARED, fd, offset);
		if (!data) {
			ANIDBFS_DEBUG("mmap failed to read data (%s)", filename);
			return NULL;
		}

		MD4_Init(&ctx);
		MD4_Update(&ctx, data, len);
		MD4_Final(hashes + (b * 16), &ctx);

		munmap(data, len);
	}

	close(fd);

	/* If we have hashed more than one block,
	   run MD4 on all the previous hashes */
	if (blocks > 1) {
		MD4_CTX ctx;

		MD4_Init(&ctx);
		MD4_Update(&ctx, hashes, 16 * blocks);
		MD4_Final(hash, &ctx);
	} else {
		memcpy(hash, hashes, 16);
	}

	/* Convert hash to hex string format */
	memset(hash_str, 0x00, 33 * sizeof(char));
	for (int i = 0; i < 16; i++) {
		hash_str[(i<<1)] = hexdigits[(((hash[i]) & 0xf0) >> 4)];
		hash_str[(i<<1)+1] = hexdigits[(((hash[i]) & 0x0f))];
	}

	free(hash);
	free(hashes);

	return hash_str;
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

			ANIDBFS_DEBUG(">");

			break;
	}
}

static void
anidbfs_hash_file (anidb_session_t *session, char *path, struct stat *info)
{
	char *hash;

	ANIDBFS_LOG("Hashing file: %s", anidbfs_basename(path));

	hash = ed2k_hash(path);

	if (hash) {
		ANIDBFS_DEBUG("Hash: %s", hash)

		anidb_result_t *res;

		res = anidb_session_file_ed2k(session, (double) info->st_size, hash);

		dump_result(res);

		anidb_result_unref(res);

		free(hash);
	}
}

static void
anidbfs_hash_dir (anidb_session_t *session, char *path)
{
	DIR *dir;
	char fullpath[9999];
	struct stat info;
	struct dirent *entry;

	ANIDBFS_LOG("Hashing dir: %s", path);

	dir = opendir(path);

	if (dir) {
		while (entry = readdir(dir)) {

			if (!strcmp (entry->d_name, ".") || !strcmp (entry->d_name, ".."))
				continue;

			sprintf(fullpath, "%s%s%s", path,
			        (path[strlen(path)-1] == '/') ? "" : "/",
			        entry->d_name);

			stat(fullpath, &info);

			if (S_ISDIR(info.st_mode)) {
				anidbfs_hash_dir(session, fullpath);
			} else if (S_ISREG(info.st_mode) || S_ISLNK(info.st_mode))  {
				anidbfs_hash_file(session, fullpath, &info);
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
		fprintf(stderr, "Usage: %s <username> <password> <path>\n", argv[0]);
		exit(0);
	}

	session = anidb_session_new("anidbfuse", "1");

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


