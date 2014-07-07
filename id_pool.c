#include <stdio.h>
#include <stdlib.h>

#include <room_service.h>
#include <util_atomic.h>

#include "id_file.h"
#include "id_hasht.h"
#include "id_pool.h"

#define	INIT_ARRSIZE		1024
#define	EXP_STEP_ARRSIZE	1024

static pthread_mutex_t mut_map_arr = PTHREAD_MUTEX_INITIALIZER;
static struct mapped_id_file_st *map_arr=NULL;
static int nr_map_elm=0, sz_map_arr=0;

static int map_array_appand(struct mapped_id_file_st *item)
{
	struct mapped_id_file_st *newarr;

	pthread_mutex_lock(&mut_map_arr);
	if (nr_map_elm+1 > sz_map_arr) {
		newarr = malloc(sizeof(struct mapped_id_file_st)*(sz_map_arr+EXP_STEP_ARRSIZE));
		if (newarr==NULL) {
			pthread_mutex_unlock(&mut_map_arr);
			return -1;
		}
		memcpy(newarr, map_arr, sizeof(struct mapped_id_file_st)*sz_map_arr);
		sz_map_arr = sz_map_arr+EXP_STEP_ARRSIZE;
		free(map_arr);
		map_arr = newarr;
	}
	memcpy(map_arr+nr_map_elm, item, sizeof(struct mapped_id_file_st));
	nr_map_elm++;
	pthread_mutex_unlock(&mut_map_arr);
	return 0;
}

int id_pool_init(const char *path)
{
	if (id_files_load(path, &map_arr, &nr_map_elm)!=0) {
		mylog(L_INFO, "id_file_load() failed.");
		return -1;
	}
	sz_map_arr = nr_map_elm;
	if (id_hasht_init(map_arr, nr_map_elm)!=0) {
		mylog(L_INFO, "id_hasht_init() failed.");
		return -1;
	}
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
	map_array_appand(&newid);
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

int id_list(MsgIDListEntry ***addr)
{
	int i;
	MsgIDListEntry **index;
	MsgIDListEntry *body;

	pthread_mutex_lock(&mut_map_arr);
	*addr = malloc( sizeof(MsgIDListEntry*)*nr_map_elm + sizeof(MsgIDListEntry)*nr_map_elm);
	if (*addr==NULL) {
		mylog(L_INFO, "%s: Memory exhausted!", __FUNCTION__);
		abort();
	}
	index = (void*)*addr;
	body = (void*)(((char*)(*addr))+sizeof(MsgIDListEntry*)*nr_map_elm);
	for (i=0;i<nr_map_elm;i++) {
		index[i] = body+i;
		msg_idlist_entry__init(body+i);
		body[i].id_name = map_arr[i].hdr->name;
		body[i].current = map_arr[i].hdr->value;
	}
	pthread_mutex_unlock(&mut_map_arr);
	return nr_map_elm;
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

