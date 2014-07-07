#include <stdio.h>
#include <stdlib.h>

#include <room_service.h>
#include <util_atomic.h>

#include "id_file.h"
#include "id_hasht.h"


int id_pool_init(const char *path)
{
	struct mapped_id_file_st *map_arr=NULL;
	int nr=0;

	if (id_files_load(path, &map_arr, &nr)!=0) {
		mylog(L_INFO, "id_file_load() failed.");
		return -1;
	}
	if (id_hasht_init(map_arr, nr)!=0) {
		mylog(L_INFO, "id_hasht_init() failed.");
		free(map_arr);
		return -1;
	}
	free(map_arr);
	return 0;
}

int id_create(const char *name, uint64_t start)
{
	struct idfile_header_st *result;
	struct mapped_id_file_st newid;
	int fd;
	char *fname;

	result = id_hasht_lookup(name);
	if (result!=NULL) {
		mylog(L_INFO, "id[%s] already exists.", name);
		goto fail;
	}
	mylog(L_DEBUG, "id[%s] is new.", name);
	fname = id_file_create(name, start);
	if (fname==NULL) {
		mylog(L_INFO, "id[%s] id_file_create() failed.", name);
		goto fail;
	}
	mylog(L_INFO, "id[%s] file was created.", name);
	if (id_file_map(&newid, fname)!=0) {
		mylog(L_INFO, "id[%s] id_file_map() failed.", newid.hdr->name);
		goto fail1;
	}
	if (id_hasht_add(newid.hdr)!=0) {
		mylog(L_INFO, "id[%s] id_hash_add() failed.", newid.hdr->name);
		goto fail1;
		// TODO: ROLL BACK!
	}
	mylog(L_INFO, "id[%s] is hashed.", newid.hdr->name);
	return 0;
fail1:
	free(fname);
fail:
	return -1;
}

int id_get(const char *name, uint64_t *dst, int64_t delta)
{
    struct idfile_header_st *ptr;

    ptr = id_hasht_lookup(name);
    if (ptr==NULL) {
        return -1;
    }
    *dst = atomic_fetch_and_add64(&ptr->value, delta);
    return 0;
}

