#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "fat32.h"
#include "bootsector.h"

/*** private function declerations ***/
void validateFatIs32(fat32 *fat32Obj);

/*** public functions region ***/

/*--------------------------------------------------------------------------------------createFat32
 * 
 */
fat32* createFat32(int deviceFP)
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
void readCluster(fat32 *fat32Obj, int clusterNum, char buf[])
{
	if (clusterNum == 0)
		clusterNum = 2;
	
	int cluster2 = fat32Obj->cluster2;
	lseek(fat32Obj->deviceFP, cluster2 * fat32Obj->sectorSize + 
		((clusterNum - 2) * 8 * fat32Obj->sectorSize), SEEK_SET);
	read(fat32Obj->deviceFP, buf, fat32Obj->clusterSize);
}// readCluster

/*---------------------------------------------------------------------------------------readSector
 * 
 */
void readSector(fat32 *fat32Obj, int sectorNum, char buf[])
{
	lseek(fat32Obj->deviceFP, sectorNum * fat32Obj->sectorSize, SEEK_SET);
	read(fat32Obj->deviceFP, buf, fat32Obj->sectorSize);
}// readSector

/*------------------------------------------------------------------------------------------readFAT
 * NOTE!!!! see calculations? remember tip from class? this is the main bug
 */
uint32_t readFAT(fat32 *fat32Obj, int n)
{
	uint32_t fatEntry = -1;
	
	fat32BS *bootSector = fat32Obj->bootSector;
	int fatOffset = n * 4;
	int thisFATSecNum = bootSector->BPB_RsvdSecCnt + (fatOffset / bootSector->BPB_BytesPerSec);
	readSector(fat32Obj, thisFATSecNum, fat32Obj->fatSectorBuf);	
	int thisFATEntOffset = fatOffset % bootSector->BPB_BytesPerSec;
	fatEntry = (uint32_t) fat32Obj->fatSectorBuf[thisFATEntOffset];
	fatEntry = fatEntry & 0x0FFFFFFF; // ignoring first 4 bits
	
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
	
	if(CountofClusters < 4085) 
	{
		printf("volume is FAT12\nExiting program\n");
		exit(EXIT_FAILURE);
	} 
	else if(CountofClusters < 65525) 
	{
		printf("volume is FAT16\nExiting program\n");
		exit(EXIT_FAILURE);
	} 
}// validateFatIs32
