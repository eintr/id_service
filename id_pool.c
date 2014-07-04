#include <stdio.h>
#include <stdlib.h>

#include <room_service.h>
#include <util_atomic.h>

#include "id_file.h"
#include "id_hasht.h"


int id_pool_init(const char *filename)
{
	if (id_file_load(filename)!=0) {
		mylog(L_INFO, "id_file_load() failed.");
		return -1;
	}
	if (id_hash_init()!=0) {
		mylog(L_INFO, "id_hash_init() failed.");
		return -1;
	}
	return 0;
}

int id_create(const char *name, uint64_t start)
{
	struct id_entry_st *newid, *result;

	id_file_spin_lock();
	result = id_hash_lookup(name);
	if (result!=NULL) {
		mylog(L_INFO, "id[%s] already exists.", name);
		goto fail;
	}
	mylog(L_INFO, "id[%s] is new.", name);
	newid = id_file_append(name, start);
	if (newid==NULL) {
		mylog(L_INFO, "id[%s] id_file_append() failed.", name);
		goto fail;
	}
	mylog(L_INFO, "id[%s] is appended.", newid->name);
	if (id_hash_add(newid)!=0) {
		mylog(L_INFO, "id[%s] id_hash_add() failed.", newid->name);
		goto fail;
		// TODO: ROLL BACK!
	}
	mylog(L_INFO, "id[%s] is hashed.", newid->name);
	id_file_spin_unlock();
	return 0;
fail:
	id_file_spin_unlock();
	return -1;
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

