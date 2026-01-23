#ifndef FS_TRAVERSE_H
#define FS_TRAVERSE_H

#include "hash.h"
#include "status.h"

#ifndef LOAD_FACTOR_DIRS
#define LOAD_FACTOR_DIRS 0.5
#endif

status_t traverse(const char* restrict path, struct hash_table** files, int oflags);

#endif
