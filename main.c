#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "fat32.h"

#define BYTE_SIZE 8
#define SECTOR_SIZE 512
#define CLUSTER_SIZE 4096

#define ATTR_DIRECTORY 0x10
#define ATTR_VOLUME_ID 0x08
#define FREE_ENTRY 0xE5
#define END_ENTRY 0x00
#define END_OF_CLUSTER 0x0FFFFFF8

/*** private functions ***/
void showInfo();
void showDir();
void changeDirectory(char dirName[]);
void getFile(char fileName[]);
int dirNameEquals(char dirName[], char cdName[]);
int fileNameEquals(char fileName[], char getName[]);
void readCluster(int clusterNum);
void readSector(int sectorNum);
uint32_t readFAT(int n);
void readFile(int startCluster, int fileFD);


/*** private global variables ***/
fat32BS *bootSector;
fat32DE *directoryEntry;
char buf[SECTOR_SIZE];
char bufCluster[CLUSTER_SIZE];
int deviceFP;
int cluster2;

int main(int argc, char* argv[])
{
	// checking ussage
	if (argc < 2)
	{
		printf("Ussage: fat32reader /dev/somedevice\nExiting program\n");
		exit(EXIT_FAILURE);	
	}
	
	deviceFP = open(argv[1], O_RDONLY);
	read(deviceFP, buf, SECTOR_SIZE);
	bootSector = (fat32BS*) malloc(sizeof(fat32BS));
	memcpy(bootSector, buf, sizeof(*bootSector));
	
	showInfo(bootSector);
	
	cluster2 = bootSector->BPB_RsvdSecCnt + (bootSector->BPB_NumFATs * bootSector->BPB_FATSz32);
	readCluster(2);
	directoryEntry = (fat32DE*) bufCluster;
	
	printf("Dir name: %s\n", directoryEntry->DIR_Name);
	
	showDir(directoryEntry);
	
	changeDirectory("LOLCATS");
	
	showDir(directoryEntry);
	
	changeDirectory("..");
	
	showDir(directoryEntry);
	
	printf("\n\nplease work!!!  %d\n\n", readFAT(6));
	
	getFile("1.JPG");
	
	return 0;
}

/*-----------------------------------------------------------------------------------------showInfo
 * Show the info of a boot sector
 */
void showInfo()
{
	printf("\n---- Device Info ----\n");
	printf("OEM Name: %.*s\n", BS_OEMName_LENGTH, bootSector->BS_OEMName);
	printf("Label: %.*s\n", BS_VolLab_LENGTH, bootSector->BS_VolLab);
	printf("File System Type: %.*s\n", BS_FilSysType_LENGTH, bootSector->BS_FilSysType);
	printf("Media Type: %#X (TODO: implement fixed/removable)\n", bootSector->BPB_Media);
	printf("Size: %d (Sectors, TODO: change)\n", bootSector->BPB_TotSec32);
	printf("Drive Number: %#X (TODO: implement floppy/hard disk logic)\n", bootSector->BS_DrvNum);
	
	printf("\n--- Geometry ---\n");
	printf("Bytes per Sector: %d\n", bootSector->BPB_BytesPerSec);
	printf("Sectors per Cluster: %d\n", bootSector->BPB_SecPerClus);
	printf("Total Sectors: %d\n", bootSector->BPB_TotSec32);
	
	printf("\n--- FS Info ---\n");
	printf("Volume ID: %.*s\n", BS_VolLab_LENGTH, bootSector->BS_VolLab);
	printf("Version: %d:%d\n", bootSector->BPB_FSVerHigh, bootSector->BPB_FSVerLow);
	printf("Reserved Sectors: %d\n", bootSector->BPB_RsvdSecCnt);
	printf("Number of FATs: %d\n", bootSector->BPB_NumFATs);
	printf("FAT Size: %d sectors\n", bootSector->BPB_FATSz32);
	printf("Mirrored FAT: %d (finish implementing logic)\n", bootSector->BPB_ExtFlags & 0x0080);
}// showInfo

/*------------------------------------------------------------------------------------------showDir
 * show dirrectory information
 */
void showDir()
{
	fat32DE *currDir = directoryEntry;
	printf("\nDIRECTORY LISTING\n");
	
	while (currDir->DIR_Name[0] != (char)END_ENTRY)
	{
		if (currDir->DIR_Name[0] != (char)FREE_ENTRY && currDir->DIR_FileSize != -1)
		{
			if (currDir->DIR_Attr == (char)ATTR_VOLUME_ID)
				printf("VOL_ID: %s\n\n", currDir->DIR_Name);
			else if (currDir->DIR_Attr == (char)ATTR_DIRECTORY)
				printf("<%s>\t%d\n", currDir->DIR_Name, currDir->DIR_FileSize);
			else
				printf("%s\t%d\n", currDir->DIR_Name, currDir->DIR_FileSize);
		}
		currDir++;
	}
	
}// showDir

