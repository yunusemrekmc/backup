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
status_t cp(int src, int dst, struct buf* buf)
{
	ssize_t n;
	while ((n = read(src, buf->b, buf->cap)) > 0) {
		if (n < 0) {
		}
		write_wrap(dst, buf->b, n);
	}

	return STATUS(ST_OK, 0, "Copied bytes successfully.", NULL);
err_return:
	return ret;
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

		/* anything else is hard error anyway */
		return -1;
	}

	return total; 		/* equals n on success */
}

status_t copy(const struct hash_table* files, const char* path, int oflags)
{
	return STATUS(ST_OK, 0, "Copied file successfully.", NULL);
}
