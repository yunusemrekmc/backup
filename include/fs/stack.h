#ifndef FS_STACK_H
#define FS_STACK_H

#include <dirent.h>
#include <unistd.h>

#include "status.h"

struct stackdir {
	char* name;		/* file name */
	DIR* dir;		/* this directory */
	char* dirname;		/* dirname of the current folder */
	struct stackdir* next;	/* next frame */
};

struct stack {
	size_t ndir;		/* this many dirs */
	struct stackdir* top;	/* any directory */
};

status_t push(struct stack* dirs, int fd, const char* name, int oflags);
status_t pop(struct stack* dirs);

#endif
