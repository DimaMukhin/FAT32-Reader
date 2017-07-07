/*
 * NAME                     : Dima Mukhin
 * STUDENT NUMBER           : 7773184
 * COURSE                   : COMP 3430
 * INSTRUCTOR               : Dr. Jim Young
 * ASSIGNMENT               : 3
 * Question                 : 1
 *
 */

/*
 * FILE     : main.c
 * REMARKS  : Contains main function for running the fat32 reader
 *			  Ussage: fat32reader /dev/somedevice
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

#include "fat32.h"
#include "bootsector.h"
#include "directoryentry.h"
#include "bootsector.h"

#define END_OF_CLUSTER 0x0FFFFFF8	// end of cluster signature
#define MAX_INPUT_SIZE 50			// maximum input to process
#define BYTETOMEG 1000000			// using this instead of 1048576 (same as in the example)
#define MEGTOBYTE 1000				// converting MB to GB
#define END_OF_READ 0				// EOF value of write() according to man page
#define CLUSTER_HI_OFFSET 16		// offset of cluster high

/*** private functions ***/
void showInfo();										// show FAT32 info
void showDir();											// show current directory
void changeDirectory(char dirName[]);					// change directory
void getFile(char fileName[]);							// get file by name
int dirNameEquals(char dirName[], char cdName[]);		// check if two dir names are equal
int fileNameEquals(char fileName[], char getName[]);	// check if two file names are equal
uint32_t getInputFromUser(char dest[]);					// get input from user, max 50 chars
void processInput(char input[]);						// process user input
void toUpperString(char *input);						// convert input to upper case

// populate a file from the device to local machine
void populateFile(uint32_t startCluster, uint32_t fileFD, uint32_t fileSize);


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
	
	uint32_t deviceFP = open(argv[1], O_RDONLY);
	
	if (deviceFP == -1)
	{
		printf("could not open device correctly\nExiting program\n");
		exit(EXIT_FAILURE);	
	}
	
	fat32Obj = createFat32(deviceFP);
	
	checkFat32(fat32Obj);
	
	char input[MAX_INPUT_SIZE];
	while (getInputFromUser(input) != END_OF_READ)
	{
		processInput(input);
	}
	
	close(deviceFP);
	printf("Exiting program...\n");
	return 0;
}

/*---------------------------------------------------------------------------------getInputFromUser
 * 
 */
uint32_t getInputFromUser(char dest[])
{
	uint32_t readVal;
	
	memset(dest, 0, MAX_INPUT_SIZE);
	write(STDOUT_FILENO, "> ", 2);
	readVal = read(STDIN_FILENO, dest, MAX_INPUT_SIZE);
	if (strlen(dest) > 0)
		dest[strlen(dest) - 1] = '\0';
	
	return readVal;
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
		if (token != NULL)
		{
			toUpperString(token);
			changeDirectory(token);
		}
	}
	else if (strcmp(token, "get") == 0)
	{
		token = strtok(NULL, delim);
		if (token != NULL)
		{
			toUpperString(token);
			getFile(token);
		}
	}
	else
	{
		printf("\nCommand not found\n");
	}
}// processInput

/*-----------------------------------------------------------------------------------------showInfo
 * Show the info of a boot sector
 */
void showInfo()
{
	fat32BS *bootSector = fat32Obj->bootSector;
	uint64_t sizeInBytes = getSizeInBytes(bootSector);
	uint64_t sizeInMB = sizeInBytes / BYTETOMEG;
	double sizeInGB = ((double) sizeInMB) / MEGTOBYTE;
	
	printf("\n---- Device Info ----\n");
	printf("OEM Name: %.*s\n", BS_OEMName_LENGTH, bootSector->BS_OEMName);
	printf("Label: %.*s\n", BS_VolLab_LENGTH, bootSector->BS_VolLab);
	printf("File System Type: %.*s\n", BS_FilSysType_LENGTH, bootSector->BS_FilSysType);
	printf("Media Type: %#X (%s)\n", bootSector->BPB_Media, getMediaType(bootSector));
	printf("Size: %lu (%luMB, %.2fGB)\n", sizeInBytes, sizeInMB, sizeInGB);
	printf("Drive Number: %d (%s)\n", bootSector->BS_DrvNum, getDriveType(bootSector));
	
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
	printf("Mirrored FAT: %d (%s)\n", getMirrFatVal(bootSector), getMirrFatMsg(bootSector));
}// showInfo

