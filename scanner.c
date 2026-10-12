/**
 * @name ifj26
 * @authors Marek Rzeszoto (xrzeszm00)
 * @file scanner.c
 */

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "scanner.h"
#include "error.h"
#include "dynamicString.h"

#define REALLOC_MEM 4096
#define KEYWORD_COUNT (sizeof(keywords) / sizeof(keywords[0]))

static stdInput input;
static dynamicString dynStr;

static void storeSign(void);
static token makeNumber(token helpToken, specTokenType numType);
static token makeString(token helpToken);
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
    {"to",        abs_control,  ct_to},
    {"until",     abs_control,  ct_until},
    {"by",        abs_control,  ct_by},
    {NULL,        0,            0}
};

static void loadInput(FILE *sourceFile){

    size_t reallocHelperCapacity = REALLOC_MEM;
    input.loadedInput = malloc(reallocHelperCapacity);
    if (input.loadedInput==NULL){
        error(INTERNAL_ERROR, "Scanners help buffer malloc memory failed");
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
            if (reallocHelper==NULL){
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
    int commentDepth = 0;
    int newlinePending = 0;

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
                } else if ((currentSign >= 'A' && currentSign <= 'Z') || (currentSign >= 'a' && currentSign <= 'z') || currentSign == '@') {
                    storeSign();
                    state = STATE_IDENTIFIER;
                } else if (currentSign == '_') {
                    storeSign();
                    state = STATE_UNDERSCORE;
                
                //abs_sign
                } else if (currentSign == '\n') {
                    bufferAdvance();
                    newlinePending = 1;
                    state = STATE_NEWLINE;
                } else if (currentSign == '"') {
                    bufferAdvance();
                    state = STATE_QUOTES_START;
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
                    storeSign();
                    state = STATE_DOUBLE_DOT;
                } else if (currentSign >= '1' && currentSign <= '9'){
                    storeSign();
                    state = STATE_INT;
                } else if (currentSign == '0'){
                    storeSign();
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
                    const keyword *foundKeyword = findKeyword(word);
                    if (foundKeyword != NULL) {
                        free(word);
                        helpToken.absType = foundKeyword->absType;
                        helpToken.specType = foundKeyword->specType;
                    } else if (word[0] == '@') {
                        free(word);
                        error(LEXICAL_ERROR, "Only '@main' can start with '@'");
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
 
            //abs_op
            case STATE_DIV:
                if (currentSign == '/') {
                    bufferAdvance();
                    state = STATE_LINECOMMENT;
                } else if (currentSign == '*') {
                    bufferAdvance();
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
                if (currentSign == EOF) {
                    if (newlinePending) {
                        state = STATE_NEWLINE;
                    } else {
                        state = STATE_START;
                    }
                } else if (currentSign == '\n') {
                    newlinePending = 1;
                    bufferAdvance();
                    state = STATE_NEWLINE;
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
                    commentDepth++;
                    state = STATE_BLOCKCOMMENT_MID;
                } else {
                    bufferAdvance();
                    state = STATE_BLOCKCOMMENT_MID;
                }
                break;

            //abs_position
            case STATE_NEWLINE:
                if (currentSign == ' ' || currentSign == '\t' || currentSign == '\n' || currentSign == '\r') {
                    bufferAdvance();
                } else if (currentSign == '/' && bufferPeeker(1) == '/') {
                    state = STATE_LINECOMMENT;
                } else {
                    helpToken.absType = abs_position;
                    helpToken.specType = ps_newLine;
                    return makeToken(helpToken);
                }
                break;

            //num literals
            case STATE_DOUBLE_DOT:
                if (currentSign >= '0' && currentSign <= '9') {
                    storeSign();
                    state = STATE_DOUBLE_FRAC;
                } else {
                    helpToken.absType = abs_sign;
                    helpToken.specType = sn_dot;
                    return makeToken(helpToken);
                }
                break;

            case STATE_ZERO:
                if (currentSign == '.') {
                    storeSign();
                    state = STATE_INT_DOT;
                } else if (currentSign == 'e' || currentSign == 'E') {
                    storeSign();
                    state = STATE_NUM_EXP;
                } else {
                    return makeNumber(helpToken, lit_int);
                }
                break;

            case STATE_INT:
                if (currentSign >= '0' && currentSign <= '9') {
                    storeSign();
                } else if (currentSign == '_') {
                    bufferAdvance();
                    state = STATE_INT_UNDERSCORE;
                } else if (currentSign == '.') {
                    storeSign();
                    state = STATE_INT_DOT;
                } else if (currentSign == 'e' || currentSign == 'E') {
                    storeSign();
                    state = STATE_NUM_EXP;
                } else {
                    return makeNumber(helpToken, lit_int);
                }
                break;

            case STATE_INT_UNDERSCORE:
                if (currentSign >= '0' && currentSign <= '9') {
                    storeSign();
                    state = STATE_INT;
                } else {
                    error(LEXICAL_ERROR, "Expected digit after '_' in number");
                }
                break;

            case STATE_INT_DOT:
                if (currentSign >= '0' && currentSign <= '9') {
                    storeSign();
                    state = STATE_DOUBLE_FRAC;
                } else {
                    error(LEXICAL_ERROR, "Expected digit after '.' in number");
                }
                break;

            case STATE_DOUBLE_FRAC:
                if (currentSign >= '0' && currentSign <= '9') {
                    storeSign();
                } else if (currentSign == '_') {
                    bufferAdvance();
                    state = STATE_FRAC_UNDERSCORE;
                } else if (currentSign == 'e' || currentSign == 'E') {
                    storeSign();
                    state = STATE_NUM_EXP;
                } else if (currentSign == 'd' || currentSign == 'D') {
                    bufferAdvance();
                    state = STATE_DOUBLE_SUFFIX;
                } else {
                    return makeNumber(helpToken, lit_double);
                }
                break;

            case STATE_FRAC_UNDERSCORE:
                if (currentSign >= '0' && currentSign <= '9') {
                    storeSign();
                    state = STATE_DOUBLE_FRAC;
                } else {
                    error(LEXICAL_ERROR, "Expected digit after '_' in number");
                }
                break;

            case STATE_NUM_EXP:
                if (currentSign >= '0' && currentSign <= '9') {
                    storeSign();
                    state = STATE_DOUBLE_EXP;
                } else if (currentSign == '+' || currentSign == '-') {
                    storeSign();
                    state = STATE_SIGN_NUM_EXP;
                } else {
                    error(LEXICAL_ERROR, "Expected digit or sign in exponent");
                }
                break;

            case STATE_SIGN_NUM_EXP:
                if (currentSign >= '0' && currentSign <= '9') {
                    storeSign();
                    state = STATE_DOUBLE_EXP;
                } else {
                    error(LEXICAL_ERROR, "Expected digit in exponent");
                }
                break;

            case STATE_DOUBLE_EXP:
                if (currentSign >= '0' && currentSign <= '9') {
                    storeSign();
                } else if (currentSign == '_') {
                    bufferAdvance();
                    state = STATE_EXP_UNDERSCORE;
                } else if (currentSign == 'd' || currentSign == 'D') {
                    bufferAdvance();
                    state = STATE_DOUBLE_SUFFIX;
                } else {
                    return makeNumber(helpToken, lit_double);
                }
                break;

            case STATE_EXP_UNDERSCORE:
                if (currentSign >= '0' && currentSign <= '9') {
                    storeSign();
                    state = STATE_DOUBLE_EXP;
                } else {
                    error(LEXICAL_ERROR, "Expected digit after '_' in number");
                }
                break;

            case STATE_DOUBLE_SUFFIX:
                return makeNumber(helpToken, lit_double);

            //strings
            case STATE_QUOTES_START:
                if (currentSign == '"') {
                    bufferAdvance();
                    state = STATE_STRING_EMPTY;
                } else {
                    state = STATE_QUOTES_BODY;
                }
                break;

            case STATE_QUOTES_BODY:
                if (currentSign == EOF) {
                    error(LEXICAL_ERROR, "Unterminated string");
                } else if (currentSign == '"') {
                    bufferAdvance();
                    state = STATE_STRING_END;
                } else if (currentSign == '\\') {
                    bufferAdvance();
                    state = STATE_QUOTES_ESCAPE;
                } else {
                    storeSign();
                }
                break;

            case STATE_QUOTES_ESCAPE:
                if (currentSign == 'n') {
                    dynStr_load(&dynStr, '\n');
                } else if (currentSign == 'r') {
                    dynStr_load(&dynStr, '\r');
                } else if (currentSign == 't') {
                    dynStr_load(&dynStr, '\t');
                } else if (currentSign == '"' || currentSign == '\\') {
                    dynStr_load(&dynStr, (char)currentSign);
                } else {
                    error(LEXICAL_ERROR, "Unknown escape sequence in string");
                }
                bufferAdvance();
                state = STATE_QUOTES_BODY;
                break;

            case STATE_STRING_END:
                return makeString(helpToken);

            case STATE_STRING_EMPTY:
                if (currentSign == '"') {
                    bufferAdvance();
                    state = STATE_MUL_STRING_START;
                } else {
                    return makeString(helpToken);
                }
                break;

            case STATE_MUL_STRING_START:
                if (currentSign == EOF) {
                    error(LEXICAL_ERROR, "Unterminated multiline string");
                } else if (currentSign == '"') {
                    bufferAdvance();
                    state = STATE_MUL_STRING_1;
                } else {
                    storeSign();
                }
                break;

            case STATE_MUL_STRING_1:
                if (currentSign == '"') {
                    bufferAdvance();
                    state = STATE_MUL_STRING_2;
                } else {
                    dynStr_load(&dynStr, '"');
                    state = STATE_MUL_STRING_START;
                }
                break;

            case STATE_MUL_STRING_2:
                if (currentSign == '"') {
                    bufferAdvance();
                    state = STATE_MUL_STRING_END;
                } else {
                    dynStr_load(&dynStr, '"');
                    dynStr_load(&dynStr, '"');
                    state = STATE_MUL_STRING_START;
                }
                break;

            case STATE_MUL_STRING_END:
                return makeString(helpToken);

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
    const keyword* keywordsReturnValue = findKeyword(helpToken.value.word);
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

static token makeNumber(token helpToken, specTokenType numType) {
    char *word = dynStr_output(&dynStr);
    helpToken.absType = abs_literal;
    helpToken.specType = numType;
    if (numType == lit_int) {
        helpToken.value.num = strtoll(word, NULL, 10);
    } else {
        helpToken.value.decNum = strtod(word, NULL);
    }
    free(word);
    return makeToken(helpToken);
}

static token makeString(token helpToken) {
    helpToken.absType = abs_literal;
    helpToken.specType = lit_string;
    helpToken.value.word = dynStr_output(&dynStr);
    return makeToken(helpToken);
}

const keyword *findKeyword(const char *word) {    
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