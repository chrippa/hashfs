
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>

#include "hashfs.h"

void test_query (gchar *querystr)
{
	hashfs_db_query_t *query;
	hashfs_db_result_t *result;


	query = hashfs_db_query_new(querystr);
	result = hashfs_db_query_result(query);

	printf("num results: %d\n", hashfs_db_result_num(result));

	for (gint i = 0; i < hashfs_db_result_num(result); i++) {
		hashfs_db_entry_t *entry;
		const gchar *tmp;

		entry = hashfs_db_result_get_entry(result, i);

		if (hashfs_db_entry_lookup(entry, "path", &tmp))
			printf("path = %s\n", tmp);

		hashfs_db_entry_destroy(entry);
	}


	hashfs_db_query_destroy(query);
	hashfs_db_result_destroy(result);
}

gint
main (gint argc, gchar **argv)
{

	hashfs_config_load();
	hashfs_db_open();

	test_query("pkey.BeginsWith(file:) path.BeginsWith(/tmp)");
	test_query("pkey.BeginsWith(file:) anidb:resolved.Equals(1)");

	hashfs_config_save();
	hashfs_db_close();

	return 0;
}
