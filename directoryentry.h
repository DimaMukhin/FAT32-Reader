#ifndef DIRECTORYENTRY_H
#define DIRECTORYENTRY_H

#include <inttypes.h>

/* Directory Entry constants */
#define DIR_Name_LENGTH 11

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
int getDirNameSize(fat32DE *dir);

#endif