/**
 * @name ifj26
 * @authors Marek Rzeszoto (xrzeszm00)
 * @file dynamicString.c
 */


#include <stdio.h>
#include <stdlib.h>
#include "error.h"
#include "malloc.h" 
#include "dynamicString.h"
#include <string.h>


#define MALLOC_MEM 16

void dynStr_ctor(dynamicString *dynStr){
    dynStr->memPointer = malloc(sizeof(char)*MALLOC_MEM);
    if(dynStr->memPointer == NULL){
        error(INTERNAL_ERROR, "DynStr buffer malloc memory failed");
    }
    dynStr->currLenght = 0;
    dynStr->capacity = MALLOC_MEM;
}

void  dynStr_load(dynamicString *dynStr, char input){
    if(dynStr->currLenght+1 >=dynStr->capacity){
        size_t reallocHelperCapacity = dynStr->capacity * 2;
        char *reallocHelper = realloc(dynStr->memPointer, reallocHelperCapacity);
        if (reallocHelper==NULL){
            error(INTERNAL_ERROR, "DynStr help buffer realloc memory failed");
        }
        dynStr->memPointer = reallocHelper;
        dynStr->capacity = reallocHelperCapacity;
    }
    dynStr->memPointer[dynStr->currLenght] = input;
    dynStr->currLenght++;
}

void  dynStr_freeBuff(dynamicString *dynStr){    
    dynStr->currLenght = 0;
}

char *dynStr_output(const dynamicString *dynStr){
    char *mallocHelper = malloc(sizeof(char)*(dynStr->currLenght+1));
    if (mallocHelper==NULL){
        error(INTERNAL_ERROR, "DynStr help buffer malloc memory failed");
    }
    memcpy(mallocHelper, dynStr->memPointer, dynStr->currLenght);
    char helpVar = '\0';
    memcpy(mallocHelper + dynStr->currLenght, &helpVar, 1);
    return mallocHelper;
}

void  dynStr_dtor(dynamicString *dynStr){
    dynStr->currLenght = 0;
    dynStr->capacity = 0;
    free(dynStr->memPointer);
    dynStr->memPointer = NULL;
}
