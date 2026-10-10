/**
 * @name ifj26
 * @authors Marek Rzeszoto (xrzeszm00)
 * @file scanner.c
 */

#include "stdio.h"
#include "stdlib.h"
#include "scanner.h"
#include "error.h"

#define REALLOC_MEM 4096

typedef struct
{
    char * loadedInput;
    size_t lenght;
    size_t pos;
    int col;
    int line;
} stdInput;

static stdInput input;

static void loadInput(FILE *sourceFile){

    size_t reallocHelperCapacity = REALLOC_MEM;
    input.loadedInput = malloc(reallocHelperCapacity);
    if (reallocHelperCapacity==NULL){
    error(INTERNAL_ERROR, "Scanners help buffer realloc memory failed");
    }   
    input.lenght = 0;
    input.pos = 0;
    input.line = 1;
    input.col = 1;

    int count = 0;
    while((count = fread(input.loadedInput+input.lenght,sizeof(char), reallocHelperCapacity - input.lenght, sourceFile))>0){
        input.lenght=input.lenght+count;
        if (input.lenght == reallocHelperCapacity){
            reallocHelperCapacity = reallocHelperCapacity *2;
            char *reallocHelper = realloc(input.loadedInput, reallocHelperCapacity);
            if (input.loadedInput==NULL){
                error(INTERNAL_ERROR, "Scanners help buffer realloc memory failed");
            }
            input.loadedInput = reallocHelper;
        }
        
    }
}

int bufferPeeker(int offset){
    if ((input.pos + offset) >= input.lenght){
        return EOF;
    }
    else{
        return (unsigned char)input.loadedInput[input.pos + offset];
    }
}


int bufferAdvance(){
    int peekHelper = bufferPeeker(0);
    if (peekHelper == EOF){
        return EOF;
    }
    input.pos++;
    if (peekHelper == '\n') {
        input.line++;
        input.col = 1;
    } else {
        input.col++;
    }
    return peekHelper;   
}


char scanner_ctor(){

}

Token getToken(){

}
  

char scanner_dtor(){
    
}