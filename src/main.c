/* A cp clone: copies files from one place to another */

uint64_t hash(const void* key);
int hcmpent(const void* a, const void* b);
int open_subdir(int fd, const char* path, int follow);
int foreach(const void* key, void* value, void* user_data);
int freet(const void* key, void* value, void* user_data);
int copy(char* src, char* dst);

int main(int argc, char* argv[])
{
	if (argc < 3) {
		fprintf(stderr, "Usage: backup SOURCE DESTINATION");
		return 1;
	}

	char* src = argv[1];
	char* dst = argv[2];

	return 0;
}


int open_subdir(int fd, const char* path, int follow)
{
	int flags = O_RDONLY | O_DIRECTORY;
	if (follow == NO_FOLLOW)
		flags |= O_NOFOLLOW;

	return (fd == -1)
		? openat(AT_FDCWD, path, flags)
		: openat(fd, path, flags);
}

uint64_t hash(const void* key)
{
	const struct hfile* f = key;
	return f->st_ino ^ (f->st_dev << 7);
}

int hcmpent(const void* a, const void* b)
{
	const struct stat* st1 = a;
	const struct stat* st2 = b;
	if (st1->st_ino == st2->st_ino && st1->st_dev == st2->st_dev)
		return 1;
	return 0;
}

int freet(const void* key, void* value, void* user_data)
{
	free(value);
	return 0;
}

int foreach(const void* key, void* value, void* user_data)
{
	int* c = user_data;
	const struct stat* sb = key;
	char* path = value;

	if (S_ISREG(sb->st_mode)) {
		printf("regular file, %s\n", path);
		(*c)++;
	}
	return 0;
}

int copy(char* src, char* dst)
{
	

	return 0;
}
