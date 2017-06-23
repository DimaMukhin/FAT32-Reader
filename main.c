#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "fat32.h"

#define SECTOR_SIZE 512

/*** private functions ***/
void showInfo(fat32BS *bootSector);

/*** private global variables ***/

int main(int argc, char* argv[])
{
	char buf[SECTOR_SIZE];
	int deviceFP;
	fat32BS *bootSector;
	
	// checking ussage
	if (argc < 2)
	{
		printf("Ussage: fat32reader /dev/somedevice\nExiting program\n");
		exit(EXIT_FAILURE);	
	}
	
	deviceFP = open(argv[1], O_RDONLY);
	read(deviceFP, buf, SECTOR_SIZE);
	bootSector = (fat32BS*) buf;
	
	showInfo(bootSector);
	
	
	return 0;
}

/*-----------------------------------------------------------------------------------------showInfo
 * Show the info of a boot sector
 */
void showInfo(fat32BS *bootSector)
{
	printf("---- Device Info ----\n");
	printf("OEM Name: %.*s\n", BS_OEMName_LENGTH, bootSector->BS_OEMName);
	printf("Label: %.*s\n", BS_VolLab_LENGTH, bootSector->BS_VolLab);
	printf("File System Type: %.*s\n", BS_FilSysType_LENGTH, bootSector->BS_FilSysType);
	printf("Media Type: %#X (TODO: implement fixed/removable)\n", bootSector->BPB_Media);
	printf("Size: TODO: Finish implementing\n");
	printf("Drive Number: %#X (TODO: implement floppy/hard disk logic)\n", bootSector->BS_DrvNum);
	
	printf("\n--- Geometry ---\n");
	printf("Bytes per Sector: %d\n", bootSector->BPB_BytesPerSec);
	printf("Sectors per Cluster: %d\n", bootSector->BPB_SecPerClus);
	printf("Total Sectors: %d\n", bootSector->BPB_TotSec32);
	
	printf("\n--- FS Info ---\n");
	printf("Volume ID: ");
}// showInfo