#ifndef DIRECTORYENTRY_H
#define DIRECTORYENTRY_H

#include <inttypes.h>

/* Directory Entry constants */
#define DIR_Name_LENGTH 11
#define DIR_LENGTH 8
#define DIR_PAD 0x20

#define ATTR_DIRECTORY 0x10
#define ATTR_VOLUME_ID 0x08
#define ATTR_LONG_NAME 0x0F
#define FREE_ENTRY 0xE5
#define END_ENTRY 0x00

#pragma pack(push)
#pragma pack(1)
struct fat32DE_struct {
	char DIR_Name[DIR_Name_LENGTH];
	uint8_t DIR_Attr;
	uint8_t DIR_NTRes;
	uint8_t DIR_CrtTimeTenth;
	uint16_t DIR_CrtTime;
	uint16_t DIR_CrtDate;
	uint16_t DIR_LstAccDate;
	uint16_t DIR_FstClusHI;
	uint16_t DIR_WrtTime;
	uint16_t DIR_WrtDate;
	uint16_t DIR_FstClusLO;
	uint32_t DIR_FileSize;
};
#pragma pack(pop)

typedef struct fat32DE_struct fat32DE;

/*** public functions ***/
uint32_t getDirNameSize(fat32DE *dir);

#endif