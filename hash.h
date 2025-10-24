#ifndef HASH_H
#define HASH_H

#include <stddef.h>		/* size_t */
#include <stdint.h> 		/* uint64_t */

struct hash_entry {
	const void* key;
	void* value;
	struct hash_entry* next;
};

struct hash_table {
	struct hash_entry** buckets;
	size_t size;
	size_t count;
	uint64_t (*hash_fn)(const void* key);
	int (*cmp_fn)(const void* a, const void* b);
};

struct hash_table* hash_create(size_t size,
                         uint64_t (*hash_fn)(const void*),
                         int (*cmp_fn)(const void*, const void*));
void* hash_lookup(struct hash_table* ht, const void* key);
int hash_insert(struct hash_table* ht, const void* key, void* value);
int hash_remove(struct hash_table* ht, const void* key);
size_t hash_foreach(const struct hash_table* ht,
		 int (*func)(const void* key, void* value, void* user_data),
		 void* user_data);
void hash_destroy(struct hash_table* ht);
struct hash_table* hash_resize(struct hash_table* ht, size_t new_size);
size_t hash_chksize(const struct hash_table* ht);


#endif
