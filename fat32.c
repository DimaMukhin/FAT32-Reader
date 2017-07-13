/*
 * FILE     : fat32.c
 * REMARKS  : everything fat32
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "fat32.h"
#include "bootsector.h"
#include "errorcheck.h"
    
#define BYTES_PER_32BIT 4           // used for FAT entry calculation (bytes per 32 bit)
#define BYTE_OFFSET 8               // byte offset
#define DBYTE_OFFSET 16             // double byte offset (used for constructing fat entry)
#define TBYTE_OFFSET 24             // triple byte offset
#define FAT_MASK 0x0FFFFFFF         // fat mask (according to white pages)
#define FAT12_CLUSTERS 4085         // max fat12 clusters (according to white pages)
#define FAT16_CLUSTERS 65525        // max fat16 clusters (according to white pages)
#define FAT0_VALID_MASK 0x0FFFFF00  // fat entry 0 signature validation mask
#define ClnShutBitMask 0x08000000   // clean/dirty bit of fat entry 2
#define HrdErrBitMask 0x04000000    // read/write err bit of fat entry 2
#define EOC 0x0FFFFFFF              // End of clusterchain

/*** private function declerations ***/
int isFat32Valid(fat32 *fat32Obj);
int isFileAllocationTableValid(fat32 *fat32Obj);

/*** private global variables ***/
int retVal; // return value for error checking

/*** public functions region ***/

/*--------------------------------------------------------------------------------------createFat32
 * create a fat32 object to handle a device
 * stores all important information like bootsector, FSinfo, and more
 * can be used for multiple different devices at the same time
 */
fat32* createFat32(uint32_t deviceFP)
{
    fat32 *fat32Obj = (fat32*) malloc(sizeof(fat32));
    fat32Obj->deviceFP = deviceFP;
    
    // initializing the bootsector
    fat32Obj->bootSector = initializeBootSector(deviceFP);
    fat32BS *bootSector = fat32Obj->bootSector;
    
    // calculating important sizes and locations
    fat32Obj->sectorSize = bootSector->BPB_BytesPerSec;
    fat32Obj->clusterSize = bootSector->BPB_SecPerClus * fat32Obj->sectorSize;
    fat32Obj->curDirCluster = bootSector->BPB_RootClus;
    fat32Obj->cluster2 = bootSector->BPB_RsvdSecCnt + 
        (bootSector->BPB_NumFATs * bootSector->BPB_FATSz32);
    
    // mallocing buffers for safety (dont want something to override memory region while reading)
    fat32Obj->dirClusterBuf = (char*) malloc(fat32Obj->clusterSize);
    fat32Obj->fatSectorBuf = (char*) malloc(fat32Obj->sectorSize);
    fat32Obj->fileClusterBuf = (char*) malloc(fat32Obj->clusterSize);
    
    // reading current directory cluster (root) and assigning current directory structure
    readCluster(fat32Obj, fat32Obj->curDirCluster, fat32Obj->dirClusterBuf);
    fat32Obj->directoryEntry = (fat32DE*) fat32Obj->dirClusterBuf;
    
    // getting FSinfo
    char tempSectorBuf[fat32Obj->sectorSize];
    readSector(fat32Obj, bootSector->BPB_FSInfo, tempSectorBuf);
    fat32Obj->fsinfo = (fat32FI*) malloc(sizeof(fat32FI));
    memcpy(fat32Obj->fsinfo, tempSectorBuf, fat32Obj->sectorSize);
    
    return fat32Obj;
}// createFat32

/*--------------------------------------------------------------------------------------readCluster
 * Read a cluster from fat32, assign data to "buf"
 */
void readCluster(fat32 *fat32Obj, uint64_t clusterNum, char buf[])
{
    // if cluster is 0, read root instead.
    if (clusterNum == 0)
        clusterNum = fat32Obj->bootSector->BPB_RootClus;
    
    uint64_t cluster2 = fat32Obj->cluster2;
    uint64_t cluster2InBytes = cluster2 * fat32Obj->sectorSize;
    uint64_t offsetInBytes = (clusterNum - 2) * fat32Obj->clusterSize;
    off_t totalOffset = cluster2InBytes + offsetInBytes;
    
    lseek(fat32Obj->deviceFP,  totalOffset, SEEK_SET);
    checkForError(totalOffset, ERR_LSEEK_MSG);
    
    retVal = read(fat32Obj->deviceFP, buf, fat32Obj->clusterSize);
    checkForError(retVal, ERR_READ_MSG);
}// readCluster

