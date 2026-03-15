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


/* Stack owns stackdir, therefore each of its members too.
 * push will handle allocation.
 * pop will free everything in stackdir, including DIR* d.
 */
status_t push(struct stack* dirs, int fd, const char* name, int oflags)
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
	if (!dir->dirname) {
		free(dir->name);
		free(dir);
		return STATUS(ST_ERR_MALLOC, errno, "Building file path", NULL);
	}

	if (ret.c != ST_OK) {
		ret.file_target = strdup(dir->dirname);
		free(dir->dirname);
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

status_t pop(struct stack* dirs)
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
