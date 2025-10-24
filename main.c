/* A cp clone: copies files from one place to another */
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hash.h"

#define EXIT_FAIL 1
#define NO_FOLLOW 0
#define FOLLOW	  1

uint64_t hash(const void* key);
int hcmpent(const void* a, const void* b);
int open_subdir(int fd, const char* path, int follow);
int traverse(const char* src, int dirfd, int follow);
int copy(char* src, char* dst);

int main(int argc, char* argv[])
{
	if (argc < 3) {
		fprintf(stderr, "Usage: backup SOURCE DESTINATION");
		return 1;
	}

	char* src = argv[1];
	char* dst = argv[2];

	traverse(src, -1, NO_FOLLOW);

	return 0;
}

int traverse(const char* src, int dirfd, int follow)
{
	/* Set variables */
	dirfd = open_subdir(dirfd, src, follow);
	DIR* dir = fdopendir(dirfd);
	struct dirent* entry;
	struct stat sb;
	struct hash_table* ht = hash_create(4099, hash, hcmpent);
	/* check if we were able to open the file */
	if (!dir) {
		perror("main.c:44 fdopendir");
		exit(EXIT_FAIL);
	}
	errno = 0;
	while((entry = readdir(dir)) != NULL) {
		if (strcmp(entry->d_name, ".") == 0 ||
		    strcmp(entry->d_name, "..") == 0) continue;
	
		/* Get the current file, set pointer to the next */
		if (fstatat(dirfd, entry->d_name, &sb, AT_SYMLINK_NOFOLLOW) == -1) {
			perror("main.c:53 fstatat");
			continue;
		}
		if (S_ISDIR(sb.st_mode)) {
			char* child_dir = malloc(strlen(entry->d_name) + 1);
			strcpy(child_dir, entry->d_name);
			traverse(child_dir, dirfd, follow);
			printf("Directory");
		}
		else if (S_ISREG(sb.st_mode))
			printf("File");
		else if (S_ISLNK(sb.st_mode))
			printf("Symlink");
		else if (S_ISSOCK(sb.st_mode))
			printf("Socket");
		else if (S_ISFIFO(sb.st_mode))
			printf("Fifo");

		printf(" name: %s, File size: %zu, File mode: %i.\n",
		       entry->d_name, sb.st_size, sb.st_mode & 0777);
	}
	if (!entry && errno != 0) {
		perror("main.c:74 readdir");
		closedir(dir);
		exit(EXIT_FAIL);
	}
	closedir(dir);

	return 0;
}

int open_subdir(int fd, const char* path, int follow)
{
	int flags = O_RDONLY | O_DIRECTORY;
	if (follow == NO_FOLLOW)
		flags |= O_NOFOLLOW;

	return (fd == -1)
		? openat(AT_FDCWD, path, flags);
		: openat(fd, path, flags);
}

uint64_t hash(const void* key)
{
	const struct stat* st = key;
	return (uint64_t)st->st_ino ^ ((uint64_t)st->st_dev << 7);
}

int hcmpent(const void* a, const void* b)
{
	const struct stat* st1 = a;
	const struct stat* st2 = b;
	if (st1->st_ino == st2->st_ino && st1->st_dev == st2->st_dev)
		return 1;
	return 0;
}

int copy(char* src, char* dst)
{
	

	return 0;
}
