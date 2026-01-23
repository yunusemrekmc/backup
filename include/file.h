#ifndef FILE_H
#define FILE_H

#include <sys/stat.h>

struct key {
	st_dev st_dev;
	st_ino st_ino;
};

typedef struct {
	int pfd; 		/* Parent fd */
	char* name;		/* relative to pfd */
} file_t;

#endif
