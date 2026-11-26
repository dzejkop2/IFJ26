/**
 * @name ifj26
 * @authors Marek Rzeszoto (xrzeszm00)
 * @file error.c
 */
#include "error.h"
#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Helper function that prints error code and message to stderr and exits
 * it
 *  *
 * @param errorNum error code that is printed and used as exit code of the
 * program
 * @param errorMsg error message that is printed, if NULL only error code is
 * printed
 */
void error(int errorNum, const char *errorMsg) {
	if (errorMsg != NULL) {
		fprintf(stderr, "ERROR: %d\nms: %s\n", errorNum, errorMsg);
	} else {
		fprintf(stderr, "ERROR: %d\n", errorNum);
	}
	exit(errorNum);
}
