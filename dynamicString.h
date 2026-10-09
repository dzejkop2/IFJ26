/**
 * @name ifj26
 * @authors Marek Rzeszoto (xrzeszm00)
 * @file dynamicString.h
 */

#ifndef DYNAMICSTRING_H
#define DYNAMICSTRING_H

#include "stddef.h"

typedef struct {
    char *memPointer;
    size_t currLenght;
    size_t capacity;
} DynamicString;

void  dynStr_ctor(DynamicString *dynStr);
void  dynStr_load(DynamicString *dynStr, char input);   
void  dynStr_freeBuff(DynamicString *dynStr);       
char *dynStr_output(const DynamicString *dynStr);   
void  dynStr_dtor(DynamicString *dynStr);

#endif
