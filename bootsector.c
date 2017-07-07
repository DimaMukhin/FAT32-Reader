#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "bootsector.h"

#define BOOTSECTOR_SIZE 512
#define FAT32SIG 0x29
#define FIXED_MEDIA_TYPE 0xF8
#define DRV_NUM_HD 0x80
#define MIRR_FAT_MASK 0x0080

/*** private variables ***/
char buf[BOOTSECTOR_SIZE];

fat32BS* initializeBootSector(uint32_t deviceFP)
{
	read(deviceFP, buf, BOOTSECTOR_SIZE);
	fat32BS *bootSector = (fat32BS*) malloc(sizeof(fat32BS));
	memcpy(bootSector, buf, sizeof(*bootSector));
	return bootSector;
}

char* getMediaType(fat32BS *bootSector)
{
	if (bootSector->BPB_Media == (uint8_t)FIXED_MEDIA_TYPE)
		return "fixed";
	else
		return "removable";
}

char* getDriveType(fat32BS *bootSector)
{
	if (bootSector->BS_DrvNum == (uint8_t)DRV_NUM_HD)
		return "HD";
	else
		return "floppy";
}

uint32_t getMirrFatVal(fat32BS *bootSector)
{
	return ((uint32_t) (bootSector->BPB_ExtFlags & MIRR_FAT_MASK));
}

int isBootSectorValid(fat32BS *bootSector)
{
	if (bootSector->BS_BootSig == (uint8_t)FAT32SIG)
		return 1;
	return 0;
}

char* getMirrFatMsg(fat32BS *bootSector)
{
	if (getMirrFatVal(bootSector) == 0)
		return "yes";
	else
		return "no";
}


uint64_t getSizeInBytes(fat32BS *bootSector)
{
	return ((uint64_t) bootSector->BPB_TotSec32) * bootSector->BPB_BytesPerSec;
}