#ifndef FS_HASH_H
#define FS_HASH_H

#include <sys/stat.h>
#include <stdint.h>
#include <stdlib.h>

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

int free_hent(const void* key, void* value, void* user_data);
uintmax_t hash(const void* key);
int hcmpent(const void* a, const void* b);
struct kfile* hcrekey(struct stat* sb);
void* hcreval(const char* path, mode_t st_mode, off_t st_size);
#endif
