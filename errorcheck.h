/*
 * FILE     : errorcheck.h
 * REMARKS  : public error checking file for code reusability across different files
 */

#ifndef ERRORCHECK_H
#define ERRORCHECK_H

/*** common error messages ***/

#define ERR_WRITE_MSG "write() failed"
#define ERR_READ_MSG "read() failed"
#define ERR_LSEEK_MSG "lseek() failed"

/*** public functions ***/

// check return value. print message and exit on error
void checkForError(int errVal, char *msg);

#endif