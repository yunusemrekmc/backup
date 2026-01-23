#ifndef FS_H
#define FS_H

#include <sys/stat.h>
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
	uintmax_t st_dev;	/* file device number */
	uintmax_t st_ino;	/* file inode number */
};

struct file {
	char* path;		/* relative to source */
	mode_t mode; 		/* file mode */
	off_t size;		/* file size */
};

struct stack {
	size_t ndir;		/* this many dirs */
	struct stackdir* top;	/* any directory */
};

status_t traverse(const char* restrict path, struct hash_table** files, int oflags);
int free_hent(const void* key, void* value, void* user_data);
status_t searchdir(struct stack* dirs, DIR* d, struct hash_table** files, int statflags);
status_t stream_subdir(int fd, const char* path, int oflags, DIR** d);
uintmax_t hash(const void* key);
int hcmpent(const void* a, const void* b);

#endif


