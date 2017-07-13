/*
 * FILE     : errorcheck.c
 * REMARKS  : public error checking file for code reusability across different files
 */

#include <stdio.h>
#include <stdlib.h>
 
#include "errorcheck.h"

/*------------------------------------------------------------------------------------checkForError
 * global error checking function for code reusability across different files
 */
void checkForError(int errVal, char *msg)
{
    if (errVal == -1)
    {
        perror(msg);
        exit(EXIT_FAILURE);
    }
}// checkForError