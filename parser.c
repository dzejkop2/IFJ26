/**
 * @name ifj26
 * @authors Jakub Geleta (xgeletj00)
 * @file parser.c
 */

#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#include "scanner.h"
#include "error.h"
#include <string.h>

static parserState parser;

void parserCleanup(void) {
    if (!parser.active) return;
    parser.active = false;

    while (parser.unattachedCount > 0) {
        parser.unattachedCount--;
        ast_free(parser.unattached[parser.unattachedCount]);
    }
    free(parser.unattached);
    parser.unattached = NULL;
    parser.unattachedCapacity = 0;
}


astNode *parserTrackUnattached(astNode *node) {
    if (node == NULL) return NULL;

    if (parser.unattachedCount == parser.unattachedCapacity) {
        size_t newCapacity = parser.unattachedCapacity == 0 ? 16 : parser.unattachedCapacity * 2;
        astNode **newUnattached = realloc(parser.unattached, newCapacity * sizeof(astNode *));
        if (newUnattached == NULL) {
            ast_free(node);
            error(INTERNAL_ERROR, "Parser unattached node stack realloc memory failed");
        }
        parser.unattached = newUnattached;
        parser.unattachedCapacity = newCapacity;
    }

    parser.unattached[parser.unattachedCount++] = node;
    return node;
}

astNode *parserMarkAttached(astNode *node) {
    if (node == NULL) return NULL;

    for (size_t i = parser.unattachedCount; i > 0; i--) {
        if (parser.unattached[i - 1] == node) {
            memmove(&parser.unattached[i - 1], &parser.unattached[i], (parser.unattachedCount - i) * sizeof(astNode *));
            parser.unattachedCount--;
            return node;
        }
    }
    return node;
}

void parserListAppend(astNodeList *list, astNode *node) {
    ast_listAppend(list, node);
    parserMarkAttached(node);
}

astNode *parse(void) {
    static bool cleanupRegistered = false;
    if (!cleanupRegistered) {
        if (atexit(parserCleanup) != 0) error(INTERNAL_ERROR, "Failed to register parser cleanup");
        cleanupRegistered = true;
    }

    memset(&parser, 0, sizeof(parser));
    parser.active = true;

    scannerInit();

    astNode *program = parseProgram();

    if(program == NULL) {
        error(SYNTAX_ERROR, "Invalid program structure");
    } else {
        printf("Program parsed successfully.\n");
    }

    parserMarkAttached(program);
    parserCleanup();
    scannerFree();
    return program;
}


void parserAdvance(void) {
    parser.current = getToken();
}

bool parserCheck(specTokenType specType) {
    return parser.current.specType == specType;
}

bool parserMatch(specTokenType specType) {
    if (!parserCheck(specType)) return false;
    parserAdvance();
    return true;
}

void skipNL(void) {
    while(parserCheck(ps_newLine)) {
        parserAdvance();
    }
}

astNode *parseProgram(void) {
    parserAdvance();
    skipNL();

    if(!parserCheck(kw_import)) error(SYNTAX_ERROR, "Expected 'import' at the start of the program");
    int line = parser.current.line;
    int col = parser.current.col;
    parserAdvance();

    if(!parserCheck(id_idtName) || strcmp(parser.current.value.word, "ifj26") != 0)
        error(SYNTAX_ERROR, "Expected 'ifj26' after 'import'");

    astNode *program = parserTrackUnattached(ast_newProgram(NULL, line, col));
    program->data.program.importName = parser.current.value.word;
    parserAdvance();

    if(!parserMatch(sn_dot)) error(SYNTAX_ERROR, "Expected '.' after 'import ifj26'");
    if(!parserMatch(op_mul)) error(SYNTAX_ERROR, "Expected '*' after 'import ifj26.'");
    if(!parserCheck(ps_newLine)) error(SYNTAX_ERROR, "Expected newline after 'import ifj26.*'");

    parseGlobalDefs(&program->data.program.defs);

    if(!parserCheck(ps_eof)) error(SYNTAX_ERROR, "Expected end of file");

    return program;
}

