#define _POSIX_C_SOURCE 200809L

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include "fs.h"

char* path_concat(const char* base, const char* name);

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
char* path_concat(const char* base, const char* name)
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