/*------------------------------------------------------------------------------------------showDir
 * show dirrectory information
 */
void showDir()
{
	fat32DE *currDir = fat32Obj->directoryEntry;
	char clusterBuffer[fat32Obj->clusterSize];
	uint32_t curEntry = 0;
	uint32_t curClust = fat32Obj->curDirCluster;
	uint32_t dirsPerClust = fat32Obj->clusterSize / sizeof(fat32DE);
	
	printf("\nDIRECTORY LISTING\n");
	printf("VOL_ID: %.*s\n\n", BS_VolLab_LENGTH, fat32Obj->bootSector->BS_VolLab);
	
	while (currDir->DIR_Name[0] != (char)END_ENTRY)
	{
		if (currDir->DIR_Name[0] != (char)FREE_ENTRY && currDir->DIR_FileSize != -1)
		{
			if (currDir->DIR_Attr == (uint8_t)ATTR_DIRECTORY)
				printf("<%.*s>\t%d\n", getDirNameSize(currDir), 
				currDir->DIR_Name, currDir->DIR_FileSize);
			else if (currDir->DIR_Attr != (uint8_t)ATTR_LONG_NAME &&
				currDir->DIR_Attr != (uint8_t)ATTR_VOLUME_ID)
				printf("%.*s\t%d\n", DIR_Name_LENGTH, currDir->DIR_Name, currDir->DIR_FileSize);
		}
		curEntry++;
		currDir++;
		if (currDir->DIR_Name[0] == (char)END_ENTRY || curEntry >= dirsPerClust)
		{
			curClust = readFAT(fat32Obj, curClust);
			if (curClust < (uint32_t)END_OF_CLUSTER && curClust != 0)
			{
				curEntry = 0;
				readCluster(fat32Obj, curClust, clusterBuffer);
				currDir = (fat32DE*) clusterBuffer;
			}
		}
	}
	
	printf("--Bytes Free: %lu\n", getFreeBytes(fat32Obj));
	printf("--DONE\n");
}// showDir

/*----------------------------------------------------------------------------------changeDirectory
 * change directory to specified directory
 */
void changeDirectory(char dirName[])
{
	fat32DE *currDir = fat32Obj->directoryEntry;
	char clusterBuffer[fat32Obj->clusterSize];
	uint32_t curClust = fat32Obj->curDirCluster;
	uint32_t curEntry = 0;
	uint32_t dirsPerClust = fat32Obj->clusterSize / sizeof(currDir);
	
	while (currDir->DIR_Name[0] != (char)END_ENTRY)
	{
		if (currDir->DIR_Name[0] != (char)FREE_ENTRY && currDir->DIR_Attr == (char)ATTR_DIRECTORY)
		{
			if (dirNameEquals(currDir->DIR_Name, dirName))
			{
				uint32_t clusterNum = currDir->DIR_FstClusLO +
				(currDir->DIR_FstClusHI << CLUSTER_HI_OFFSET);
				readCluster(fat32Obj, clusterNum, fat32Obj->dirClusterBuf);
				if (clusterNum == 0)
					fat32Obj->curDirCluster = fat32Obj->bootSector->BPB_RootClus;
				else
					fat32Obj->curDirCluster = clusterNum;
				fat32Obj->directoryEntry = (fat32DE*) fat32Obj->dirClusterBuf;
				return;
			}
		}
		curEntry++;
		currDir++;
		if (currDir->DIR_Name[0] == (char)END_ENTRY || curEntry >= dirsPerClust)
		{
			curClust = readFAT(fat32Obj, curClust);
			if (curClust < (uint32_t)END_OF_CLUSTER && curClust != 0)
			{
				curEntry = 0;
				readCluster(fat32Obj, curClust, clusterBuffer);
				currDir = (fat32DE*) clusterBuffer;
			}
		}
	}
	
	printf("\nError: folder not found\n");
}// changeDirectory

