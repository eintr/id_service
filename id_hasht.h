#ifndef ID_HASHT_H
#define ID_HASHT_H

#include "id_file.h"

int id_hash_init(void);

struct id_entry_st *id_hash_lookup(const char *name);

int id_hash_add(struct id_entry_st *);

int id_get(const char *name, uint64_t *dst, int64_t n);

#endif

