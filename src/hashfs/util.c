
#include <sys/time.h>
#include <string.h>
#include <time.h>

#include "hashfs.h"

gchar *
hashfs_basename (const gchar *name)
{
	gchar *ptr, *base;

	for (ptr = (gchar *) name; *ptr; ptr++) {
		if (*ptr == '/') {
			base = ptr + 1;
		}
	}

	return base;
}

gchar *
hashfs_current_time (void)
{
	char buf[256];
	time_t tv;
	struct tm st;

	tv = time(NULL);
	localtime_r(&tv, &st);
	strftime(buf, sizeof(buf), "%H:%M:%S", &st);

	return g_strdup(buf);
}

gchar *
hashfs_md5_str (const gchar *str)
{
	return g_compute_checksum_for_string(G_CHECKSUM_MD5, str, -1);
}

