#define _POSIX_C_SOURCE 200809L

#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "hash.h"
#include "fs.h"
#include "status.h"

static status_t searchdir(struct stack* dirs, struct hash_table** files, int oflags);

/* Goes through a directory recursively, and each file it founds
 * adds it to the given hash table. Will resize the hash table appropriately,
 * however, never owns it. Will never take the responsibility to free it.
 */
status_t traverse(const char* restrict path, struct hash_table** files, int oflags)
{
	struct stack dirs = { .top = NULL, .ndir = 0 };

	/* the hash table is a must for safety */
	if (!(*files))
		return STATUS(ST_INT_ISNULL, EINVAL, "No hash table", NULL);
	status_t ret;
	ret = push(&dirs, -2, path, oflags);
	if (ret.c != ST_OK) return ret;

	while(dirs.top) {
		ret = searchdir(&dirs, files, oflags);
		if (ret.c != ST_OK) {
			/* Permission error, rather skip */
			if (ret.sysc == EACCES) {
				sterr(ret);
				status_free(ret);
				continue;
			}
			goto err_free_stack;
		}
	}
	
err_free_stack:
	while(dirs.top) pop(&dirs);
	return ret;
}

static status_t searchdir(struct stack* dirs, struct hash_table** files, int oflags)
{
	/* To hold the readdir entry, and fstatat stat struct. */
	struct dirent* entry;
	struct stat sb;
	/* the directory stream at the top of the stack */
	DIR* d = dirs->top->dir;
	/* dynamically choose fstatat flags */
	int statflags = 0;
	if (oflags & O_NOFOLLOW) statflags = AT_SYMLINK_NOFOLLOW;

	while((entry = readdir(d)) != NULL) {
		/* Skip the current and previous directory */
		if ((strcmp(entry->d_name, ".")) == 0 ||
		    strcmp(entry->d_name, "..") == 0) continue;
		errno = 0;
		*files = hash_upsize(*files);
		if (errno != 0)
			return STATUS_E(ST_ERR_HASH_UPS, "Resizing table", NULL);

		if (fstatat(dirfd(d), entry->d_name, &sb, statflags) == -1) {
			if (errno == EACCES) {
				sterr(STATUS_E(ST_ERR_FILERD_MD,
					       "Reading file metadata", entry->d_name));
				continue;
			}
			return STATUS_E(ST_ERR_FILERD_MD,
					"Reading file metadata", strdup(entry->d_name));
		}

		struct kfile* key = hcrekey(&sb);
		if (!key) {
			/* couldn't allocate key */
			return STATUS_E(ST_ERR_MALLOC, "Inserting hash entries", NULL);
		}
		if (hash_lookup(*files, key)) {
			/* seen this file already */
			free(key);
			continue;
		}

		char* full = path_concat(dirs->top->dirname, entry->d_name);
		struct file* val = hcreval(full, sb.st_mode, sb.st_size);
		free(full);
		if (!val || hash_insert(*files, key, val) < 0) {
			free(val);
			free(key);
			pop(dirs);
			return STATUS_E(ST_ERR_MALLOC, "Inserting hash entries", NULL);
		}
		
		if (S_ISDIR(sb.st_mode)) {
			status_t ret = push(dirs, dirfd(d), entry->d_name, oflags);
			return ret;
		}

	}
	pop(dirs);
	return STATUS(ST_OK, 0, NULL, NULL);
}
