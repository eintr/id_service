#ifndef ID_POOL_H
#define ID_POOL_H

int id_pool_init(const char *id_config_path);

int id_create(const char *name, uint64_t start);

//int id_list();

int id_get(const char *name, uint64_t *dst, int64_t n);

#endif

