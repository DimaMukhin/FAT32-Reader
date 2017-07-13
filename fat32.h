/*
 * FILE     : fat32.h
 * REMARKS  : header file of fat32.c
 */

#ifndef FAT32_H
#define FAT32_H

#include <inttypes.h>

#include "bootsector.h"
#include "directoryentry.h"
#include "fsinfo.h"

#define BYTE_SIZE 8

/* Fat32 Datastructure
 * Used for storing Fat32 file system information
 */
typedef struct fat32_struct
{
    uint32_t deviceFP;
    uint32_t cluster2;
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

// create FAT32 structure for a device
fat32* createFat32(uint32_t deviceFP);

// read a cluster from fat device into a buffer
void readCluster(fat32 *fat32Obj, uint64_t clusterNum, char buf[]);

// read file allocation table entry n, return its entry
uint32_t readFAT(fat32 *fat32Obj, uint32_t n);

// read a sector from fat device into a buffer
void readSector(fat32 *fat32Obj, uint64_t sectorNum, char buf[]);

// make sure device was loaded correctly
int checkFat32(fat32 *fat32Obj);

// get amount of free bytes from device
uint64_t getFreeBytes(fat32 *fat32Obj);

// free all allocated memory
void freeFat32(fat32 *fat32Obj);

#endif
