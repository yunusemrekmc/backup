#ifndef FS_H
#define FS_H

#include <stdint.h>

#include "status.h"

/* key for our hash table that stores each file */
struct kfile {
	uint64_t st_dev;	/* file device number */
	uint64_t st_ino;	/* file inode number */
};

struct file {
	int pfd; 		/* parent fd */
	char* path;		/* relative to pfd */
};

status_t traverse(const char* path);
uint64_t hash(const void* key);
int hcmpent(const void* a, const void* b);


#endif