/*---------------------------------------------------------------------------------------readSector
 * Read a sector from fat32, assign data to "buf"
 */
void readSector(fat32 *fat32Obj, uint64_t sectorNum, char buf[])
{
    off_t seekOffset = ((off_t) sectorNum * fat32Obj->sectorSize);
    lseek(fat32Obj->deviceFP, seekOffset, SEEK_SET);
    checkForError(seekOffset, ERR_LSEEK_MSG);
    retVal = read(fat32Obj->deviceFP, buf, fat32Obj->sectorSize);
    checkForError(retVal, ERR_READ_MSG);
}// readSector

/*------------------------------------------------------------------------------------------readFAT
 * Read fat entry of a cluster
 * n: the cluster number
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
 * Validate fat32 device was loaded correctly and that it is infact a fat32 device
 */
int checkFat32(fat32 *fat32Obj)
{
    // validate bootsector
    if (!isBootSectorValid(fat32Obj->bootSector))
    {
        printf("BootSector was not loaded correctly\n");
        return 0;
    }
    
    // validate fat is 32
    if (!isFat32Valid(fat32Obj))
        return 0;
    
    // validate the file allocation table
    if (!isFileAllocationTableValid(fat32Obj))
        return 0;
    
    // validate FS info
    if (!isFSinfoValid(fat32Obj->fsinfo))
    {
        printf("file system info was not loaded correctly\n");
        return 0;
    }
    
    return 1;
}// checkFat32

/*-------------------------------------------------------------------------------------getFreeBytes
 * calculate and return the number of free bytes
 */
uint64_t getFreeBytes(fat32 *fat32Obj)
{
    return fat32Obj->fsinfo->FSI_Free_Count * ((uint64_t) fat32Obj->clusterSize);
}// getFreeBytes

/*----------------------------------------------------------------------------------------freeFat32
 * free all allocated memory
 */
void freeFat32(fat32 *fat32Obj)
{
    free(fat32Obj->dirClusterBuf);
    free(fat32Obj->fatSectorBuf);
    free(fat32Obj->fileClusterBuf);
    free(fat32Obj->bootSector);
    free(fat32Obj->fsinfo);
    free(fat32Obj);
}// freeFat32

/*** private functions region ***/

/*-------------------------------------------------------------------------------------isFat32Valid
 * Make sure the device loaded is fat32, exit otherwise.
 */
int isFat32Valid(fat32 *fat32Obj)
{
    fat32BS* bootSector = fat32Obj->bootSector;
    
    uint32_t RootDirSectors = ((((uint32_t) bootSector->BPB_RootEntCnt) * BYTES_PER_DIR_ENT) + 
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
        printf("Error, volume is FAT12\n");
        return 0;
    } 
    else if(CountofClusters < FAT16_CLUSTERS) 
    {
        printf("Error, volume is FAT16\n");
        return 0;
    } 
    
    return 1;
}// isFat32Valid

/*-----------------------------------------------------------------------isFileAllocationTableValid
 * validate the first file allocation table of the device.
 * checks the first two entries of the file allocation table according
 * to the white pages. (see page 18)
 */
int isFileAllocationTableValid(fat32 *fat32Obj)
{
    uint32_t fat0signature = ((uint32_t)fat32Obj->bootSector->BPB_Media) | FAT0_VALID_MASK;
    if (readFAT(fat32Obj, 0) != fat0signature)
    {
        printf("Filed to validate fat entry 0\n");
        return 0;
    }
        
    // fat entry 1 with maksked out dirty bit and read/write error bit (according to white pages)
    uint32_t fat1masked = readFAT(fat32Obj, 1) | ClnShutBitMask | HrdErrBitMask;
    
    uint32_t fat1signature = EOC;
    if (fat1masked != fat1signature)
    {
        printf("Filed to validate fat entry 1\n");
        return 0;
    }
        
    return 1;
}// isFileAllocationTableValid
