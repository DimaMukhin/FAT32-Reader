#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "fat32.h"
#include "bootsector.h"

#define BYTES_PER_32BIT 4
#define BYTE_OFFSET 8
#define DBYTE_OFFSET 16
#define TBYTE_OFFSET 24
#define FAT_MASK 0x0FFFFFFF
#define FAT12_CLUSTERS 4085
#define FAT16_CLUSTERS 65525

/*** private function declerations ***/
void validateFatIs32(fat32 *fat32Obj);

/*** public functions region ***/

/*--------------------------------------------------------------------------------------createFat32
 * 
 */
fat32* createFat32(uint32_t deviceFP)
{
	fat32 *fat32Obj = (fat32*) malloc(sizeof(fat32));
	fat32Obj->deviceFP = deviceFP;
	fat32Obj->bootSector = initializeBootSector(deviceFP);
	
	fat32BS *bootSector = fat32Obj->bootSector;
	
	fat32Obj->sectorSize = bootSector->BPB_BytesPerSec;
	fat32Obj->clusterSize = bootSector->BPB_SecPerClus * fat32Obj->sectorSize;
	fat32Obj->curDirCluster = bootSector->BPB_RootClus;
	fat32Obj->cluster2 = bootSector->BPB_RsvdSecCnt + 
		(bootSector->BPB_NumFATs * bootSector->BPB_FATSz32);
	
	fat32Obj->dirClusterBuf = (char*) malloc(fat32Obj->clusterSize);
	fat32Obj->fatSectorBuf = (char*) malloc(fat32Obj->sectorSize);
	fat32Obj->fileClusterBuf = (char*) malloc(fat32Obj->clusterSize);
	
	readCluster(fat32Obj, fat32Obj->curDirCluster, fat32Obj->dirClusterBuf);
	fat32Obj->directoryEntry = (fat32DE*) fat32Obj->dirClusterBuf;
	
	char tempSectorBuf[fat32Obj->sectorSize];
	readSector(fat32Obj, bootSector->BPB_FSInfo, tempSectorBuf);
	fat32Obj->fsinfo = (fat32FI*) malloc(sizeof(fat32FI));
	memcpy(fat32Obj->fsinfo, tempSectorBuf, fat32Obj->sectorSize);
	
	return fat32Obj;
}// createFat32

/*--------------------------------------------------------------------------------------readCluster
 * 
 */
void readCluster(fat32 *fat32Obj, uint64_t clusterNum, char buf[])
{
	if (clusterNum == 0)
		clusterNum = fat32Obj->bootSector->BPB_RootClus;
	
	uint64_t cluster2 = fat32Obj->cluster2;
	uint64_t cluster2InBytes = cluster2 * fat32Obj->sectorSize;
	uint64_t offsetInBytes = (clusterNum - 2) * fat32Obj->clusterSize;
	uint64_t totalOffset = cluster2InBytes + offsetInBytes;
	lseek(fat32Obj->deviceFP,  totalOffset, SEEK_SET);
	read(fat32Obj->deviceFP, buf, fat32Obj->clusterSize);
}// readCluster

/*---------------------------------------------------------------------------------------readSector
 * 
 */
void readSector(fat32 *fat32Obj, uint64_t sectorNum, char buf[])
{
	lseek(fat32Obj->deviceFP, sectorNum * fat32Obj->sectorSize, SEEK_SET);
	read(fat32Obj->deviceFP, buf, fat32Obj->sectorSize);
}// readSector

/*------------------------------------------------------------------------------------------readFAT
 * NOTE!!!! see calculations? remember tip from class? this is the main bug
 */
uint32_t readFAT(fat32 *fat32Obj, uint32_t n)
{
	uint32_t fatEntry = -1;
	
	fat32BS *bootSector = fat32Obj->bootSector;
	uint64_t fatOffset = n * BYTES_PER_32BIT;
	uint64_t thisFATSecNum = bootSector->BPB_RsvdSecCnt + (fatOffset / bootSector->BPB_BytesPerSec);
	readSector(fat32Obj, thisFATSecNum, fat32Obj->fatSectorBuf);	
	uint64_t thisFATEntOffset = fatOffset % bootSector->BPB_BytesPerSec;
	fatEntry = (uint8_t) fat32Obj->fatSectorBuf[thisFATEntOffset] +
		((uint8_t) fat32Obj->fatSectorBuf[thisFATEntOffset + 1] << BYTE_OFFSET) +
		((uint8_t) fat32Obj->fatSectorBuf[thisFATEntOffset + 2] << DBYTE_OFFSET) +
		((uint8_t) fat32Obj->fatSectorBuf[thisFATEntOffset + 3] << TBYTE_OFFSET);
	fatEntry = fatEntry & FAT_MASK; // ignoring first 4 bits
	
	return fatEntry;
}// readFAT

/*---------------------------------------------------------------------------------------checkFat32
 * 
 */
void checkFat32(fat32 *fat32Obj)
{
	if (!isBootSectorValid(fat32Obj->bootSector))
	{
		printf("BootSector was not loaded correctly\nExiting program\n");
		exit(EXIT_FAILURE);
	}
	
	validateFatIs32(fat32Obj);
	
	if (!isFSinfoValid(fat32Obj->fsinfo))
	{
		printf("file system info was not loaded correctly\nExiting program\n");
		exit(EXIT_FAILURE);
	}
}// checkFat32

/*** private functions region ***/

/*----------------------------------------------------------------------------------validateFatIs32
 * 
 */
void validateFatIs32(fat32 *fat32Obj)
{
	fat32BS* bootSector = fat32Obj->bootSector;
	
	uint32_t RootDirSectors = ((((uint32_t) bootSector->BPB_RootEntCnt) * 32) + 
		(bootSector->BPB_BytesPerSec - 1));
		
	uint32_t FATSz;
	if(bootSector->BPB_FATSz16 != 0)
		FATSz = bootSector->BPB_FATSz16;
	else
		FATSz = bootSector->BPB_FATSz32;
	
	uint32_t TotSec;
	if(bootSector->BPB_TotSec16 != 0)
		TotSec = bootSector->BPB_TotSec16;
	else
		TotSec = bootSector->BPB_TotSec32;
	
	uint32_t DataSec = TotSec - (bootSector->BPB_RsvdSecCnt + 
		(((uint32_t)bootSector->BPB_NumFATs) * FATSz) + RootDirSectors);
	
	uint32_t CountofClusters = DataSec / ((uint32_t) bootSector->BPB_SecPerClus);
	
	if(CountofClusters < FAT12_CLUSTERS) 
	{
		printf("volume is FAT12\nExiting program\n");
		exit(EXIT_FAILURE);
	} 
	else if(CountofClusters < FAT16_CLUSTERS) 
	{
		printf("volume is FAT16\nExiting program\n");
		exit(EXIT_FAILURE);
	} 
}// validateFatIs32

uint64_t getFreeBytes(fat32 *fat32Obj)
{
	return fat32Obj->fsinfo->FSI_Free_Count * ((uint64_t) fat32Obj->clusterSize);
}