void parseGlobalDefs(astNodeList *defs) {
    bool mainFound = false;

    skipNL();
    while(!parserCheck(ps_eof)) {
        astNode *def = parseGlobalDef();
        if(def == NULL) error(SYNTAX_ERROR, "Invalid global definition");

        parserListAppend(defs, def);

        if(def->type == AST_FUN_DEF && def->data.funDef.isMain) {
            if(mainFound) {
                error(SYNTAX_ERROR, "Multiple main functions found");
            }
            mainFound = true;
        }

        skipNL();
    }
}

astNode *parseGlobalDef(void) {
    if(parserCheck(kw_extension)) {
        return parseExtension();
    } else if(parserCheck(kw_def)) {
        return parseFuncDef(false);
    } else if(parserCheck(kw_main)) {
        parserAdvance();
        parserMatch(ps_newLine);
        if(!parserCheck(kw_def)) error(SYNTAX_ERROR, "Expected 'def' after '@main'");
        return parseFuncDef(true);
    }

    return NULL;
}

astNode *parseExtension(void) {
    int line = parser.current.line;
    int col = parser.current.col;
    parserAdvance();

    if(!parserMatch(br_start)) error(SYNTAX_ERROR, "Expected '(' after 'extension'");
    if(!parserCheck(id_idtName)) error(SYNTAX_ERROR, "Expected a identifier in extension");

    astNode *extension = parserTrackUnattached(ast_newExtension(NULL, line, col));
    extension->data.extension.receiver = parserMarkAttached(parseParam());

    if(!parserMatch(br_end)) error(SYNTAX_ERROR, "Expected ')' after extension");

    parseExtBody(extension);

    return extension;
}

void parseExtBody(astNode *extension) {
    printf("Parsing extension body\n");
    (void)extension;
}

astNode *parseFuncDef(bool isMain) {
    (void) isMain;
    printf("Parsing function definition (isMain: %s)\n", isMain ? "true" : "false");
    return NULL;
}

void parseParamLists(astNodeList *paramLists) {
    (void)paramLists;
}

void parseParams(astNodeList *params) {
    (void)params;
}

astNode *parseParam(void) {
    if(!parserCheck(id_idtName)) return NULL;

    astNode *param = parserTrackUnattached(ast_newParam(NULL, TYPE_NONE, parser.current.line, parser.current.col));
    param->data.param.name = parser.current.value.word;
    parserAdvance();

    if(!parserMatch(sn_colon)) error(SYNTAX_ERROR, "Expected ':' after parameter name");

    param->data.param.type = parseType();
    if(param->data.param.type == TYPE_NONE) error(SYNTAX_ERROR, "Expected type of parameter");

    return param;
}

astDataType parseTypeOpt(void) {
    return TYPE_NONE;
}

astDataType parseType(void) {
    astDataType type = ast_typeFromToken(parser.current.specType);
    if(type != TYPE_NONE) parserAdvance();
    return type;
}

astNode *parseBody(void) {
    return NULL;
}

astNode *parseBlock(void) {
    return NULL;
}

void parseStmt(astNodeList *stmts) {
    (void)stmts;
}

astNode *parseSimple(void) {
    return NULL;
}

astNode *parseInline(void) {
    return NULL;
}

astNode *parseVarDecl(bool isMutable) {
    (void)isMutable;
    return NULL;
}

astNode *parseReturn(void) {
    return NULL;
}

astNode *parseWhile(void) {
    return NULL;
}

astNode *parseValue(void) {
    return NULL;
}

astNode *parseIf(void) {
    return NULL;
}

astNode *parseFor(void) {
    return NULL;
}

void parseForHeader(astNode *forNode) {
    (void)forNode;
}

void parseGenerator(astNode *forNode) {
    (void)forNode;
}

void parseGuards(astNodeList *guards) {
    (void)guards;
}

void parseGuardLines(astNodeList *guards) {
    (void)guards;
}

astNode *parseExpr(void) {
    return NULL;
}

bool isExprStart(specTokenType specType) {
    (void)specType;
    return false;
}





// TOTO NEVIEM CI BUDE TREBA UVIDIME
token parserExpect(specTokenType specType, const char *errorMsg) {
    (void)specType;
    (void)errorMsg;
    return parser.current;
}
