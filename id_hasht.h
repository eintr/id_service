#ifndef ID_HASHT_H
#define ID_HASHT_H

#include "id_file.h"

int id_hasht_init(struct mapped_id_file_st *arr, int nr);

struct idfile_header_st *id_hasht_lookup(const char *name);

int id_hasht_add(struct idfile_header_st*);

int id_get(const char *name, uint64_t *dst, int64_t n);

#endif