/*------------------------------------------------------------------------------------------getFile
 * 
 */
void getFile(char fileName[])
{
	fat32DE *currDir = fat32Obj->directoryEntry;
	char clusterBuffer[fat32Obj->clusterSize];
	uint32_t curClust = fat32Obj->curDirCluster;
	uint32_t curEntry = 0;
	uint32_t dirsPerClust = fat32Obj->clusterSize / sizeof(currDir);
	
	while (currDir->DIR_Name[0] != (char)END_ENTRY)
	{
		if (currDir->DIR_Name[0] != (char)FREE_ENTRY && currDir->DIR_FileSize != -1)
		{
			if (fileNameEquals(currDir->DIR_Name, fileName))
			{
				uint32_t clusterNum = currDir->DIR_FstClusLO + 
				(currDir->DIR_FstClusHI << CLUSTER_HI_OFFSET);
				int fileFD = open(fileName, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
				populateFile(clusterNum, fileFD, currDir->DIR_FileSize);
				close(fileFD);
				return;
			}
		}
		curEntry++;
		currDir++;
		if (currDir->DIR_Name[0] == (char)END_ENTRY || curEntry >= dirsPerClust)
		{
			curClust = readFAT(fat32Obj, curClust);
			if (curClust < (uint32_t)END_OF_CLUSTER && curClust != 0)
			{
				curEntry = 0;
				readCluster(fat32Obj, curClust, clusterBuffer);
				currDir = (fat32DE*) clusterBuffer;
			}
		}
	}
	
	printf("\nError: file not found\n");
}// getFile

/*------------------------------------------------------------------------------------dirNameEquals
 * compares directory name to user entered name
 */
int dirNameEquals(char dirName[], char cdName[])
{
	if (strlen(cdName) > DIR_LENGTH)
		return 0;
	
	for (int i = 0; i < strlen(cdName); i++)
	{
		if (dirName[i] != cdName[i])
			return 0;
	}
	
	if (strlen(cdName) < DIR_LENGTH && dirName[strlen(cdName)] != (char)DIR_PAD)
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
	
	while (getNamePtr < strlen(getName) || fileNamePtr < strlen(fileName))
	{
		if (getName[getNamePtr] == '.')
			getNamePtr++;
		else if (fileName[fileNamePtr] == (char)DIR_PAD)
			fileNamePtr++;
		else if (getName[getNamePtr] != fileName[fileNamePtr])
			return 0;
		else
		{
			if (getNamePtr < strlen(getName))
				getNamePtr++;
			if (fileNamePtr < strlen(fileName))
				fileNamePtr++;
		}
	}
	
	return 1;
}// fileNameEquals

/*-----------------------------------------------------------------------------------------populateFile
 * 
 */
void populateFile(uint32_t startCluster, uint32_t fileFD, uint32_t fileSize)
{
	uint32_t currCluster = startCluster;
	uint32_t nextCluster;
	uint32_t lastClusterSize = fileSize % fat32Obj->clusterSize;
	
	printf("\nGetting file, please wait...\n");
	
	while (currCluster < (uint32_t)END_OF_CLUSTER && currCluster != 0)
	{
		readCluster(fat32Obj, currCluster, fat32Obj->fileClusterBuf);
		
		nextCluster = readFAT(fat32Obj, currCluster);
		if (nextCluster >= (uint32_t)END_OF_CLUSTER)
			write(fileFD, fat32Obj->fileClusterBuf, lastClusterSize);
		else
			write(fileFD, fat32Obj->fileClusterBuf, fat32Obj->clusterSize);
		
		currCluster = nextCluster;
	}
	
	printf("\nDone.\n");
}// populateFile

void toUpperString(char *input)
{
	for (int i = 0; i < strlen(input); i++)
		input[i] = toupper(input[i]);
}