/*----------------------------------------------------------------------------------changeDirectory
 * change directory to specified directory
 */
void changeDirectory(char dirName[])
{
	fat32DE *currDir = directoryEntry;
	
	while (currDir->DIR_Name[0] != (char)END_ENTRY)
	{
		if (currDir->DIR_Name[0] != (char)FREE_ENTRY && currDir->DIR_Attr == (char)ATTR_DIRECTORY)
		{
			if (dirNameEquals(currDir->DIR_Name, dirName))
			{
				readCluster(currDir->DIR_FstClusLO);
				directoryEntry = (fat32DE*) bufCluster;
			}
		}
		currDir++;
	}
}// changeDirectory

/*------------------------------------------------------------------------------------------getFile
 * 
 */
void getFile(char fileName[])
{
	fat32DE *currDir = directoryEntry;
	
	while (currDir->DIR_Name[0] != (char)END_ENTRY)
	{
		if (currDir->DIR_Name[0] != (char)FREE_ENTRY && currDir->DIR_FileSize != -1)
		{
			if (fileNameEquals(currDir->DIR_Name, fileName))
			{
				int fileFD = open(fileName, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
				readFile(currDir->DIR_FstClusLO, fileFD);
				close(fileFD);
			}
		}
		currDir++;
	}
}// getFile

/*------------------------------------------------------------------------------------dirNameEquals
 * compares directory name to user entered name
 */
int dirNameEquals(char dirName[], char cdName[])
{
	if (strlen(cdName) > 8)
		return 0;
	
	for (int i = 0; i < strlen(cdName); i++)
	{
		if (dirName[i] != cdName[i])
			return 0;
	}
	
	return 1;
}// dirNameEquals

/*-----------------------------------------------------------------------------------fileNameEquals
 * 
 */
int fileNameEquals(char fileName[], char getName[])
{
	int getNamePtr = 0;
	int fileNamePtr = 0;
	
	while (getNamePtr < strlen(getName) && fileNamePtr < strlen(fileName))
	{
		if (getName[getNamePtr] == '.')
			getNamePtr++;
		else if (fileName[fileNamePtr] == (char)0x20)
			fileNamePtr++;
		else if (getName[getNamePtr] != fileName[fileNamePtr])
			return 0;
		else
		{
			getNamePtr++;
			fileNamePtr++;
		}
	}
	
	return 1;
}// fileNameEquals

/*--------------------------------------------------------------------------------------readCluster
 * 
 */
void readCluster(int clusterNum)
{
	if (clusterNum == 0)
		clusterNum = 2;
	
	lseek(deviceFP, cluster2 * SECTOR_SIZE + ((clusterNum - 2) * 8 * SECTOR_SIZE), SEEK_SET);
	read(deviceFP, bufCluster, CLUSTER_SIZE);
}// readCluster

/*---------------------------------------------------------------------------------------readSector
 * 
 */
void readSector(int sectorNum)
{
	lseek(deviceFP, sectorNum * SECTOR_SIZE, SEEK_SET);
	read(deviceFP, buf, SECTOR_SIZE);
}// readSector

/*------------------------------------------------------------------------------------------readFAT
 * NOTE!!!! see calculations? remember tip from class? this is the main bug
 */
uint32_t readFAT(int n)
{
	uint32_t fatEntry = -1;
	
	int fatOffset = n * 4;
	int thisFATSecNum = bootSector->BPB_RsvdSecCnt + (fatOffset / bootSector->BPB_BytesPerSec);
	readSector(thisFATSecNum);	
	int thisFATEntOffset = fatOffset % bootSector->BPB_BytesPerSec;
	fatEntry = (uint32_t) buf[thisFATEntOffset];
	fatEntry = fatEntry & 0x0FFFFFFF; // ignoring first 4 bits
	
	return fatEntry;
}// readFAT

/*-----------------------------------------------------------------------------------------readFile
 * 
 */
void readFile(int startCluster, int fileFD)
{
	int currCluster = startCluster;
	
	while (currCluster < (uint32_t)END_OF_CLUSTER && currCluster != 0)
	{
		readCluster(currCluster);
		//printf("%.*s", CLUSTER_SIZE, bufCluster);
		write(fileFD, bufCluster, CLUSTER_SIZE);
		currCluster = readFAT(currCluster);
	}
}// readFile





