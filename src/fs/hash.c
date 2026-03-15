#define _POSIX_C_SOURCE 200809L

#include <sys/stat.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "fs.h"

#include "hash.h"

/* Will create a kfile struct for use with the `files' hash table.
 *
 * Returns NULL on allocation failure, malloc will set errno.
 */
struct kfile* hcrekey(struct stat* sb)
{
	struct kfile* key = malloc(sizeof(struct kfile));

	if (!key) return NULL;

	key->st_ino = (uintmax_t)sb->st_ino;
	key->st_dev = (uintmax_t)sb->st_dev;

	return key;
}

/* Given the relative path of the file, relative to the pfd, this function
 * will create a file struct for use with the hash table.
 * This function internally copies path over.
 *
 * Returns NULL on allocation failure, malloc will set errno to the corresponding value.
 */
void* hcreval(const char* path, mode_t st_mode, off_t st_size)
{
	if (!path) return NULL;

	struct file* val = malloc(sizeof(struct file));

	if (!val) return NULL;

	char* path_copy = strdup(path);
	if (!path_copy) {
		free(val);
		return NULL;
	}
	val->path = path_copy;
	val->mode = st_mode;
	val->size = st_size;
	
	return val;
}

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

/* The hash table never owns the key and value. */
/* Free them seperately. */
int free_hent(const void* key, void* value, void* user_data)
{
	free((void*)key);
	free(((struct file*)value)->path);
	free(value);
	return 0;
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

/* Uses file inode and device number to create the hash.
 * Uses the Murmur finalizer.
 */
uintmax_t hash(const void* key)
{
	const struct kfile* f = key;
	uintmax_t k = f->st_ino ^ (f->st_dev << 7);
	k ^= k >> 33;
	k *= 0xff51afd7ed558ccdULL;
	k ^= k >> 33;
	k *= 0xc4ceb9fe1a85ec53ULL;
	k ^= k >> 33;
	return k;
}

int hcmpent(const void* a, const void* b)
{
	const struct stat* st1 = a;
	const struct stat* st2 = b;
	if (st1->st_ino == st2->st_ino && st1->st_dev == st2->st_dev)
		return 1;
	return 0;
}
