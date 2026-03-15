#ifndef FS_H
#define FS_H

#include <stdint.h>
#include <unistd.h>
#include <dirent.h>

#include "hash.h"
#include "status.h"

#ifndef LOAD_FACTOR
#define LOAD_FACTOR 0.5
#endif

/* key for our hash table that stores each file */
struct kfile {
	uint64_t st_dev;	/* file device number */
	uint64_t st_ino;	/* file inode number */
};

struct file {
	int pfd; 		/* parent fd */
	char* path;		/* relative to pfd */
};

struct stack {
	size_t ndir;		/* this many dirs */
	struct stackdir* top;	/* any directory */
};

status_t traverse(const char* restrict path, struct hash_table* files, int follow);
int free_hent(const void* key, void* value, void* user_data);
status_t searchdir(struct stack* dirs, DIR* d, struct hash_table** files, int statflags);
uint64_t hash(const void* key);
int hcmpent(const void* a, const void* b);


#endif
