#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "hash.h"

static int chkprime(size_t n);
static size_t findprime(size_t n);

struct hash_table* hash_create(size_t size, uint64_t (*hash_fn)(const void* key),
			       int (*cmp_fn)(const void* a, const void* b))
{
	struct hash_table* ht = malloc(sizeof(struct hash_table));
	if (!ht) {
		errno = ENOMEM;
		return NULL;
	}

	ht->buckets = calloc(size, sizeof(struct hash_entry*));
	if (!ht->buckets) {
		free(ht);
		errno = ENOMEM;
		return NULL;
	}

	ht->size = size;
	ht->count = 0;
	ht->hash_fn = hash_fn;
	ht->cmp_fn = cmp_fn;

	return ht;
}

void* hash_lookup(struct hash_table* ht, const void* key)
{
	struct hash_entry* tp;

	for (tp = ht->buckets[ht->hash_fn(key) % ht->size]; tp != NULL; tp = tp->next)
		if (ht->cmp_fn(tp->key, key))
			return tp->value; /* Found the entry */
	return NULL;		   /* Couldn't find the entry */
}

int hash_insert(struct hash_table* ht, const void* key, void* value)
{
	struct hash_entry* tp;
	uint64_t h;

	if ((tp = hash_lookup(ht, key)) == NULL) {
		tp = malloc(sizeof(struct hash_entry));
		if (!tp) {
			errno = ENOMEM;
			return -1; /* Couldn't acquire memory */
		}
		h = ht->hash_fn(key) % ht->size;
		tp->key = key;
		tp->value = value;
		tp->next = ht->buckets[h];
		ht->buckets[h] = tp;
		ht->count++;
		return 0; 	/* Done */
	}
	return 1; 		/* Already here */
}

int hash_remove(struct hash_table* ht, const void* key)
{
	uint64_t h = ht->hash_fn(key) % ht->size;
	struct hash_entry* prev = NULL;
        struct hash_entry *tp = ht->buckets[h];

        while (tp) {
		if (ht->cmp_fn(tp->key, key)) {
			if (prev) prev->next = tp->next;
			else ht->buckets[h] = tp->next;
			free(tp);
			return 0; /* found */
		}
		prev = tp;
		tp = tp->next;
        }
	return 1; 		/* not found */
}

size_t hash_foreach(const struct hash_table* ht,
		 int (*func)(const void* key, void* value, void* user_data),
		 void* user_data)
{
	if (!ht)
		/* Got NULL for some reason */
		return 0;

	size_t entc;
	size_t size = ht->size;
	for (size_t i = 0; i < size; ++i) {
		struct hash_entry* tp = ht->buckets[i];
		while(tp) {
			if (func(tp->key, tp->value, user_data))
				/* early exit */
				return entc;
			tp = tp->next;
			entc++;
		}
	}

	return entc;
}

void hash_destroy(struct hash_table* ht)
{
	/* Got NULL for some reason */
	if (!ht)
		return;

	size_t size = ht->size;

	for (size_t i = 0; i < size; ++i) {
		struct hash_entry* tp = ht->buckets[i]; /* iterate through the array */
		while (tp) {
			/* copy the next pointer before freeing */
			struct hash_entry* next = tp->next;
			free(tp);
			tp = next;
		}
	}

	free(ht->buckets);
	free(ht);
}

struct hash_table* hash_resize(struct hash_table* ht, size_t new_size)
{
	struct hash_table* nht;
	uint64_t h;

	nht = hash_create(new_size, ht->hash_fn, ht->cmp_fn);
	/* Couldn't allocate */
	if (!nht)
		return NULL;

	nht->size = new_size;
	nht->count = ht->count;
	nht->hash_fn = ht->hash_fn;
	nht->cmp_fn = ht->cmp_fn;

	for (size_t i = 0; i < ht->size; ++i) {
		struct hash_entry* tp = ht->buckets[i];
		while (tp) {
			struct hash_entry* next = tp->next;
			h = nht->hash_fn(tp->key) % nht->size;
			tp->next = nht->buckets[h];
			nht->buckets[h] = tp;
			tp = next;
		}
	}
	/* Resizing done */
	free(ht->buckets);
	free(ht);
	return nht;
}

size_t hash_chksize(const struct hash_table* ht)
{
	if ((double)ht->count / ht->size > 0.75) {
		size_t next = findprime(ht->count * 2);
		if (next == 0) {
			perror("Memory exhausted");
			return ht->count;
		}
		if (ht->count > ht->size) {
			return next;
		}
	}
	return ht->size;
}

int chkprime(size_t n)
{
	/* Found this online */
	/* is n 1 or 0 */
	if (n <= 1)
		return 0;
	/* is 2 or 3 */
	if (n == 2 || n == 3)
		return 1;
	/* is it divisible by 2 or 3 */
	if (n % 3 == 0 || n % 2 == 0)
		return 0;
	/* Check from 5 to square root of n */
	/* Iterate i by (i+6) */
	for (size_t i = 5; i*i<=n; i = i + 6)
		if (n % i == 0 || n % (i + 2) == 0)
			return 0;
	/* return true */
	return 1;
}

size_t findprime(size_t n)
{
	/* Base case */
	if (n <= 1)
		return 2;

	size_t cn = n;

	/* Until chkprime returns true */
	while (cn < SIZE_MAX) {
		cn++;

		if (chkprime(cn))
			return cn;
	}

	errno = EOVERFLOW;
	return 0;
}
