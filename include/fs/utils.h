#ifndef FS_UTILS_H
#define FS_UTILS_H

#include <dirent.h>

#include "status.h"

status_t stream_subdir(int fd, const char* path, int oflags, DIR** d);
char* path_concat(const char* base, const char* name);

#endif
