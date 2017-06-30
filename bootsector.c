#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "bootsector.h"

#define BOOTSECTOR_SIZE 512
#define FAT32SIG 0x29

/*** private variables ***/
char buf[BOOTSECTOR_SIZE];

fat32BS* initializeBootSector(int deviceFP)
{
	read(deviceFP, buf, BOOTSECTOR_SIZE);
	fat32BS *bootSector = (fat32BS*) malloc(sizeof(fat32BS));
	memcpy(bootSector, buf, sizeof(*bootSector));
	return bootSector;
}

char* getMediaType(fat32BS *bootSector)
{
	if (bootSector->BPB_Media == (uint8_t)0xF8)
		return "fixed";
	else
		return "removable";
}

char* getDriveType(fat32BS *bootSector)
{
	if (bootSector->BS_DrvNum == (uint8_t)0x80)
		return "HD";
	else
		return "floppy";
}

int getMirrFatVal(fat32BS *bootSector)
{
	return bootSector->BPB_ExtFlags & 0x0080;
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