#ifndef FAT32_H
#define FAT32_H

#include <inttypes.h>

#include "bootsector.h"
#include "directoryentry.h"
#include "fsinfo.h"

#define BYTE_SIZE 8

typedef struct fat32_struct
{
	int deviceFP;
	int cluster2;
	uint32_t sectorSize;
	uint32_t clusterSize;
	uint32_t curDirCluster;
	char *dirClusterBuf;
	char *fatSectorBuf;
	char *fileClusterBuf;
	fat32BS *bootSector;
	fat32DE *directoryEntry;
	fat32FI *fsinfo;
} fat32;

/*** public functions ***/
fat32* createFat32(int deviceFP);

void readCluster(fat32 *fat32Obj, int clusterNum, char buf[]);

uint32_t readFAT(fat32 *fat32Obj, int n);

void readSector(fat32 *fat32Obj, int sectorNum, char buf[]);

void checkFat32(fat32 *fat32Obj);

#endif
