#include <stdio.h>

#include "id_file_format.h"
#include "id_mem_format.h"

struct id_mem_entry_st *id_array=NULL
int id_array_size=0;

static int create_default_id_file(const char *fname)
{
	struct idfile_header_st hdr;
	struct id_entry_st *id0, idtail;
	int rec_len, ret;
	FILE *fp;

	fp = fopen(fname, "wb");
	if (fp==NULL) {
		return -1;
	}

	memcpy(&hdr.magic, "LEID", 4);
	hdr.id0_offset = sizeof(struct idfile_header_st);
	hdr.sync_mark = 0;

	rec_len=sizeof(sizeof(struct id_entry_st)+strlen(DEFAULT_ID_NAME));
	id0 = alloca(rec_len);
	id0->id=1;
	strcpy(id0->name, DEFAULT_ID_NAME);

	idtail.rec_len = 0;
	idtail.id=0;
	idtail.name[0]='\0';

	ret = fwrite(&hdr, 1, sizeof(hdr), fp);
	if (ret<sizeof(hdr)) {
		goto drop_fail;
	}
	ret = fwrite(id0, 1, id0->rec_len, fp);
	if (ret<id0->rec_len) {
		goto drop_fail;
	}
	ret = fwrite(&idtail, 1, sizeof(idtail), fp);
	if (ret<sizeof(idtail)) {
		goto drop_fail;
	}
	fclose(fp);
	return 0;

drop_fail:
	fclose(fp);
	return -1;
}

static int map_id_file(int fd);

static int check_id_filed(int fd);

static int open_or_create(const char *fname)
{
	int fd, ret;

	fd = open(fname, O_RDWR);
	if (fd<0 && errno==ENOENT) {
		fd = create_default_id_file(fname);
		if (ret<0) {
			mylog(L_ERR, "Can't create new id file: %m");
			return -2;
		}
		ret = map_id_file(fd);
		close(fd);
		return ret;
	} else if (fd<0) {
		mylog(L_ERR, "Can't open id file(%s): %m", fname);
		return -2;
	} else {
		check_id_file(fd);
		map_id_file(fd);
		close(fd);
		return 0;
	}
}

int id_file_load(const char *fname)
{
	return open_or_create(fname);
}

void id_file_sync(void)
{
}

