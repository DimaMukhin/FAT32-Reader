#ifndef FSINFO_H
#define FSINFO_H

#include <inttypes.h>

/* FSinfo constants */
#define RESERVED1_SIZE 480
#define RESERVED2_SIZE 12

#pragma pack(push)
#pragma pack(1)
struct fat32FI_struct {
	uint32_t FSI_LeadSig;
	char FSI_Reserved1[RESERVED1_SIZE];
	uint32_t FSI_StrucSig;
	uint32_t FSI_Free_Count;
	uint32_t FSI_Nxt_Free;
	char FSI_Reserved2[RESERVED2_SIZE];
	uint32_t FSI_TrailSig;
};
#pragma pack(pop)

typedef struct fat32FI_struct fat32FI;

/*** public functions ***/
int isFSinfoValid(fat32FI *fsinfo);

#endif