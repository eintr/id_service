#include <stdio.h>

#include <ds_hasht.h>
#include <util_log.h>
#include <util_misc.h>
#include <util_atomic.h>

#include "id_hasht.h"
#include "mod_config.h"

static hasht_t *idpool_hasht=NULL;

static void id_hash_destroy(void)
{
	hasht_delete(idpool_hasht);
}

int id_hash_init(void)
{
	int i;
	int count;

	idpool_hasht = hasht_new(NULL, 1000000);	// TODO: config this.
	if (idpool_hasht==NULL) {
		return -1;
	}
	atexit(id_hash_destroy);

	count = 0;
	for (i=0;i<id_array_size;++i) {
		if (id_hash_add(id_array[i])==0) {
			count++;
		} else {
			mylog(L_DEBUG, "Failed to hash id[%d]-%s", i, id_array[i]->name);
		}
	}
	mylog(L_DEBUG, "Successfully hashed %d ids.", count);
	return 0;
}

struct id_entry_st *id_hash_lookup(const char *name)
{
	hashkey_t key;
	struct id_entry_st *tmp;
	int len;

	len = strlen(name);

	tmp = alloca(sizeof(struct id_entry_st)+len);
	strcpy(tmp->name, name);
	key.offset = offsetof(struct id_entry_st, name);
	key.len = len;
	return hasht_find_item(idpool_hasht, &key, tmp);
}

int id_hash_add(struct id_entry_st *entry)
{
	hashkey_t key;
	int len;

	len = strlen(entry->name);

	key.offset = offsetof(struct id_entry_st, name);
	key.len = len;
	return hasht_add_item(idpool_hasht, &key, entry);
}

int id_get(const char *name, uint64_t *dst, int64_t delta)
{
	struct id_entry_st *ptr;

	ptr = id_hash_lookup(name);
	if (ptr==NULL) {
		return -1;
	}
	*dst = atomic_fetch_and_add64(&ptr->id, delta);
	return 0;
}

