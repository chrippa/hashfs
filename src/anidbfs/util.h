#ifndef _ANIDBFS_UTIL_H
#define _ANIDBFS_UTIL_H

char * anidbfs_current_time(void);
char * anidbfs_basename (char *name);


#define ANIDBFS_LOG(fmt, ...) { \
	printf("%s  LOG ", anidbfs_current_time()); \
	printf(fmt"\n", ## __VA_ARGS__); \
}

#ifdef DEBUG
	#define ANIDBFS_DEBUG(fmt, ...) { \
		printf("%s  DEBUG %s:%d: ", anidbfs_current_time(), __FILE__, __LINE__); \
		printf(fmt"\n", ## __VA_ARGS__); \
	}
#else
	#define ANIDBFS_DEBUG(...)
#endif

#endif
