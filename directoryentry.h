/*
 * FILE     : directoryentry.h
 * REMARKS  : header file of directoryentry.c
 */

#ifndef DIRECTORYENTRY_H
#define DIRECTORYENTRY_H

#include <inttypes.h>

/* Directory Entry constants */
#define DIR_Name_LENGTH 11      // length of DIR_Name
#define DIR_LENGTH 8            // max length of file name (not including extension)
#define DIR_PAD 0x20            // padding byte of DIR_Name
#define BYTES_PER_DIR_ENT 32    // directory entry structyre total bytes

#define ATTR_DIRECTORY 0x10     // attribute directory signature
#define ATTR_VOLUME_ID 0x08     // volume id directory signature
#define ATTR_LONG_NAME 0x0F     // long name directory signature
#define FREE_ENTRY 0xE5         // free entry directory signature
#define END_ENTRY 0x00          // end/last entry directory signature

#pragma pack(push)
#pragma pack(1)
struct fat32DE_struct {
    char DIR_Name[DIR_Name_LENGTH];
    uint8_t DIR_Attr;
    uint8_t DIR_NTRes;
    uint8_t DIR_CrtTimeTenth;
    uint16_t DIR_CrtTime;
    uint16_t DIR_CrtDate;
    uint16_t DIR_LstAccDate;
    uint16_t DIR_FstClusHI;
    uint16_t DIR_WrtTime;
    uint16_t DIR_WrtDate;
    uint16_t DIR_FstClusLO;
    uint32_t DIR_FileSize;
};
#pragma pack(pop)

typedef struct fat32DE_struct fat32DE;

/*** public functions ***/

// get the size of the name of the directory entry
uint32_t getDirNameSize(fat32DE *dir);

#endif