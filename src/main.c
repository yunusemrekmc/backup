/* A cp clone: copies files from one place to another */
#define _POSIX_C_SOURCE 200809L

#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>

#include "fs.h"
#include "hash.h"
#include "status.h"

status_t listing(int follow, const char* src);
int list(const void* key, void* val, void* user_data);

int main(int argc, char* argv[])
{
	if (argc < 3) {
		fprintf(stderr, "Usage: backup SOURCE DESTINATION\n");
		return 1;
	}

	const char* src = argv[1];

	status_t ret = listing(0, src);
	if (ret.c != ST_OK) {
		sterr(ret);
		status_free(ret);
		return -1;
	}
	return 0;
}

status_t listing(int follow, const char* src)
{
	struct hash_table* files = hash_create(4099, hash, hcmpent);
	if (!files)
		return STATUS_E(ST_ERR_HASH_CRE, "Creating hash table", NULL);

	int oflags = O_DIRECTORY | O_RDONLY; 	/* flags given to open */

	/* Don't follow symlinks */
	if (!follow) oflags |= O_NOFOLLOW;

	status_t ret = traverse(src, &files, oflags);
	if (ret.c != ST_OK) {
		hash_foreach(files, free_hent, NULL);
		hash_destroy(files);
		return ret;
	}

	size_t n = 0;
	hash_foreach(files, list, &n);
	printf("Listed this many files: %zu\n", n);
	hash_foreach(files, free_hent, NULL);
	hash_destroy(files);

	return STATUS(ST_OK, 0, "Listing of a directory", NULL);
}

int list(const void* key, void* val, void* user_data)
{
	const struct kfile* k = key;
	struct file* f = val;
	size_t* nfiles = user_data;
	(*nfiles)++;
	printf("File name: %s, mode: %o, size: %zu, st_dev: %" PRIuMAX ", st_ino: %" PRIuMAX "\n",
	       f->path, (f->mode & 0777), f->size, k->st_dev, k->st_ino);

	return 0;
}
