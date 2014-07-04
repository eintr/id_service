#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <alloca.h>

#include <util_log.h>

#include "id_file_format.h"

static char *file_map;
static off_t file_size;
static int file_des;
static off_t tail_pos;

/** Used for hasht init ONLY, free'ed in id_hash_init() */
struct id_entry_st **id_array=NULL;
int id_array_size=0;

static int roundup_64(int n)
{
	return n%8 ? n/8*8+8 : n;
}

static int calc_reclen_align64(const char *name)
{
	return roundup_64(sizeof(struct id_entry_st)+strlen(name));
}

static int create_default_id_file(const char *fname)
{
	struct idfile_header_st *hdr;
	int hdr_size;
	struct id_entry_st *id0, *idtail;
	int id0_len, idtail_len, ret;
	int fd;

	fd = open(fname, O_RDWR|O_CREAT, 0600);
	if (fd<0) {
		return -1;
	}

	hdr_size = roundup_64(sizeof(struct idfile_header_st));
	hdr = alloca(hdr_size);
	memset(hdr, 0, hdr_size);
	memcpy(&hdr->magic, "LEID", 4);
	hdr->id0_offset = hdr_size;
	hdr->clean_mark = 0;

	id0_len=calc_reclen_align64(DEFAULT_ID_NAME);
	id0 = alloca(id0_len);
	memset(id0, 0, id0_len);
	id0->id=1;
	id0->rec_len = id0_len;
	strcpy(id0->name, DEFAULT_ID_NAME);

	idtail_len = calc_reclen_align64("");
	idtail = alloca(idtail_len);
	memset(idtail, 0, idtail_len);
	idtail->id=0;
	idtail->rec_len = 0;

	ret = write(fd, hdr, hdr->id0_offset);
	if (ret<hdr->id0_offset) {
		goto drop_fail;
	}
	ret = write(fd, id0, id0->rec_len);
	if (ret<id0->rec_len) {
		goto drop_fail;
	}
	ret = write(fd, idtail, idtail_len);
	if (ret<idtail_len) {
		goto drop_fail;
	}
	lseek(fd, 0, SEEK_SET);
	return fd;

drop_fail:
	close(fd);
	return -1;
}

static int count_ids_in_file(int fd)
{
	struct idfile_header_st hdr;
	struct id_entry_st idbuf;
	off_t savepos;
	int count=0, ret;

	savepos = lseek(fd, 0, SEEK_CUR);

	lseek(fd, 0, SEEK_SET);
	read(fd, &hdr, sizeof(hdr));

	lseek(fd, hdr.id0_offset, SEEK_SET);

	while (1) {
		tail_pos = lseek(fd, 0, SEEK_CUR);
		ret = read(fd, &idbuf, sizeof(idbuf));
		if (ret==0) {
			mylog(L_ERR, "ID file unexpectly EOF!");
			break;
		}
		if (ret<sizeof(idbuf)) {
			mylog(L_ERR, "ID file struct is bad!");
			break;
		}
		if (idbuf.rec_len==0) {
			break;
		}
		lseek(fd, idbuf.rec_len-sizeof(idbuf), SEEK_CUR);
		count++;
	}

	lseek(fd, savepos, SEEK_SET);

	return count;
}

