#ifndef ID_FILE_FORMAT_H
#define ID_FILE_FORMAT_H

#include <stdint.h>
#include <time.h>

#define	FILE_CLEAN	0x00000000UL
#define	FILE_DIRTY	0x00000001UL

#ifdef __ORDER_LITTLE_ENDIAN__
#define NATIVE_ENDIAN_MAGIC	"LEID"
#else
#define NATIVE_ENDIAN_MAGIC	"BEID"
#endif

struct idfile_header_st {
	uint8_t magic[4];	// Always be "LEID" for Little-Endian machines, and "BEID" for Big-Endian machines.
	uint32_t	clean_mark;
	uint64_t	value;
	struct timeval last_fetch_tv;
	uint8_t name[1];
};

#endif

