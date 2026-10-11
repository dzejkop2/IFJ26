/**
 * @name ifj26
 * @authors Marek Rzeszoto (xrzeszm00)
 * @file scanner.c
 */

#include "stdio.h"
#include "stdlib.h"
#include "scanner.h"
#include "error.h"
#include "dynamicString.h"

#define REALLOC_MEM 4096
#define KEYWORD_COUNT (sizeof(keywords) / sizeof(keywords[0]))

static stdInput input;
static dynamicString *dynStr;
static const Keyword keywords[] = {
    {"import",    abs_keyword,  kw_import},
    {"return",    abs_keyword,  kw_return},
    {"do",        abs_keyword,  kw_do},
    {"while",     abs_keyword,  kw_while},
    {"else",      abs_keyword,  kw_else},
    {"if",        abs_keyword,  kw_if},
    {"then",      abs_keyword,  kw_then},
    {"var",       abs_keyword,  kw_var},
    {"val",       abs_keyword,  kw_val},
    {"def",       abs_keyword,  kw_def},
    {"@main",     abs_keyword,  kw_main},
    {"for",       abs_keyword,  kw_for},
    {"extension", abs_keyword,  kw_extension},
    {"true",      abs_literal,  lit_true},
    {"false",     abs_literal,  lit_false},
    {"Int",       abs_dataType, dt_int},
    {"Double",    abs_dataType, dt_double},
    {"String",    abs_dataType, dt_string},
    {"Unit",      abs_dataType, dt_unit},
    {"Boolean",   abs_dataType, dt_bool},
    {"case",      abs_reserved, res_case},
    {"class",     abs_reserved, res_class},
    {"List",      abs_reserved, res_list},
    {"match",     abs_reserved, res_match},
    {"type",      abs_reserved, res_type},
    {NULL,        0,            0}
};

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


void scannerInit(){
    loadInput(stdin);
    dynStr_ctor(&dynStr);
}

Token getToken(){
    Token helpToken;
    int currentSign = bufferPeeker(0);

    while (true){
        switch (currentSign){
        case STATE_START:
            if (currentSign == '_'){
                helpToken.absType = abs_identifier;
                helpToken.specType = id_idtName;
            }
            else if( currentSign == '"'){}
            else if( currentSign == '\n'){}
            else if( currentSign == '<'){}
            else if( currentSign == '>'){}
            else if( currentSign == '='){}
            else if( currentSign == '+'){}
            else if( currentSign == '-'){}
            else if( currentSign == '*'){}
            else if( currentSign == '/'){}
            else if( currentSign == '&'){}
            else if( currentSign == '|'){}
            else if( currentSign == '!'){}
            else if( (int)currentSign >= 48 && (int)currentSign <= 57){}
            else if( currentSign >= 58 && currentSign <= 90){}
            else if( currentSign >= 97 && currentSign <= 122){}
            else if ( currentSign == ' ' || currentSign == '\t' || currentSign == '\r')

            
            break;
        
        default:
            break;
        }
    }
    
}

Token makeToken(Token helpToken){
    Token newToken;
    newToken.absType = helpToken.absType;
    newToken.specType = helpToken.specType;
    newToken.value = helpToken.value;
    newToken.line = helpToken.line;
    newToken.col = helpToken.col;
    return newToken;
}

void skip(){
    if (bufferPeeker(0) == ' ' || bufferPeeker(0) == '\t' || bufferPeeker(0) == '\r'){
        bufferAdvance();
    }   
}

void scanIdentifier(Token helpToken, Token newToken, Keyword keywords){
    Keyword* keywordsReturnValue = findKeyword(helpToken.value.word);
    if(keywordsReturnValue != NULL){
        newToken.absType = keywordsReturnValue->absType;
        newToken.specType = keywordsReturnValue->specType;
        newToken.line = helpToken.line;
        newToken.col = helpToken.col;
        newToken.value.word = helpToken.value.word;
    }
}
// void scanNumber(){

// }
// void scanString(){

// }
Keyword *findKeyword(const char *word) {
    for (int keyWordHelper = 0; keywords[keyWordHelper].word != NULL; keyWordHelper++) {
        if (strcmp(word, keywords[keyWordHelper].word) == 0) {
            return &keywords[keyWordHelper];
        }
    }
    return NULL;
}

void scanner_dtor(){
    free(input.loadedInput); 
}

void scannerFree(){
    scanner_dtor();
    dynStr_dtor(&dynStr);
}