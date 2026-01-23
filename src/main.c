/* A cp clone: copies files from one place to another */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>

#include "fs.h"
#include "hash.h"

status_t listing(int follow, const char* src);
int list(const void* key, void* val, void* user_data);

int main(int argc, char* argv[])
{
	if (argc < 3) {
		fprintf(stderr, "Usage: backup SOURCE DESTINATION\n");
		return 1;
	}

	const char* src = argv[1];

	listing(0, src);
	return 0;
}

status_t listing(int follow, const char* src)
{
	struct hash_table* files = hash_create(4099, hash, hcmpent);
	if (!files)
		return STATUS_E(ST_ERR_HASH_CRE, "Creating hash table", NULL);

	traverse(src, files, follow);

	int sa = 0;
	hash_foreach(files, list, &sa);
	hash_foreach(files, free_hent, NULL);
	hash_destroy(files);

	return STATUS(ST_OK, 0, "Listing of a directory", NULL);
}

int list(const void* key, void* val, void* user_data)
{
	const struct kfile* k = key;
	struct file* f = val;
	int* nfiles = user_data;
	(*nfiles)++;
	printf("File name: %s, parent fd: %i, st_dev: %lu, st_ino: %lu\n",
	       f->path, f->pfd, k->st_dev, k->st_ino);

	return 0;
}
