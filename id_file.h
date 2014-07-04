#ifndef ID_FILE_H
#define ID_FILE_H

#include "id_file_format.h"

extern struct id_entry_st **id_array;
extern int id_array_size;

int id_file_load(const char *fname);

struct id_entry_st *id_file_append(const char *name, uint64_t start);

void id_file_sync(void);

void id_file_spin_lock(void);
void id_file_spin_unlock(void);

#endif

