#include "directoryentry.h"
#include "fat32.h"
#include "bootsector.h"

#define MAX_DIR_NAME_SIZE 8
#define NAME_PADDING 0x20

uint32_t getDirNameSize(fat32DE *dir)
{
	uint32_t size = 0;
	
	for (int i = 0; i < MAX_DIR_NAME_SIZE; i++)
		if (dir->DIR_Name[i] != (char)NAME_PADDING)
			size++;
	
	return size;
}