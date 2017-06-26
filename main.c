#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "fat32.h"
#include "bootsector.h"
#include "directoryentry.h"

#define ATTR_DIRECTORY 0x10
#define ATTR_VOLUME_ID 0x08
#define FREE_ENTRY 0xE5
#define END_ENTRY 0x00
#define END_OF_CLUSTER 0x0FFFFFF8
#define MAX_INPUT_SIZE 50

/*** private functions ***/
void showInfo();
void showDir();
void changeDirectory(char dirName[]);
void getFile(char fileName[]);
int dirNameEquals(char dirName[], char cdName[]);
int fileNameEquals(char fileName[], char getName[]);
void populateFile(int startCluster, int fileFD);
void getInputFromUser(char dest[]);
void processInput(char input[]);


/*** private global variables ***/
fat32 *fat32Obj;

int main(int argc, char* argv[])
{
	// checking ussage
	if (argc < 2)
	{
		printf("Ussage: fat32reader /dev/somedevice\nExiting program\n");
		exit(EXIT_FAILURE);	
	}	
	int deviceFP = open(argv[1], O_RDONLY); // TODO: dont forget to close
	fat32Obj = createFat32(deviceFP);
	
	char input[MAX_INPUT_SIZE];
	while (1)
	{
		getInputFromUser(input);
		processInput(input);
	}
	
	showInfo();
	
	showDir();
	
	changeDirectory("LOLCATS");
	
	showDir();
	
	changeDirectory("..");
	
	showDir();
	
	printf("\n\nplease work!!!  %d\n\n", readFAT(fat32Obj, 6));
	
	getFile("1.JPG");
	
	return 0;
}

/*---------------------------------------------------------------------------------getInputFromUser
 * 
 */
void getInputFromUser(char dest[])
{
	memset(dest, 0, MAX_INPUT_SIZE);
	write(STDOUT_FILENO, "> ", 2);
	read(STDIN_FILENO, dest, MAX_INPUT_SIZE);
	if (strlen(dest) > 0)
		dest[strlen(dest) - 1] = '\0';
}// getInputFromUser

/*-------------------------------------------------------------------------------------processInput
 * 
 */
void processInput(char input[])
{
	char delim[2] = " ";
	char *token;
	
	token = strtok(input, delim);
	if (token == NULL || strlen(token) <= 0)
		return;
	
	if (strcmp(token, "info") == 0)
		showInfo();
	else if (strcmp(token, "dir") == 0)
		showDir();
	else if (strcmp(token, "cd") == 0)
	{
		token = strtok(NULL, delim);
		changeDirectory(token);
	}
	else if (strcmp(token, "get") == 0)
	{
		token = strtok(NULL, delim);
		getFile(token);
	}
}// processInput

/*-----------------------------------------------------------------------------------------showInfo
 * Show the info of a boot sector
 */
void showInfo()
{
	fat32BS *bootSector = fat32Obj->bootSector;
	
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
	fat32DE *currDir = fat32Obj->directoryEntry;
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
	fat32DE *currDir = fat32Obj->directoryEntry;
	
	while (currDir->DIR_Name[0] != (char)END_ENTRY)
	{
		if (currDir->DIR_Name[0] != (char)FREE_ENTRY && currDir->DIR_Attr == (char)ATTR_DIRECTORY)
		{
			if (dirNameEquals(currDir->DIR_Name, dirName))
			{
				readCluster(fat32Obj, currDir->DIR_FstClusLO, fat32Obj->dirClusterBuf);
				fat32Obj->directoryEntry = (fat32DE*) fat32Obj->dirClusterBuf;
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
	fat32DE *currDir = fat32Obj->directoryEntry;
	
	while (currDir->DIR_Name[0] != (char)END_ENTRY)
	{
		if (currDir->DIR_Name[0] != (char)FREE_ENTRY && currDir->DIR_FileSize != -1)
		{
			if (fileNameEquals(currDir->DIR_Name, fileName))
			{
				int fileFD = open(fileName, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
				populateFile(currDir->DIR_FstClusLO, fileFD);
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
	
	if (strlen(cdName) < 8 && dirName[strlen(cdName)] != (char)0x20)
		return 0;
	
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

/*-----------------------------------------------------------------------------------------populateFile
 * 
 */
void populateFile(int startCluster, int fileFD)
{
	int currCluster = startCluster;
	
	while (currCluster < (uint32_t)END_OF_CLUSTER && currCluster != 0)
	{
		readCluster(fat32Obj, currCluster, fat32Obj->fileClusterBuf);
		write(fileFD, fat32Obj->fileClusterBuf, CLUSTER_SIZE);
		currCluster = readFAT(fat32Obj, currCluster);
	}
}// populateFile





