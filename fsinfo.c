#include <inttypes.h>

#include "fsinfo.h"

#define SIG_VALID 0x41615252

int isFSinfoValid(fat32FI *fsinfo)
{
	if (fsinfo->FSI_LeadSig == (uint32_t)SIG_VALID)
		return 1;
	return 0;
}