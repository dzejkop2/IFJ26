/**
 * @name ifj26
 * @authors Marek Rzeszoto (xrzeszm00)
 * @file error.h
 */
#include "error.h"
#include <stdio.h>
#include <stdlib.h>
 
void Error(int errorNum, const char* errorMsg) {
    if (errorMsg != NULL) {
        fprintf(stderr, "ERROR: %d\nms: %s\n", errorNum, errorMsg);
    } else {
        fprintf(stderr, "ERROR: %d\n", errorNum);
    }
    exit(errorNum);

    
    // fprintf(stderr, "Error %d: %s\n", errorNum, errorMsg);
    // exit(errorNum);

}
