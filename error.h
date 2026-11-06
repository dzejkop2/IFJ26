/**
 * @name ifj26
 * @authors Marek Rzeszoto (xrzeszm00)
 * @file error.h
 */

#ifndef ERROR_H
#define ERROR_H

#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Identifies diffrent types of errors in project
 */
typedef enum {
    LEXICAL_ERROR = 1,
    SYNTAX_ERROR = 2,
    SEMANTIC_UNDEFINITION_ERROR = 3,
    SEMANTIC_REDEFINITION_ERROR = 4,
    SEMANTIC_ARGUMENT_COUNT_ERROR = 5,
    SEMANTIC_TYPE_ERROR = 6,
    SEMANTIC_UNSUPPORTED_MODIFICATON_ERROR = 7,
    SEMANTIC_ERROR = 10,
    INTERNAL_ERROR = 99
} ErrorCode;

/**
 * @brief Handles error messages distribution
 * @param errorNum defines number of the error
 * @param errorMsg defines additional message to the error 
 */
void error(int errorNum, const char *errorMsg);

#endif
