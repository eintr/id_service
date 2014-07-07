#include <stdio.h>

#include <ds_hasht.h>
#include <util_log.h>
#include <util_misc.h>
#include <util_atomic.h>

#include "id_hasht.h"
#include "mod_config.h"

static hasht_t *idpool_hasht=NULL;

static void id_hasht_destroy(void)
{
	hasht_delete(idpool_hasht);
}

int id_hasht_init(struct mapped_id_file_st *arr, int nr)
{
	int i;
	int count=0;

	struct idfile_header_st *hdr;
	char *pos;
	struct idfile_header_st *curid;

	idpool_hasht = hasht_new(NULL, 1000000);	// TODO: config this.
	if (idpool_hasht==NULL) {
		return -1;
	}
	atexit(id_hasht_destroy);

    for (i=0; i<nr; ++i) {
		if (id_hasht_add(arr[i].hdr)==0) {
			count++;
		} else {
			mylog(L_DEBUG, "Failed to hash id[%d]-%s", i, arr[i].hdr->name);
		}
    }
	mylog(L_DEBUG, "Successfully hashed %d ids.", count);
	return 0;
}

struct idfile_header_st *id_hasht_lookup(const char *name)
{
	hashkey_t key;
	struct idfile_header_st *tmp;
	int len;

	len = strlen(name);

	tmp = alloca(sizeof(struct idfile_header_st)+len);
	strcpy(tmp->name, name);
	key.offset = offsetof(struct idfile_header_st, name);
	key.len = len;
	return hasht_find_item(idpool_hasht, &key, tmp);
}

int id_hasht_add(struct idfile_header_st *entry)
{
	hashkey_t key;
	int len;

	len = strlen(entry->name);

	key.offset = offsetof(struct idfile_header_st, name);
	key.len = len;
	return hasht_add_item(idpool_hasht, &key, entry);
}

