#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "bootsector.h"

/*** private variables ***/
char buf[SECTOR_SIZE];

fat32BS* initializeBootSector(int deviceFP)
{
	read(deviceFP, buf, SECTOR_SIZE);
	fat32BS *bootSector = (fat32BS*) malloc(sizeof(fat32BS));
	memcpy(bootSector, buf, sizeof(*bootSector));
	return bootSector;
}