#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "hash.h"

#ifndef LOAD_FACTOR
#define LOAD_FACTOR 0.5
#endif

static int chkprime(size_t n);
static size_t findprime(size_t n, int nth);

/* Create a hash_table struct of size `size', and populate its state.
 * hash_fn and cmp_fn are for hashing and comparing keys respectively.
 *
 * This function will not limit your freedom to give 0 as `size'.
 * What will happen after that is implementation defined: errno might be set,
 * with calloc returning a NULL pointer, or: a pointer to the allocated space
 * might be returned. It is your responsibility to not use it to access an object.
 */
struct hash_table* hash_create(size_t size, uint64_t (*hash_fn)(const void* key),
			       int (*cmp_fn)(const void* a, const void* b))
{
	struct hash_table* ht = malloc(sizeof(struct hash_table));
	if (!ht)
		return NULL;

	ht->buckets = calloc(size, sizeof(struct hash_entry*));
	if (!ht->buckets) {
		free(ht);
		return NULL;
	}

	ht->size = size;
	ht->count = 0;
	ht->hash_fn = hash_fn;
	ht->cmp_fn = cmp_fn;

	return ht;
}

/* Look for the key given, if found, return the value that's associated.
 *
 * `ht' cannot be NULL. On NULL, the assertion will fail, causing the
 * program to terminate. Key can be NULL: in such cases, hash_lookup
 * will act as if no entries are found.
 */
void* hash_lookup(struct hash_table* restrict ht, const void* key)
{
	struct hash_entry* tp;

	/* Must have a valid hash table */
	assert(ht);

	if (!key)
		/* Return NULL on NULL */
		return NULL;

	for (tp = ht->buckets[ht->hash_fn(key) % ht->size]; tp != NULL; tp = tp->next)
		if (ht->cmp_fn(tp->key, key))
			return tp->value; /* Found the entry */
	return NULL;		   /* Couldn't find the entry */
}

/* Insert the taken key and value into the hash table.
 *
 * Never takes ownership. Use hash_foreach later to free
 * each key and value seperately. If cannot allocate memory,
 * will set errno to ENOMEM and return -1.
 */
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

	size_t seen = 0;
	size_t size = ht->size;
	for (size_t i = 0; i < size; ++i) {
		struct hash_entry* tp = ht->buckets[i];
		while(tp) {
			if (func(tp->key, tp->value, user_data))
				/* early exit */
				return seen;
			tp = tp->next;
			seen++;
		}
	}

	return seen;
}

/* Recursively free each hash_entry in the given hash_table struct.
 *
 * Technical notes:
 * This function will free the given pointer, `ht', too.
 * This function does not free the elements of hash_entry, namely, `key' and `value'.
 * Free them yourself using the hash_foreach function.
 */
void hash_destroy(struct hash_table* ht)
{
	/* Got NULL for some reason */
	if (!ht)
		return;

	size_t size = ht->size;

	for (size_t i = 0; i < size; ++i) {
		struct hash_entry* tp = ht->buckets[i]; /* going through the linked list*/
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

/* Create a new table of size `new_size', and then rehash all entries.
 * It is assumed that your hash is sized by primes. This function will
 * use the next 10th prime as `new_size'.
 *
 * If resizing fails, or is not needed, the old pointer will be returned.
 * Set errno to 0 before this function to check for errors.
 */
struct hash_table* hash_upsize(struct hash_table* restrict ht)
{
	size_t new_size = hash_chksize(ht, 10);
	if (new_size <= ht->size)
		return ht;

	if (new_size == SIZE_MAX)
		return ht;
	
	struct hash_table* nht = hash_rehash(ht, new_size);
	if (!nht)
		return ht;

	return nht;
}

/* Rehashes all entries into a new hash_table of `new_size'.
 * Existing entries are moved, not copied.
 * Safe for both growth and shrink, but shrink will increase collisions.
 *
 * NOTE: Will free the old table and its buckets array, and return the new table.
 * Does not duplicate or free individual entries.
 * Use only for reallocation, not for copying or merging tables.
 */
struct hash_table* hash_rehash(struct hash_table* ht, size_t new_size)
{
	struct hash_table* nht;
	uint64_t h;

	if (!ht || new_size == 0)
		return NULL;

	nht = hash_create(new_size, ht->hash_fn, ht->cmp_fn);
	/* Couldn't allocate */
	if (!nht)
		return NULL;

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

size_t hash_chksize(const struct hash_table* restrict ht, int n)
{
	if (((double)ht->count / ht->size) >= LOAD_FACTOR) {
		size_t next = findprime(ht->size * 2, n);
		if (next == 0)
			/* next overflowed */
			return SIZE_MAX;
		
		return next;
	}
	return 0;
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

size_t findprime(size_t n, int nth)
{
	/* Base case */
	if (n <= 1)
		return 2;

	size_t cn = n;
	int i = 0;

	/* Until chkprime returns true */
	while (cn < SIZE_MAX) {
		cn++;

		if (chkprime(cn))
			i++;

		if (i == nth)
			return cn;
	}

	errno = EOVERFLOW;
	return 0;
}
