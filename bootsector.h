/*
 * FILE     : bootsector.h
 * REMARKS  : header file of bootsector.h
 */

#ifndef BOOTSECTOR_H
#define BOOTSECTOR_H

#include <inttypes.h>

/*         boot sector constants         */
/* Data structure provided by assignment */
#define BS_OEMName_LENGTH 8     // length of BS_OEMName
#define BS_VolLab_LENGTH 11     // length of BS_VolLab
#define BS_FilSysType_LENGTH 8  // length BS_FilSysType_LENGTH

#pragma pack(push)
#pragma pack(1)
struct fat32BS_struct {
    char BS_jmpBoot[3];
    char BS_OEMName[BS_OEMName_LENGTH];
    uint16_t BPB_BytesPerSec;
    uint8_t BPB_SecPerClus;
    uint16_t BPB_RsvdSecCnt;
    uint8_t BPB_NumFATs;
    uint16_t BPB_RootEntCnt;
    uint16_t BPB_TotSec16;
    uint8_t BPB_Media;
    uint16_t BPB_FATSz16;
    uint16_t BPB_SecPerTrk;
    uint16_t BPB_NumHeads;
    uint32_t BPB_HiddSec;
    uint32_t BPB_TotSec32;
    uint32_t BPB_FATSz32;
    uint16_t BPB_ExtFlags;
    uint8_t BPB_FSVerLow;
    uint8_t BPB_FSVerHigh;
    uint32_t BPB_RootClus;
    uint16_t BPB_FSInfo;
    uint16_t BPB_BkBootSec;
    char BPB_reserved[12];
    uint8_t BS_DrvNum;
    uint8_t BS_Reserved1;
    uint8_t BS_BootSig;
    uint32_t BS_VolID;
    char BS_VolLab[BS_VolLab_LENGTH];
    char BS_FilSysType[BS_FilSysType_LENGTH];
    char BS_CodeReserved[420];
    uint8_t BS_SigA;
    uint8_t BS_SigB;
};
#pragma pack(pop)

typedef struct fat32BS_struct fat32BS;

/*** public functions ***/

// initialize the boot sector of the device
fat32BS* initializeBootSector(uint32_t deviceFP);

// get mirrored fat value
uint32_t getMirrFatVal(fat32BS *bootSector);

// signature validation of the boot sector 
int isBootSectorValid(fat32BS *bootSector);

// get mefia type (removable/fixed)
char* getMediaType(fat32BS *bootSector);

// get drive type (HD/floppy)
char* getDriveType(fat32BS *bootSector);

// is fat mirrored?
char* getMirrFatMsg(fat32BS *bootSector);

// get storage size in bytes
uint64_t getSizeInBytes(fat32BS *bootSector);

#endif