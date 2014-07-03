#ifndef ID_FILE_FORMAT_H
#define ID_FILE_FORMAT_H

#include <stdint.h>

#define	DEFAULT_ID_NAME	"Default"

struct id_entry_st {	// DON'T CHANGE THE ORDER OF MEMBERS! For keep the id aligned to 64bit border.
	uint64_t id;
	uint32_t rec_len;
	uint8_t name[1];
};

struct idfile_header_st {
	uint8_t magic[4];	// Always be "LEID" for Little-Endian machines, and "BEID" for Big-Endian machines.
	uint32_t	id0_offset;
	uint32_t	sync_mark;
};

#endif