static int map_id_file(int fd)
{
	struct idfile_header_st *hdr;
	int i,count;
	struct id_entry_st *curid;
	char *pos;

	count = count_ids_in_file(fd);
	id_array = malloc(sizeof(void*)*count);

	file_map = mmap(NULL, file_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if (file_map==MAP_FAILED) {
		mylog(L_ERR, "mmap(): %m");
		return -1;
	}

	hdr = (void*)file_map;

	pos = file_map + hdr->id0_offset;
	curid = (void*)pos;
	for (i=0;i<count;++i) {
		id_array[i] = curid;
		id_array_size++;
		pos += curid->rec_len;
		curid = (void*)pos;
	}

	return 0;
}

static int check_id_filed(int fd)
{
	off_t savepos;
	struct stat stat_res;
	struct idfile_header_st hdr;
	int ret;

	if (fstat(fd, &stat_res)) {
		mylog(L_ERR, "fstat() failed: %m");
		return -1;
	}
	if (!S_ISREG(stat_res.st_mode)) {
		mylog(L_ERR, "id file is not a regular file.");
		return -1;
	}

	file_size = stat_res.st_size;

	savepos = lseek(fd, 0, SEEK_CUR);

	lseek(fd, 0, SEEK_SET);
	ret = read(fd, &hdr, sizeof(hdr));
	if (ret<sizeof(hdr)) {
		mylog(L_ERR, "id file seems too small.");
		return -1;
	}

	if (memcmp(&hdr.magic, "BEID", 4)==0) {
		mylog(L_ERR, "id file is not native endian coded. Convert please.");
		return -1;
	}

	if (memcmp(&hdr.magic, "LEID", 4)!=0) {
		mylog(L_ERR, "Can't understand this id file.");
		return -1;
	}

	// TODO: Check also the file struct.

	lseek(fd, savepos, SEEK_SET);

	return 0;
}


static int open_or_create(const char *fname)
{
	int fd;

	fd = open(fname, O_RDWR);
	if (fd<0 && errno==ENOENT) {
		fd = create_default_id_file(fname);
		if (fd<0) {
			mylog(L_ERR, "Can't create new id file: %m");
			return -1;
		}
		return fd;
	} else if (fd<0) {
		mylog(L_ERR, "Can't open id file(%s): %m", fname);
		return -1;
	} else {
		return fd;
	}
}

void id_file_spin_lock(void)
{
	struct idfile_header_st *file_hdr;

	file_hdr = (void*)file_map;

	while (!__sync_bool_compare_and_swap(&file_hdr->clean_mark, FILE_CLEAN, FILE_DIRTY));
}

void id_file_spin_unlock(void)
{
	struct idfile_header_st *file_hdr;

	file_hdr = (void*)file_map;

	file_hdr->clean_mark = FILE_CLEAN;
}

static void id_file_unload(void)
{
	close(file_des);
	msync(file_map, file_size, MS_SYNC);
	munmap(file_map, file_size);
}

int id_file_load(const char *fname)
{
	int fd;

	fd = open_or_create(fname);
	if (fd<0) {
		mylog(L_ERR, "open_or_create(%s) failed.", fname);
		return -2;
	}
	if (check_id_filed(fd)!=0) {
		mylog(L_ERR, "Check id file(%s) failed.", fname);
		close(fd);
		return -2;
	}
	if (map_id_file(fd)!=0) {
		mylog(L_ERR, "Map id file(%s) failed.", fname);
		close(fd);
		return -2;
	}
	file_des = fd;
	atexit(id_file_unload);
	return 0;
}

struct id_entry_st *id_file_append(const char *name, uint64_t start)
{
	struct id_entry_st *newid;
	char *tailid;
	int tailid_len;
	int newid_len;
	off_t newid_pos, newtailid_pos;
	void *p;

	tailid_len = calc_reclen_align64("");
	tailid = alloca(tailid_len);
	memset(tailid, 0, tailid_len);

	newid_len = calc_reclen_align64(name);
	newid = alloca(newid_len);
	newid->rec_len = newid_len;
	newid->id = start;
	strcpy(newid->name, name);
	fprintf(stderr, "newid:{name=%s, start=%llu}\n", __FUNCTION__, newid->rec_len, newid->id);

	newid_pos = lseek(file_des, tail_pos, SEEK_SET);
	if (write(file_des, newid, newid_len)<0) {
		goto fail;
	}
	newtailid_pos = lseek(file_des, 0, SEEK_CUR);
	if (write(file_des, tailid, tailid_len)<0) {
		goto fail;
	}
	p = mremap(file_map, file_size, file_size+newid_len, MREMAP_FIXED);
	if (p==MAP_FAILED) {
		goto fail;
	}
	file_size += newid_len;
	tail_pos = newtailid_pos;
	return (void*)(file_map + newid_pos);
fail:
	return NULL;
}

void id_file_sync(void)
{
	msync(file_map, file_size, MS_ASYNC);
}

