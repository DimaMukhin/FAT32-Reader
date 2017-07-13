/*
 * FILE     : bootsector.c
 * REMARKS  : boot sector data structure implementation
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "bootsector.h"
#include "errorcheck.h"

#define BOOTSECTOR_SIZE 512     // boot sector size, not sector size
#define FAT32SIG 0x29           // fat32 bootsector signature
#define FIXED_MEDIA_TYPE 0xF8   // fixed media type signature bytes
#define DRV_NUM_HD 0x80         // HD type signature bytes
#define MIRR_FAT_MASK 0x0080    // mirrored FAT mask

/*** private variables ***/
char buf[BOOTSECTOR_SIZE];
int retVal;

/*** public functions ***/

/*-----------------------------------------------------------------------------initializeBootSector
 * Initialize the boot sector of the device and return it
 */
fat32BS* initializeBootSector(uint32_t deviceFP)
{
    retVal = read(deviceFP, buf, BOOTSECTOR_SIZE);
    checkForError(retVal, ERR_READ_MSG);
    fat32BS *bootSector = (fat32BS*) malloc(sizeof(fat32BS));
    memcpy(bootSector, buf, sizeof(*bootSector));
    return bootSector;
}// initializeBootSector

/*-------------------------------------------------------------------------------------getMediaType
 * get mefia type (removable/fixed)
 */
char* getMediaType(fat32BS *bootSector)
{
    if (bootSector->BPB_Media == (uint8_t)FIXED_MEDIA_TYPE)
        return "fixed";
    else
        return "removable";
}// getMediaType

/*-------------------------------------------------------------------------------------getDriveType
 * get drive type (HD/floppy)
 */
char* getDriveType(fat32BS *bootSector)
{
    if (bootSector->BS_DrvNum == (uint8_t)DRV_NUM_HD)
        return "HD";
    else
        return "floppy";
}// getDriveType

/*-------------------------------------------------------------------------------------getDriveType
 * get mirrored fat value
 */
uint32_t getMirrFatVal(fat32BS *bootSector)
{
    return ((uint32_t) (bootSector->BPB_ExtFlags & MIRR_FAT_MASK));
}// getDriveType

/*--------------------------------------------------------------------------------isBootSectorValid
 * signature validation of the boot sector
 */
int isBootSectorValid(fat32BS *bootSector)
{
    if (bootSector->BS_BootSig == (uint8_t)FAT32SIG)
        return 1;
    return 0;
}// isBootSectorValid

/*------------------------------------------------------------------------------------getMirrFatMsg
 * is fat mirrored?
 */
char* getMirrFatMsg(fat32BS *bootSector)
{
    if (getMirrFatVal(bootSector) == 0)
        return "yes";
    else
        return "no";
}// getMirrFatMsg

/*-----------------------------------------------------------------------------------getSizeInBytes
 * get storage size in bytes
 */
uint64_t getSizeInBytes(fat32BS *bootSector)
{
    return ((uint64_t) bootSector->BPB_TotSec32) * bootSector->BPB_BytesPerSec;
}// getSizeInBytes