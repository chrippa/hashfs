#define _FILE_OFFSET_BITS 64
#define _LARGEFILE_SOURCE 1
#define _LARGEFILE64_SOURCE 1
#define _LARGE_FILES 1

#define BLOCKSIZE (9500*1024)

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>

#include <openssl/md4.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gprintf.h>

#include "hashfs.h"


static const gchar hexdigits[16] = "0123456789abcdef";

gint
hashfs_file_hash_ed2k (hashfs_file_t *file, gchar **out)
{
	gint blocks, fd, len;
	gint64 size, offset;
	guchar *hash_blocks, *hash_final;
	gchar *hash_str;
	MD4_CTX ctx;
	gpointer data;

	if ((fd = g_open(file->filename, O_RDONLY, "rb")) < 0) {
		HASHFS_DEBUG("Failed to open file (%s)", file->filename);

		return 0;
	}

	blocks = file->size / BLOCKSIZE;

	if ((file->size % BLOCKSIZE) > 0)
		blocks++;


	if (blocks < 1)
		return 0;


	hash_blocks = g_malloc(blocks * 16);
	hash_final = g_malloc(16);
	hash_str = g_strnfill(33, 0);
	offset = 0;

	g_return_val_if_fail(hash_blocks != NULL, 0);
	g_return_val_if_fail(hash_final != NULL, 0);
	g_return_val_if_fail(hash_str != NULL, 0);

	for (gint b = 0; b < blocks; b++) {
		len = (blocks - b) == 1 ? file->size % BLOCKSIZE : BLOCKSIZE;
		data = mmap(NULL, len, PROT_READ, MAP_SHARED, fd, offset);

		g_return_val_if_fail(data != NULL, 0);

		MD4_Init(&ctx);
		MD4_Update(&ctx, data, len);
		MD4_Final(hash_blocks + (b * 16), &ctx);

		munmap(data, len);

		offset += len;
	}

	close(fd);

	/* If we have hashed more than one block,
	   run MD4 on all the previous hashes */
	if (blocks > 1) {
		MD4_Init(&ctx);
		MD4_Update(&ctx, hash_blocks, 16 * blocks);
		MD4_Final(hash_final, &ctx);
	} else {
		hash_final = g_memdup(hash_blocks, 16);
	}

	/* Convert hash to hex string format */
	for (gint i = 0; i < 16; i++) {
		hash_str[(i<<1)] = hexdigits[(((hash_final[i]) & 0xf0) >> 4)];
		hash_str[(i<<1)+1] = hexdigits[(((hash_final[i]) & 0x0f))];
	}

	g_free(hash_blocks);
	g_free(hash_final);

	*out = hash_str;

	return 1;
}
