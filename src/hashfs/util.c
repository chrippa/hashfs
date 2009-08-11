
#include <sys/time.h>
#include <string.h>
#include <time.h>

#include "hashfs.h"

gchar *
hashfs_basename (char *name)
{
	const char *base;

	for (base = name; *name; name++) {
		if (*name == '/') {
			base = name + 1;
		}
	}

	return (char *) base;
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

