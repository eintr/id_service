#ifndef ID_FILE_H
#define ID_FILE_H

#include "id_file_format.h"

struct mapped_id_file_st {
	void *map_addr;
	struct idfile_header_st *hdr;
	off_t map_size;
};

int id_files_load(const char *path, struct mapped_id_file_st **arr, int *nr);

char *id_file_create(const char *idname, uint64_t start);

int id_file_map(struct mapped_id_file_st *res, const char *fname);

#endif

