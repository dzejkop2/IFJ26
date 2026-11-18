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
} dynamicString;

void dynStr_ctor(dynamicString *dynStr);
void dynStr_load(dynamicString *dynStr, char input);
void dynStr_freeBuff(dynamicString *dynStr);
char *dynStr_output(const dynamicString *dynStr);
void dynStr_dtor(dynamicString *dynStr);

#endif
