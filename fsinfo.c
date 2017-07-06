#include <inttypes.h>

#include "fsinfo.h"

#define LEAD_SIG_VALID 0x41615252
#define TRAIL_SIG_VALID 0xAA550000

int isFSinfoValid(fat32FI *fsinfo)
{
	if (fsinfo->FSI_LeadSig == (uint32_t)LEAD_SIG_VALID &&
		fsinfo->FSI_TrailSig == (uint32_t)TRAIL_SIG_VALID)
		return 1;
	return 0;
}