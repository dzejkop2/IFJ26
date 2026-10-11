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
static const keyword keywords[] = {
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

token getToken(void) {
    states state = STATE_START;
    token helpToken;
    helpToken.value.word = NULL;
    dynStr_freeBuff(&dynStr);
 
    while (1) {
        int currentSign = bufferPeeker(0);
 
        switch (state) {
            case STATE_START:
                helpToken.line = input.line;
                helpToken.col = input.col;

                if (currentSign == ' ' || currentSign == '\t' || currentSign == '\r') {
                    bufferAdvance();            
                } else if (currentSign == EOF) {
                    helpToken.absType = abs_position;
                    helpToken.specType = ps_eof;
                    helpToken.line = input.line;
                    helpToken.col = input.col;
                    return makeToken(helpToken);
                } else if (currentSign <= 'A' && currentSign >= 'Z' || currentSign <= 'a' && currentSign >= 'z') {
                    storeSign();
                    state = STATE_IDENTIFIER;
                } else if (currentSign == '_') {
                    storeSign();
                    state = STATE_UNDERSCORE;
                }else if (currentSign == '@') {
                    bufferAdvance();
                    state = STATE_MAIN;
                
                //abs_sign
                } else if (currentSign == '\n') {
                    bufferAdvance();
                    helpToken.absType = abs_position;
                    helpToken.specType = ps_newLine;
                    helpToken.line = input.line;
                    helpToken.col = input.col;
                    return makeToken(helpToken);
                } else if (currentSign == '(') {
                    bufferAdvance();
                    helpToken.absType = abs_bracet;
                    helpToken.specType = br_start;
                    helpToken.line = input.line;
                    helpToken.col = input.col;
                    return makeToken(helpToken);
                } else if (currentSign == ')') {
                    bufferAdvance();
                    helpToken.absType = abs_bracet;
                    helpToken.specType = br_end;
                    helpToken.line = input.line;
                    helpToken.col = input.col;
                    return makeToken(helpToken);
                } else if (currentSign == ',') {
                    bufferAdvance();
                    helpToken.absType = abs_sign;
                    helpToken.specType = sn_comma;
                    helpToken.line = input.line;
                    helpToken.col = input.col;
                    return makeToken(helpToken);
                } else if (currentSign == ':') {
                    bufferAdvance();
                    helpToken.absType = abs_sign;
                    helpToken.specType = sn_colon;
                    helpToken.line = input.line;
                    helpToken.col = input.col;
                    return makeToken(helpToken);
                } else if (currentSign == ';') {
                    bufferAdvance();
                    helpToken.absType = abs_sign;
                    helpToken.specType = sn_semiColon;
                    helpToken.line = input.line;
                    helpToken.col = input.col;
                    return makeToken(helpToken);
                } else if (currentSign == '+') {
                    bufferAdvance();
                    helpToken.absType = abs_op;
                    helpToken.specType = op_add;
                    helpToken.line = input.line;
                    helpToken.col = input.col;
                    return makeToken(helpToken);
                } else if (currentSign == '-') {
                    bufferAdvance();
                    helpToken.absType = abs_op;
                    helpToken.specType = op_sub;
                    helpToken.line = input.line;
                    helpToken.col = input.col;
                    return makeToken(helpToken);
                } else if (currentSign == '*') {
                    bufferAdvance();
                    helpToken.absType = abs_op;
                    helpToken.specType = op_mul;
                    helpToken.line = input.line;
                    helpToken.col = input.col;
                    return makeToken(helpToken);
                } else if (currentSign == '/') {
                    bufferAdvance();
                    state = STATE_DIV;
                } else if (currentSign == '=') {
                    bufferAdvance();
                    state = STATE_ASSIGN;
                } else if (currentSign == '>') {
                    bufferAdvance();
                    state = STATE_BIGGER_THEN;
                } else if (currentSign == '<') {
                    bufferAdvance();
                    state = STATE_LOWER_THEN;
                } else if (currentSign == '!') {
                    bufferAdvance();
                    state = STATE_NOT;
                } else if (currentSign == '&') {
                    bufferAdvance();
                    state = STATE_AMP;
                } else if (currentSign == '|') {
                    bufferAdvance();
                    state = STATE_PIPE;
                } else if (currentSign == '.'){
                    bufferAdvance();
                    state = STATE_DOUBLE_DOT;
                } else if (currentSign >= '1' && currentSign <= '9'){
                    bufferAdvance();
                    state = STATE_INT;
                } else if (currentSign == 0){
                    bufferAdvance();
                    state = STATE_ZERO;
                }else {
                    error(LEXICAL_ERROR, "Unexpected character");
                }
                break;
            
            //abs_identifier
            case STATE_IDENTIFIER:
                if (( (int)currentSign>='0'&& (int)currentSign<='9' || currentSign>='A'&&currentSign<='Z' || currentSign>='a'&&currentSign<='z') || currentSign == '_') {
                    storeSign();
                } else {
                    char *word = dynStr_output(&dynStr);
                    const keyword *keywords = findKeyword(word);
                    if (keywords != NULL) {
                        free(word);
                        helpToken.absType = keywords->absType;
                        helpToken.specType = keywords->specType;
                    } else {
                        helpToken.absType = abs_identifier;
                        helpToken.specType = id_idtName;
                        helpToken.value.word = word;
                    }
                    helpToken.line = input.line;
                    helpToken.col = input.col;
                    return makeToken(helpToken);
                }
                break;
 
            case STATE_UNDERSCORE:
                if (( (int)currentSign>='0'&& (int)currentSign<='9' || currentSign>='A'&&currentSign<='Z' || currentSign>='a'&&currentSign<='z') || currentSign == '_') {
                    storeSign();
                    state = STATE_IDENTIFIER;
                } else {
                    helpToken.absType = abs_sign;
                    helpToken.specType = sn_underscore;
                    helpToken.line = input.line;
                    helpToken.col = input.col;
                    return makeToken(helpToken);
                }
                break;
 
            case STATE_MAIN:
                if (bufferPeeker(0) == 'm') {
                    bufferAdvance();
                } else {
                    error(LEXICAL_ERROR, "Expected 'm' after '@'");
                }

                if (bufferPeeker(0) == 'a') {
                    bufferAdvance();
                } else {
                    error(LEXICAL_ERROR, "Expected 'a' after '@m'");
                }

                if (bufferPeeker(0) == 'i') {
                    bufferAdvance();
                } else {
                    error(LEXICAL_ERROR, "Expected 'i' after '@ma'");
                }

                if (bufferPeeker(0) == 'n') {
                    bufferAdvance();
                } else {
                    error(LEXICAL_ERROR, "Expected 'n' after '@mai'");
                }

                char nextChar = bufferPeeker(0);
                if (( (int)currentSign>='0'&& (int)currentSign<='9' || currentSign>='A'&&currentSign<='Z' || currentSign>='a'&&currentSign<='z') || nextChar == '_') {
                    error(LEXICAL_ERROR, "Unexpected identifier after '@'");
                }
                helpToken.absType = abs_keyword;
                helpToken.specType = kw_main;
                helpToken.line = input.line;
                helpToken.col = input.col;
                break;
            
            //abs_op
            case STATE_DIV:
                if (currentSign == '/') {
                    bufferAdvance();
                    state = STATE_LINECOMMENT;
                } else if (currentSign == '*') {
                    bufferAdvance();
                    int commentDepth;
                    commentDepth = 1;
                    state = STATE_BLOCKCOMMENT_MID;
                } else {
                    helpToken.absType = abs_op;
                    helpToken.specType = op_div;
                    return makeToken(helpToken);
                }
                break;
                
            case STATE_ASSIGN:
                if (currentSign == '=') {
                    bufferAdvance();
                    helpToken.absType = abs_op;
                    helpToken.specType = op_equal;
                    helpToken.line = input.line;
                    helpToken.col = input.col;
                    return makeToken(helpToken);
                }
                helpToken.absType = abs_op;
                helpToken.specType = op_assign;
                helpToken.line = input.line;
                helpToken.col = input.col;
                return makeToken(helpToken);
 
            case STATE_BIGGER_THEN:
                if (currentSign == '=') {
                    bufferAdvance();
                    helpToken.absType = abs_op;
                    helpToken.specType = op_biggerEqual;
                    helpToken.line = input.line;
                    helpToken.col = input.col;
                    return makeToken(helpToken);
                }
                helpToken.absType = abs_op;
                helpToken.specType = op_biggerThen;
                helpToken.line = input.line;
                helpToken.col = input.col;
                return makeToken(helpToken);
 
            case STATE_LOWER_THEN:
                if (currentSign == '=') {
                    bufferAdvance();
                    helpToken.absType = abs_op;
                    helpToken.specType = op_lowerEqual;
                    helpToken.line = input.line;
                    helpToken.col = input.col;
                    return makeToken(helpToken);
                }
                if (currentSign == '-') {
                    bufferAdvance();
                    helpToken.absType = abs_control;
                    helpToken.specType = ct_arrowLeft;
                    helpToken.line = input.line;
                    helpToken.col = input.col;
                    return makeToken(helpToken);
                }
                helpToken.absType = abs_op;
                helpToken.specType = op_lowerThen;
                helpToken.line = input.line;
                helpToken.col = input.col;
                return makeToken(helpToken);
 
            case STATE_NOT:
                if (currentSign == '=') {
                    bufferAdvance();
                    helpToken.absType = abs_op;
                    helpToken.specType = op_notEqual;
                    helpToken.line = input.line;
                    helpToken.col = input.col;
                    return makeToken(helpToken);
                }
                helpToken.absType = abs_op;
                helpToken.specType = op_not;
                helpToken.line = input.line;
                helpToken.col = input.col;
                return makeToken(helpToken);
 
            case STATE_AMP:
                if (currentSign == '&') {
                    bufferAdvance();
                    helpToken.absType = abs_op;
                    helpToken.specType = op_and;
                    helpToken.line = input.line;
                    helpToken.col = input.col;
                    return makeToken(helpToken);
                }
                error(LEXICAL_ERROR, "Expected '&&'");
                break;
 
            case STATE_PIPE:
                if (currentSign == '|') {
                    bufferAdvance();
                    helpToken.absType = abs_op;
                    helpToken.specType = op_or;
                    helpToken.line = input.line;
                    helpToken.col = input.col;
                    return makeToken(helpToken);
                }
                error(LEXICAL_ERROR, "Expected '||'");
                break;
            
            //abs_comments
            case STATE_LINECOMMENT:
                if (currentSign == '\n' || currentSign == EOF) {
                    state = STATE_START;
                } else {
                    bufferAdvance();
                }
                break;

            case STATE_BLOCKCOMMENT_MID:
                if (currentSign == EOF) {
                    error(LEXICAL_ERROR, "Unterminated block comment");
                } else if (currentSign == '*') {
                    bufferAdvance();
                    state = STATE_BLOCKCOMMENT_END;
                } else if (currentSign == '/') {
                    bufferAdvance();
                    state = STATE_BLOCKCOMMENT_SLASH;
                } else {
                    bufferAdvance();
                }
                break;

            case STATE_BLOCKCOMMENT_END:
                if (currentSign == EOF) {
                    error(LEXICAL_ERROR, "Unterminated block comment");
                } else if (currentSign == '*') {
                    bufferAdvance();
                } else if (currentSign == '/') {
                    bufferAdvance();
                    int commentDepth;
                    commentDepth--;
                    if (commentDepth == 0) {
                        state = STATE_START;
                    } else {
                        state = STATE_BLOCKCOMMENT_MID;
                    }
                } else {
                    bufferAdvance();
                    state = STATE_BLOCKCOMMENT_MID;
                }
                break;

            case STATE_BLOCKCOMMENT_SLASH:
                if (currentSign == EOF) {
                    error(LEXICAL_ERROR, "Unterminated block comment");
                } else if (currentSign == '/') {
                    bufferAdvance();
                } else if (currentSign == '*') {
                    bufferAdvance();
                    int commentDepth;
                    commentDepth++;
                    state = STATE_BLOCKCOMMENT_MID;
                } else {
                    bufferAdvance();
                    state = STATE_BLOCKCOMMENT_MID;
                }
                break;

             case STATE_DOUBLE_DOT:
                if (currentSign == '.'){
                     bufferAdvance();
                    state = STATE_INT_DOT;
                }
                
             case STATE_ZERO:
                if (currentSign == '.'){
                    bufferAdvance();
                    state = STATE_INT_DOT;
                } else if (currentSign == 'e' || currentSign == 'E'){
                    bufferAdvance();
                    state = STATE_NUM_EXP;
                }
                
             case STATE_INT:  
                helpToken.absType = abs_literal;
                helpToken.specType = lit_int;
                helpToken.line = input.line;
                helpToken.col = input.col;
                
             case STATE_INT_DOT:
                if (currentSign >= '0' && currentSign <= '9'){
                    bufferAdvance();
                    if (currentSign >= '0' && currentSign <= '9'){
                        helpToken.absType = abs_literal;
                        helpToken.specType = lit_double;
                        helpToken.line = input.line;
                        helpToken.col = input.col;
                    } else if (currentSign == 'e' || currentSign == 'E'){
                        bufferAdvance();
                        state = STATE_NUM_EXP;
                    }
                }
            case STATE_NUM_EXP:
                
                
            default:
                error(INTERNAL_ERROR, "Scanner state not implemented");
                break;
        }
    }
}


token makeToken(token helpToken){
    token newToken;
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

void scanIdentifier(token helpToken, token newToken, keyword keywords){
    keyword* keywordsReturnValue = findKeyword(helpToken.value.word);
    if(keywordsReturnValue != NULL){
        newToken.absType = keywordsReturnValue->absType;
        newToken.specType = keywordsReturnValue->specType;
        newToken.line = helpToken.line;
        newToken.col = helpToken.col;
        newToken.value.word = helpToken.value.word;
    }
}

static void storeSign(void) {
    dynStr_load(&dynStr, (char)bufferAdvance());
}

keyword *findKeyword(const char *word) {    
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