#define _POSIX_C_SOURCE 200809L

#include <sys/stat.h>
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

struct stackdir {
	char* name;		/* file name */
	DIR* dir;		/* this directory */
	char* dirname;		/* dirname of the current folder */
	struct stackdir* next;	/* next frame */
};

//static int freehtfiles(const void* key, void* value, void* user_data);
static status_t push(struct stack* dirs, int fd, const char* name, int oflags);
static status_t pop(struct stack* dirs);
static char* path_concat(const char* base, const char* name);
static inline struct kfile* hcrekey(struct stat* sb);
static void* hcreval(const char* path, mode_t st_mode, off_t st_size);

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
		struct stackdir* frame = dirs.top;
		DIR* d = frame->dir;
		ret = searchdir(&dirs, d, files, oflags);
		if (ret.c != ST_OK) {
			goto err_free_stack;
		}
	}
err_free_stack:
	while(dirs.top) pop(&dirs);
	return ret;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wunused-parameter"

/* The hash table never owns the key and value. */
/* Free them seperately. */
int free_hent(const void* key, void* value, void* user_data)
{
	free((void*)key);
	free(((struct file*)value)->path);
	free(value);
	return 0;
}

#pragma GCC diagnostic pop

status_t searchdir(struct stack* dirs, DIR* d, struct hash_table** files, int oflags)
{
	/* We choose to only use stack for anything new
	 * in this function for safety and simplicity.
	 * Only the created table entries are malloc'd, and that is done indirectly,
	 * via the hcrekey and hcreval functions. Will free those two on error.
	 */
	struct dirent* entry;
	struct stat sb;
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
			if (ret.sysc == EACCES) {
				sterr(ret);
				status_free(ret);
				continue;
			}
			return ret;
		}

	}
	pop(dirs);
	return STATUS(ST_OK, 0, NULL, NULL);
}

/* Stack owns stackdir, therefore each of its members too.
 * push will handle allocation.
 * pop will free everything in stackdir, including DIR* d.
 */
static status_t push(struct stack* dirs, int fd, const char* name, int oflags)
{
	if (!dirs)
		/* Got NULL for some reason */
		return STATUS(ST_INT_ISNULL, 0, "Pushing directory", NULL);

	struct stackdir* dir = malloc(sizeof(struct stackdir));
	if (!dir)
		return STATUS(ST_ERR_MALLOC, errno, "Pushing directory", NULL);
	dir->name = strdup(name); /* Store name in a seperate buffer */
	if (!dir->name) {
		free(dir);
		return STATUS(ST_ERR_MALLOC, errno, "Pushing directory", NULL);
	}
	const char* parent_path = (!dirs->top) ? "" : dirs->top->dirname;

	/* Bind d stream to fd */
	DIR* d;  /* current directory's stream */
	status_t ret = stream_subdir(fd, name, oflags, &d);
	dir->dirname = path_concat(parent_path, name);
	if (ret.c != ST_OK) {
		ret.file_target = strdup(dir->dirname);
		free(dir->name);
		free(dir);
		return ret;
	}
	dir->dir = d;
	dir->next = dirs->top;
	dirs->top = dir;
	dirs->ndir++;
	return STATUS(ST_OK, 0, "Pushing directory", NULL);
}

static status_t pop(struct stack* dirs)
{
	if (!dirs)
		/* Got NULL */
		return STATUS(ST_INT_ISNULL, 0, "No stack given", NULL);

	struct stackdir* dir = dirs->top;
	if (!dir)
		return STATUS(ST_INT_ISNULL, 0, "Empty stack", NULL);

	/* Remove from top */
	dirs->top = dir->next;

	/* Free memory */
	free(dir->dirname);
	free(dir->name);
	closedir(dir->dir);
	free(dir);

	/* one directory removed */
	dirs->ndir--;

	return STATUS(ST_OK, 0, "Popping directory", NULL);	
}

/* stream_subdir will open the path, and then create a stream to it.
 * If the path is relative, and fd is -1, the path is assumed to be
 * relative to the current working directory of the program,
 * if fd is larger than -1, the path is assumed to be relative to fd.
 * Otherwise, if the path is absolute, the path will be opened as is.
 */
status_t stream_subdir(int fd, const char* path, int oflags, DIR** d)
{
	if (!path)
		return STATUS(ST_INT_ISNULL, 0, "Opening directory", NULL);

	int dir_fd = (fd < 0) ? AT_FDCWD : fd; /* given fd */
	int cfd = -2;  /* the current file descriptor */

	/* openat() will set cfd to -1 if there's an error, we prefer values
	 * below -1.
	 */
	cfd = openat(dir_fd, path, oflags);

	if (cfd < 0)
		return STATUS(ST_ERR_OPEN, errno, "Opening directory", NULL);

	*d = fdopendir(cfd);
	if (*d == NULL) {
		int err = errno;
		close(cfd);
		return STATUS(ST_ERR_OPEN, err, "Opening directory", NULL);
	}

	return STATUS(ST_OK, 0, "Opening directory", NULL);
}

/* path_concat never owns
 */
static char* path_concat(const char* base, const char* name)
{
	/* Handle the special case where the argument specifies the root */
	if (strcmp(base, "") == 0 || strcmp(base, ".") == 0)
		return strdup(name);

	size_t len = strlen(base);
	uint8_t need_seperator = 0;
	/* add to len because we need to append a seperator to base */
	if (base[len - 1] != '/') {
		len = len + 1;
		need_seperator = 1;
	}

	/* allocate space, add 1 for NUL */
	len = len + strlen(name) + 1;
	char* new_path = malloc(len);

	/* check if we failed allocating memory */
	if (!new_path) return NULL;

	/* copy base into new buffer */
	strcpy(new_path, base);

	/* the file path needs a seperator */
	if (need_seperator) strcat(new_path, "/");

	/* concatenate the base and the name */
	new_path = strcat(new_path, name);

	return new_path;
}

/* Will create a kfile struct for use with the `files' hash table.
 *
 * Returns NULL on allocation failure, malloc will set errno.
 */
static inline struct kfile* hcrekey(struct stat* sb)
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
static void* hcreval(const char* path, mode_t st_mode, off_t st_size)
{
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
