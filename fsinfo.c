/*
 * FILE     : fsinfo.c
 * REMARKS  : file system info data structure implementation
 */

#include <inttypes.h>

#include "fsinfo.h"

#define LEAD_SIG_VALID 0x41615252
#define TRAIL_SIG_VALID 0xAA550000

/*------------------------------------------------------------------------------------isFSinfoValid
 * validation of the file system info structure
 */
int isFSinfoValid(fat32FI *fsinfo)
{
    if (fsinfo->FSI_LeadSig == (uint32_t)LEAD_SIG_VALID &&
        fsinfo->FSI_TrailSig == (uint32_t)TRAIL_SIG_VALID)
        return 1;
    return 0;
}// isFSinfoValid