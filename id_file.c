#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <alloca.h>
#include <glob.h>

#include <util_log.h>

#include "mod_config.h"
#include "id_file_format.h"
#include "id_file.h"

static int roundup_64(int n)
{
	return n%8 ? n/8*8+8 : n;
}

static int calc_filesize_align64(const char *name)
{
	return roundup_64(sizeof(struct idfile_header_st)+strlen(name));
}

char *id_file_create(const char *idname, uint64_t start)
{
    struct idfile_header_st *hdr;
    int hdr_size;
    int ret;
    int fd;
    char fname_template[64], id_fname[64];

	//fprintf(stderr, "Try to create id file for %s\n", idname);
    do {
    	snprintf(fname_template, 64, "%s/.%s.id_XXXXXX", id_module_config.id_config_dir, idname);
        mktemp(fname_template);
        fd = open(fname_template, O_RDWR|O_CREAT|O_EXCL, 0600);
        if (fd<0) {
            if (errno==EEXIST) {
                continue;
            }
			mylog(L_DEBUG, "open(%s) failed: %m", fname_template);
            return NULL;
        }
    } while(fd<0);
	//fprintf(stderr, "id file %s created\n", fname_template);
    hdr_size = calc_filesize_align64(idname);
	//fprintf(stderr, "%s size will be %d\n", fname_template, hdr_size);
    hdr = alloca(hdr_size);
    memset(hdr, 0, hdr_size);
    memcpy(&hdr->magic, NATIVE_ENDIAN_MAGIC, 4);
    hdr->value = start;
    strcpy(hdr->name, idname);

    ret = write(fd, hdr, hdr_size);
    if (ret<hdr_size) {
        goto drop_fail;
    }
    lseek(fd, 0, SEEK_SET);
	//fprintf(stderr, "%s was written.\n", fname_template);

    snprintf(id_fname, 64, "%s/%s.id", id_module_config.id_config_dir, idname);
    if (link(fname_template, id_fname)<0) {
        goto drop_fail;
    }
	close(fd);
    unlink(fname_template);
	//fprintf(stderr, "%s was renamed to %s.\n", fname_template, id_fname);
    return strdup(id_fname);

drop_fail:
    close(fd);
    unlink(fname_template);
    return NULL;
}


static int analysis_id_fd(int fd)
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

	savepos = lseek(fd, 0, SEEK_CUR);

	lseek(fd, 0, SEEK_SET);
	ret = read(fd, &hdr, sizeof(hdr));
	if (ret<sizeof(hdr)) {
		mylog(L_ERR, "id file seems too small.");
		goto fail;
	}

	if (memcmp(&hdr.magic, NATIVE_ENDIAN_MAGIC, 4)!=0) {
		mylog(L_ERR, "Can't understand this id file.");
		goto fail;
	}

	if (hdr.clean_mark != FILE_CLEAN) {
		mylog(L_ERR, "Id file seems not clean.");
		goto fail;
	}
	lseek(fd, savepos, SEEK_SET);
	return 0;
fail:
	lseek(fd, savepos, SEEK_SET);
	return -1;
}

static void id_file_unload(void)
{
//	int i;
//
//	for (i=0; i<nr_mapped_id_file; ++i) {
//		msync(mapped_id_file[i].map_addr, mapped_id_file[i].map_size, MS_SYNC);
//		munmap(mapped_id_file[i].map_addr, mapped_id_file[i].map_size);
//	}
}

int id_file_map(struct mapped_id_file_st *res, const char *fname)
{
	int fd;
	struct stat stat_res;

	if (lstat(fname, &stat_res)) {
		mylog(L_ERR, "fstat() failed: %m");
		return -1;
	}
	fd = open(fname, O_RDWR);
	if (fd<0) {
		mylog(L_INFO, "open(%s) failed: %m.", fname);
		return -1;
	}
	if (analysis_id_fd(fd)!=0) {
		mylog(L_ERR, "Check id file(%s) failed.", fname);
		close(fd);
		return -1;
	}
	res->map_addr = mmap(NULL, stat_res.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if (res->map_addr==MAP_FAILED) {
		mylog(L_ERR, "Map id file(%s) failed.", fname);
		close(fd);
		return -1;
	}
	res->hdr = res->map_addr;
	close(fd);
	return 0;
}

int id_files_load(const char *path, struct mapped_id_file_st **arr, int *nr)
{
	int fd;
	glob_t glob_res;
	int ret, count;
	char pat[1024];
	int i, pos;

	snprintf(pat, 1024, "%s/%s", id_module_config.id_config_dir, CONFIG_FILE_PATTERN);

	ret = glob(pat, 0, NULL, &glob_res);
	if (ret==GLOB_NOMATCH) {
		mylog(L_INFO, "glob(%s) failed: No id_files found.", pat);
		*arr = NULL;
		*nr = 0;
		return 0;
	} else if (ret==GLOB_ABORTED) {
		mylog(L_ERR, "glob(%s) failed: I/O error.", pat);
		return -1;
	} else if (ret==GLOB_NOSPACE) {
		mylog(L_ERR, "glob(%s) failed: Out of mempry.", pat);
		return -1;
	}

	if (*arr==NULL) {
		*arr = malloc(sizeof(struct mapped_id_file_st)*glob_res.gl_pathc);
		count = glob_res.gl_pathc;
	} else {
		if (*nr < glob_res.gl_pathc) {
			mylog(L_ERR, "%s: array is too small, use %d results only.", __FUNCTION__, *nr);
			count = *nr;
		} else {
			count = glob_res.gl_pathc;
		}
	}
	pos = 0;
	for (i=0; i<count; ++i) {
		if (id_file_map((*arr)+pos, glob_res.gl_pathv[i])==0) {
			mylog(L_DEBUG, "id_file: %s was mapped.", glob_res.gl_pathv[i]);
			pos++;
		} else {
			mylog(L_INFO, "id_file: %s mapping failed.", glob_res.gl_pathv[i]);
		}
	}
	*nr = pos;
	mylog(L_INFO, "%d id_files loaded.", pos);
	globfree(&glob_res);
	return 0;
}

