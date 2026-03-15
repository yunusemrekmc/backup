#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#include "fs.h"
#include "hash.h"
#include "status.h"

#define BUF_LIMIT 8192
#ifndef BUF_WANT_SIZE
#define BUF_WANT_SIZE 65536
#endif

struct buf {
	size_t cap;
	char* b;
};

status_t ret;

status_t preparecp(struct buf *b);
static ssize_t write_wrap(int fd, char* restrict buf, size_t n);

/* Prepares a buffer. Saves its pointer and size to b.
 *
 * On failure, struct b is not modified.
 * This function will return a status_t struct. Check that for errors.
 */
status_t preparecp(struct buf* b)
{
	char* buf = NULL;
	size_t buf_size = BUF_WANT_SIZE;

	if (!b) return STATUS(ST_INT_ISNULL, 0, "Got NULL for argument.", NULL);
	buf = malloc(buf_size);
	if(!buf) {
		buf_size = BUF_LIMIT;
		buf = malloc(buf_size);
	}
	if (!buf)
		return STATUS_E(ST_ERR_MALLOC, "Creating a buffer for copying", NULL);

	b->b = buf;
	b->cap = buf_size;
	return STATUS(ST_OK, 0, "Opening files for read", NULL);
}

/* Copy a file over until EOF received.
 *
 * src and dst are both file descriptors that respectively refer to:
 * the source and the destination file.
 * buf is a buffer prepared beforehand via preparecp()
 */
status_t cp(int src, int dst, struct buf* buf, const char* filename)
{
	ssize_t n;
	status_t ret;
	while ((n = read(src, buf->b, buf->cap)) > 0) {
		if (write_wrap(dst, buf->b, n) < 0) {
			return STATUS_E(ST_ERR_FILEWR, "Couldn't write data", strdup(filename));
		}
	}

	return STATUS(ST_OK, 0, "Copied bytes successfully.", NULL);
}

static ssize_t write_wrap(int fd, char* restrict buf, size_t n)
{
	size_t total = 0;
	while (total < n) {
		ssize_t n = write(fd, buf, n);
		if (n > 0) {
			total += n;
			continue;
		}

		if (n == 0) {
			/* ts shouldn't happen on a regular file/pipe
			 * but you never know, treat like error anyway
			 */
			errno = EIO;
			return -1;
		}

		/* ret < 0 */
		if (errno == EINTR) {
			/* interrupted -> just try again */
			continue;
		}

		/* anything else is a hard error anyway */
		return -1;
	}

	return total;		/* equals n on success */
}

int perkey(const void* key, void* value, void* user_data)
{
	
	return 0;
}

status_t copy(const struct hash_table* files, const char* path, int oflags)
{
	hash_foreach(files);
	return STATUS(ST_OK, 0, "Copied file successfully.", NULL);
}
