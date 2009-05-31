#ifndef _LIBANIDB_UTIL_H
#define _LIBANIDB_UTIL_H

#define ANIDB_ERROR(str, ...) \
	fprintf(stderr, "LIBANIDB ERROR: "str"\n", ##__VA_ARGS__); \
	exit(1);

#define LENGTH(x) (sizeof x / sizeof x[0])

#endif
