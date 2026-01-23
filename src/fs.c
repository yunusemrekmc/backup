#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "hash.h"
#include "fs.h"
#include "status.h"

#define SRC_NAME "fs.c"

static struct stackdir {
	int pfd; 		/* parent fd */
	char* path;		/* relative to pfd */
	DIR* dir;		/* this directory */
	struct stackdir* next;	/* next frame */
};

static struct stack {
	size_t ndir;		/* this many dirs */
	struct stackdir* top;	/* any directory */
};

static int free_hent(const void* key, void* value, void* user_data);
static status_t push(struct stack* dirs, int fd, char* path, DIR* d);

status_t traverse(const char* restrict path)
{
	struct stack* dirs = malloc(sizeof(struct stack));
	if (!dirs)
		return (status_t){ST_ERR_NOMEM, errno, "Creating stack"};

	int fd = openat(AT_FDCWD, path, O_DIRECTORY | O_RDONLY | O_NOFOLLOW);
	if (fd < 0) {
		int err = errno;
		free(dirs);
		return (status_t){ST_ERR_OPEN, err, "SRC_NAME: Opening directory"};
	}

	DIR* d = fdopendir(fd);
	if (!d) {
		int err = errno;
		free(dirs);
		close(fd);
		return (status_t){ST_ERR_OPEN, err, "SRC_NAME: Opening directory"};
	}

	struct hash_table* files = hash_create(4099, hash, hcmpent);
	if (!files) {
		int err = errno;
		free(dirs);
		closedir(d);
		return (status_t){ST_ERR_NOMEM, err, "SRC_NAME: Allocating space for hash table"};
	}

	struct dirent* ent;
	struct stat sb;
	while ((ent = readdir(d)) != NULL) {
		if (strcmp(ent->d_name, ".") == 0 ||
		    strcmp(ent->d_name, "..") == 0) continue;
		if (fstatat(fd, ent->d_name, &sb, AT_SYMLINK_NOFOLLOW) == -1) {
			sterr((status_t){ST_ERR_FILERD, errno, "SRC_NAME: Searching directory"});
			continue;
		}

		struct kfile* key = malloc(sizeof(struct kfile));
		struct file* f = malloc(sizeof(struct file));
		if (!key || !f) {
			free(dirs);
			closedir(d);
			free(files);
			return (status_t){ST_ERR_NOMEM, errno, "SRC_NAME: Allocating space for hash entry"};
		}
		key->st_dev = (uint64_t)sb.st_dev;
		key->st_ino = (uint64_t)sb.st_ino;
		f->pfd = fd;
		f->path = strdup(ent->d_name);

		if (!f->path) {
			free(dirs);
			closedir(d);
			hash_foreach(files, free_hent, NULL);
			hash_destroy(files);
			free(key);
			free(f);
			return (status_t){ST_ERR_NOMEM, errno, "SRC_NAME: Storing filename"};
		}
		size_t nsize = hash_chksize(files);
		struct hash_table* nfiles;
		if (nsize > files->size) nfiles = hash_resize(files, nsize);
		if (!nfiles) {
			free(dirs);
			closedir(d);
			hash_foreach(files, free_hent, NULL);
			hash_destroy(files);
			free(key);
			free(f);
			return (status_t){ST_ERR_NOMEM, errno, "SRC_NAME: Resizing hash table"};
		}

		if (hash_insert(files, key, f) < 0) {
			free(dirs);
			closedir(d);
			hash_foreach(files, free_hent, NULL);
			hash_destroy(files);
			free(key);
			free(f);
			return (status_t){ST_ERR_NOMEM, errno, "SRC_NAME: Inserting into hash table"};
		}

		if (S_ISDIR(sb.st_mode)) {
			push(dirs, fd, ent->d_name, d);
		}
	}
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"

/* The hash table never owns the key and value. */
/* Free them seperately. */
int free_hent(const void* key, void* value, void* user_data)
{
	free((void*)key);
	free(value);
	return 0;
}

#pragma GCC diagnostic pop


int traverse(const char* src, int dirfd, int follow)
{
	/* Set variables */
	dirfd = open_subdir(dirfd, src, follow);
	DIR* dir = fdopendir(dirfd);
	struct dirent* entry;
	struct stat sb;
	struct hash_table* ht = hash_create(4099, hash, hcmpent);
	/* check if we were able to open the file */
	if (!dir) {
		perror("main.c:44 fdopendir");
		exit(EXIT_FAIL);
	}
	errno = 0;
	while((entry = readdir(dir)) != NULL) {
		if (strcmp(entry->d_name, ".") == 0 ||
		    strcmp(entry->d_name, "..") == 0) continue;
	
		/* Get the current file, set pointer to the next */
		if (fstatat(dirfd, entry->d_name, &sb, AT_SYMLINK_NOFOLLOW) == -1) {
			perror("main.c:53 fstatat");
			continue;
		}
		if (S_ISDIR(sb.st_mode)) {
			char* child_dir = malloc(strlen(entry->d_name) + 1);
			strcpy(child_dir, entry->d_name);
			traverse(child_dir, dirfd, follow);
			free(child_dir);
			printf("Directory");
		}
		else if (S_ISREG(sb.st_mode))
			printf("File");
		else if (S_ISLNK(sb.st_mode))
			printf("Symlink");
		else if (S_ISSOCK(sb.st_mode))
			printf("Socket");
		else if (S_ISFIFO(sb.st_mode))
			printf("Fifo");

		hash_insert(ht, &sb, entry->d_name);
		
		printf(" name: %s, File size: %zu, File mode: %i.\n",
		       entry->d_name, sb.st_size, sb.st_mode & 0777);
	}
	if (!entry && errno != 0) {
		perror("main.c:74 readdir");
		closedir(dir);
		exit(EXIT_FAIL);
	}

	closedir(dir);

	int* c = malloc(sizeof(int));
	hash_foreach(ht, foreach, c);
	hash_foreach(ht, freet, NULL);
	hash_destroy(ht);
	free(c);

	return 0;
}

/* Stack owns stackdir, therefore each of its members too.
 * push will handle allocation.
 * pop will free everything in stackdir, including DIR* d.
 */
static status_t push(struct stack* dirs, int fd, char* path, DIR* d)
{
	if (!dirs)
		/* Got NULL for some reason */
		return (status_t){ST_INT_ISNULL, 0, "pushing directory"};

	for (size_t i = 0; i < dirs->ndir; ++i) {
		
	}

	return (status_t){ST_OK, 0, "pushing directory"};
}

static status_t pop(struct stackdir* stdir)
{
	
}

int open_subdir(int fd, const char* path, int follow)
{
	int flags = O_RDONLY | O_DIRECTORY;
	if (follow == NO_FOLLOW)
		flags |= O_NOFOLLOW;

	return (fd == -1)
		? openat(AT_FDCWD, path, flags)
		: openat(fd, path, flags);
}

uint64_t hash(const void* key)
{
	const struct kfile* f = key;
	return f->st_ino ^ (f->st_dev << 7);
}

int hcmpent(const void* a, const void* b)
{
	const struct stat* st1 = a;
	const struct stat* st2 = b;
	if (st1->st_ino == st2->st_ino && st1->st_dev == st2->st_dev)
		return 1;
	return 0;
}
