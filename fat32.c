#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "fat32.h"
#include "bootsector.h"

/*--------------------------------------------------------------------------------------createFat32
 * 
 */
fat32* createFat32(int deviceFP)
{
	fat32 *fat32Obj = (fat32*) malloc(sizeof(fat32));
	fat32Obj->deviceFP = deviceFP;
	fat32Obj->bootSector = initializeBootSector(deviceFP);
	
	fat32BS *bootSector = fat32Obj->bootSector;
	fat32Obj->cluster2 = bootSector->BPB_RsvdSecCnt + 
		(bootSector->BPB_NumFATs * bootSector->BPB_FATSz32);
	
	fat32Obj->dirClusterBuf = (char*) malloc(CLUSTER_SIZE);
	fat32Obj->fatSectorBuf = (char*) malloc(SECTOR_SIZE);
	fat32Obj->fileClusterBuf = (char*) malloc(CLUSTER_SIZE);
	
	readCluster(fat32Obj, 2, fat32Obj->dirClusterBuf);
	fat32Obj->directoryEntry = (fat32DE*) fat32Obj->dirClusterBuf;
	
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
	lseek(fat32Obj->deviceFP, cluster2 * SECTOR_SIZE + ((clusterNum - 2) * 8 * SECTOR_SIZE), SEEK_SET);
	read(fat32Obj->deviceFP, buf, CLUSTER_SIZE);
}// readCluster

/*---------------------------------------------------------------------------------------readSector
 * 
 */
void readSector(fat32 *fat32Obj, int sectorNum, char buf[])
{
	lseek(fat32Obj->deviceFP, sectorNum * SECTOR_SIZE, SEEK_SET);
	read(fat32Obj->deviceFP, buf, SECTOR_SIZE);
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

